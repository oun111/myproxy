
#include <math.h>
#include "sql_router.h"
#include "myproxy_config.h"
#include "dbg_log.h"

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

int sql_router::ha_rule_rangeMap(SHARDING_VALUE *psv, std::vector<uint8_t> &slst)
{
  return 0;
}

int sql_router::ha_rule_modN(SHARDING_VALUE *psv, std::vector<uint8_t> &slst)
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
    slst.push_back(dn);
  }
  return 0;
}

int sql_router::get_route_by_modN(int type,sv_t *psv, int &dn, int mod_op)
{
#define modN(m,n) (m%n)
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
      dn = (int)fmod(psv->u.vf,mod_op);
      //log_print("valf: %f\n",psv->u.vf);
      break ;

    case MYSQL_TYPE_DOUBLE:
      dn = (int)fmod(psv->u.vd,mod_op);
      //log_print("vald: %f\n",psv->u.vd);
      break ;

    default:
      /* XXX: unsupported column type */
      log_print("unknown type %d\n",type);
      return -1;
  }
  return 0;
}

int sql_router::ha_rule_dummy(SHARDING_VALUE *psv, std::vector<uint8_t> &slst)
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
    slst.push_back(dn);
  }
  return 0;
}

int sql_router::get_full_route(/*int cid,*/std::vector<uint8_t> &rlist)
{
  /* get route to all data nodes */
  safe_container_base<int,tDNInfo*>::ITR_TYPE itr ;
  tDNInfo *pd = m_nodes.next(itr,true);

  /* add routes for all data nodes */
  for (;pd;pd=m_nodes.next(itr,false)) 
    rlist.push_back(pd->no);
  return 0;
}

int sql_router::calc_intersection(
  std::vector<uint8_t> &rlist, /* the ultimated route list */
  std::vector<uint8_t> &slst /* the route set of a single sharding value */
  )
{
  uint16_t n=0,m=0;
  uint8_t val = 0;
  std::vector<uint8_t> tmp ;

  /* initial state: no items in ultimated route list */
  if (rlist.empty()) {
    rlist = slst ;
    return 0;
  }
  for (n=0;n<slst.size();n++) {
    val = slst[n];
    /* find matching val in rlist */
    for (m=0;m<rlist.size();m++) {
      if (rlist[m]==val) {
        tmp.push_back(val);
        break ;
      }
    }
  }
  rlist = tmp ;
  return 0;
}

int sql_router::get_route(int cid,tSqlParseItem *sp, 
  std::vector<uint8_t> &rlist)
{
  uint16_t i = 0;
  SHARDING_VALUE *psv = 0;
  SHARDING_KEY *psk = 0;
  ruleHandler ha ;
  /* routing list of a single sharding value */
  std::vector<uint8_t> slst ;

  rlist.clear();

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
    /* reset the single route list */
    slst.clear();
    /* get route set of a single sharding value */
    ha = m_handlers[psk->rule].ha ;
    (this->*ha)(psv,slst);
    /* 
     * calculate intersection with 'rlist' and 'slst'
     */
    calc_intersection(rlist,slst);
  } /* end for(...) */
  /* if the rlist is empty, then give it a full route */
  if (rlist.empty()) {
    log_print("route list's empty, give a full route "
      "to all data nodes\n");
    get_full_route(rlist);
  }
#if 0
  /* XXX: test */
  {
    log_print("content of route list(dn): ");
    for (i=0;i<rlist.size();i++) {
      log_print(" %d ",rlist[i]);
    }
    log_print("\n\n");
  }
#endif
  return 0;
}

