
#include <math.h>
#include "sql_router.h"
#include "myproxy_config.h"
#include "dbg_log.h"
#include "env.h"
#include "dnmgr.h"

using namespace GLOBAL_ENV;

sql_router::sql_router(safeDataNodeList &lst):
  m_nodes(lst),m_handlers(0)
{
  register_rule_handlers();
}

sql_router::~sql_router(void)
{
  unregister_rule_handlers();
}

int sql_router::register_rule_handlers(void)
{
  m_handlers = new ruleHandler_t[t_maxRules];

  /* the dummy rule */
  m_handlers[t_dummy].ha = &sql_router::ha_rule_dummy ;
  /* the range-mapping rule */
  m_handlers[t_rangeMap].ha = &sql_router::ha_rule_rangeMap ;
  /* the mod(N) rule */
  m_handlers[t_modN].ha = &sql_router::ha_rule_modN ;
  /* TODO: more rule types */
  return 0;
}

void sql_router::unregister_rule_handlers(void)
{
  if (m_handlers) {
    delete [] m_handlers ;
    m_handlers = 0;
  }
}

int sql_router::ha_rule_rangeMap(SHARDING_VALUE *psv, std::set<uint8_t> &slst)
{
  return 0;
}

int sql_router::ha_rule_modN(SHARDING_VALUE *psv, std::set<uint8_t> &slst)
{
  int i=0, dn = 0;
  size_t total_dns = m_nodes.size();

  /* no data nodes */
  if (total_dns<=0) {
    return -1;
  }
  /* only use data node 0 and 1 with mod(2) method: 
   *
   *   if the sharding value is even, use data node 0
   *
   *   if the sharding value is odd, use data node 1
   */
  for (i=0;i<MAX_SV_ITEMS&&i<psv->num_vals;i++) {
    /* assign sharding values by mod(dn-count) */
    if (get_route_by_modN(psv->type,&psv->sv[i],dn,total_dns)) {
      continue ;
    }
    log_print("route to datanode %d, mod op %zu\n", dn, total_dns);
    slst.insert(dn);
  }
  return 0;
}

int sql_router::get_route_by_modN(int type,sv_t *psv, int &dn, int mod_op)
{
  auto modN = [&](auto m, auto n) {
    auto p = n-n;
    for (;m>=(p+n);p+=n) ;
    return m-p;
  } ;

  switch (type) {
    case MYSQL_TYPE_TINY:  
      dn = modN(psv->u.v8,mod_op);
      //log_print("val8: %d\n",psv->u.v8);
      break;

    case MYSQL_TYPE_SHORT: 
      dn = modN(psv->u.v16,mod_op);
      //log_print("val16: %d\n",psv->u.v16);
      break ;

    case MYSQL_TYPE_LONG:
      dn = modN(psv->u.v32,mod_op);
      //log_print("val32: %ld\n",psv->u.v32);
      break ;

    case MYSQL_TYPE_LONGLONG:
      dn = modN(psv->u.v64,mod_op);
      //log_print("val64: %lld\n",psv->u.v64);
      break ;

    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_FLOAT:
      dn = (int)/*fmod*/modN(psv->u.vf,mod_op);
      //log_print("valf: %f\n",psv->u.vf);
      break ;

    case MYSQL_TYPE_DOUBLE:
      dn = (int)/*fmod*/modN(psv->u.vd,mod_op);
      //log_print("vald: %f\n",psv->u.vd);
      break ;

    default:
      /* XXX: unsupported column type */
      log_print("unknown type %d\n",type);
      return -1;
  }
  return 0;
}

int sql_router::ha_rule_dummy(SHARDING_VALUE *psv, std::set<uint8_t> &slst)
{
  int i=0, dn = 0;

  /* only use data node 0 and 1 with mod(2) method: 
   *
   *   if the sharding value is even, use data node 0
   *
   *   if the sharding value is odd, use data node 1
   */
  for (i=0;i<MAX_SV_ITEMS&&i<psv->num_vals;i++) {
    /* assign sharding values by mod(2) */
    if (get_route_by_modN(psv->type,&psv->sv[i],dn,2)) {
      continue ;
    }
    log_print("route to datanode %d\n", dn);
    slst.insert(dn);
  }
  return 0;
}

int sql_router::get_full_route_by_conf(
  tSqlParseItem *sp,
  std::set<uint8_t> &rlist
  )
{
  /* calculate routes of related tables in statement by configs */
  if (get_related_table_route(sp,rlist)) {
    log_print("get table route fail\n");
    return -1;
  }

  return 0;
}

int sql_router::get_full_route(std::set<uint8_t> &lst)
{
  /* get route to all data nodes */
  safe_container_base<int,tDNInfo*>::ITR_TYPE itr ;

  /* add routes for all data nodes */
  for (tDNInfo *pd=m_nodes.next(itr,true);pd;
    pd=m_nodes.next(itr,false)) {

    if (pd->stat!=s_free)
      continue;

    lst.insert(pd->no);
  }

  return 0;
}

int sql_router::calc_intersection(
  std::set<uint8_t> &rlist, /* the ultimated route list */
  std::set<uint8_t> &slst /* the route set of a single sharding value */
  )
{
  std::set<uint8_t> tmp{} ;

  /* initial state: no items in ultimated route list */
  if (rlist.empty()) {
    rlist = slst ;
    return 0;
  }

  for (auto n : slst) {
    /* find matching val in rlist */
    for (auto m : rlist) {
      if (m==n) {
        tmp.insert(n);
        break ;
      }
    }
  }

  rlist = tmp ;

  return 0;
}

int 
sql_router::get_related_table_route(tSqlParseItem *sp, std::set<uint8_t> &rlst)
{
  TABLE_NAME *p_tn = 0;

  /* iterates each related table */
  for (p_tn=sp->m_tblKeyLst.next(true);p_tn;p_tn=sp->m_tblKeyLst.next()) {

    /* get mapping info from table list */
    tTblDetails *td = m_tables.get(p_tn->sch.c_str(),p_tn->tbl.c_str());
    tDnMappings *pm = 0;
    std::set<uint8_t> tr ;

    for (pm=m_tables.first_map(td);pm;pm=m_tables.next_map(pm)) {

      if (pm->dn<0) {
        log_print("fatal: dn number of '%s' invalid!\n",td->table.c_str());
        continue ;
      }

      log_print("fetch dn %d for `%s.%s`\n",pm->dn,
        p_tn->sch.c_str(),p_tn->tbl.c_str());

      tr.insert(pm->dn);
    }

    calc_intersection(rlst,tr);

  } // end for()

  return 0;
}

int 
sql_router::get_sharding_route(tSqlParseItem *sp, std::set<uint8_t> &rlist)
{
  uint16_t i = 0;
  SHARDING_KEY *psk = 0;
  SHARDING_VALUE *psv = 0;

  /* iterates each sharding values in list */
  for (psv=0,i=0;i<sp->get_num_sv();i++) {

    psv = sp->get_sv(i) ;
    /* related sharding column */
    if (!(psk=psv->sk)) {
      continue ;
    }

    /* invalid rule type */
    if (psk->rule>=t_maxRules) {
      continue ;
    }

    /* get route set of a single sharding value */
    ruleHandler ha = m_handlers[psk->rule].ha ;
    (this->*ha)(psv,rlist);

  } /* end for(...) */
  return 0;
}

int sql_router::get_route(int cid,tSqlParseItem *sp, 
  std::set<uint8_t> &rlist)
{
  std::set<uint8_t> tr{} ;

  rlist.clear();

  /* calculate routes by sharding column values */
  if (get_sharding_route(sp,rlist)) {
    log_print("get sharding route fail\n");
    return -1;
  }

#if 1
  /* XXX: test */
  {
    log_print("content of route list000: \n");
    for (auto i : rlist) {
      log_print(">>> %d \n",i);
    }
    log_print("\n\n");
  }
#endif

  /* calculate routes of related tables in statement by configs */
  if (get_related_table_route(sp,tr)) {
    log_print("get table route fail\n");
    return -1;
  }

#if 1
  /* XXX: test */
  {
    log_print("content of route list111: \n");
    for (auto i : tr) {
      log_print(">>> %d \n",i);
    }
    log_print("\n\n");
  }
#endif

  /* get intersection with sharding datanodes and configured datanodes */
  calc_intersection(rlist,tr);

  /* if the rlist is empty, then give it a full route */
  if (unlikely(rlist.empty())) {

    if (tr.size()==0) {
      log_print("fatal: no route!\n");
      return -1;
    }

    log_print("route list's empty, given a full route (count %zu)\n",tr.size());

    rlist = tr ;
  }

#if 1
  /* XXX: test */
  {
    log_print("content of route list: \n");
    for (auto i : rlist) {
      log_print(">>> %d \n",i);
    }
    log_print("\n\n");
  }
#endif
  return 0;
}

