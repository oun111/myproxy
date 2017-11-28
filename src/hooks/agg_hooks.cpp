
#include "agg_hooks.h"
#include "dbg_log.h"
#include "xa_item.h"
#include "container_impl.h"
#include "porting.h"
#include "simple_types.h"
#include "resizable_container.h"
#include "dbug.h"

using namespace AGG_HOOK_ENV;


agg_hooks::agg_hooks() : hook_object(h_res)
{
  (void)mysql_cmd2str;
}

agg_hooks::~agg_hooks()
{
}

int agg_hooks::get_col_val(xa_item *xai, int nCol, char *inbuf, size_t szIn,
  uint8_t *outVal, size_t *szOut, size_t *outOffs, int &colType)
{
  int cmd = xai->get_cmd();
  tContainer *pc = xai->m_cols.get_first() ; 
  auto pf = cmd==com_query?mysql_pickup_text_col_val:
    cmd==com_stmt_execute?mysql_pickup_bin_col_val:NULL;

  if (!pf) {
    log_print("pickup function is null, invalid cmd %d\n", cmd);
    return -1;
  }

  if (!pc) {
    log_print("fatal: no col def!\n");
    return -1;
  }

  int numCols = xai->get_col_count();

  if (nCol>=numCols) {
    log_print("invalid column no %d\n", nCol);
    return -1;
  }

  char *pCols = pc->tc_data();
  MYSQL_COLUMN *pfCol = (((MYSQL_COLUMN*)pCols) + nCol);

  if (pfCol->name[0]=='\0') {
    strncpy(pfCol->name,pfCol->alias,MAX_NAME_LEN);
    pfCol->name[MAX_NAME_LEN-1] = '\0';
  }

  /* fetch targeting column value from 'inbuf' */
  if (pf(pCols,numCols,inbuf,szIn,pfCol->name,outVal,szOut,outOffs)) {
    log_print("cant get col '%s' 's value\n",pfCol->name);
    return -1;
  }

  colType = pfCol->type ;

  return 0;
}

int 
agg_hooks::fill_new_txt_val(tContainer &finalRes, int valOffs, char newVal[])
{
  char *res = finalRes.tc_data();
  /* to the begining of 'lenenc-int + string' */
  char *tr = res+valOffs;
  /* length of the 'lenenc-int + string' */
  size_t szOrigVal = lenenc_str_size_get(tr) ;
  size_t szNewVal = get_str_lenenc_size(newVal);

  /* new value's size not changed  */
  if (szOrigVal==szNewVal) {
    lenenc_str_set(tr,newVal);
  }
  /* new value's bigger */
  else if (szOrigVal<szNewVal) {
    tContainer tmp ;
    size_t origLen = finalRes.tc_length(), ln=0;

    /* do backup */
    tmp.tc_copy(&finalRes);

    /* resize the res buffer */
    finalRes.tc_resize(origLen+szNewVal);
    res = finalRes.tc_data();
    tr  = res + valOffs ;

    /* copy contents */
    finalRes.tc_copy(&tmp);

    /* move contents after new val */
    ln = origLen-valOffs-szOrigVal ;
    memmove(tr+szNewVal,tr+szOrigVal,ln);

    /* set new value (plus lenenc-int part) */
    lenenc_str_set(res+valOffs,newVal) ;

    /* update final size */
    ln = origLen+(szNewVal-szOrigVal);
    finalRes.tc_update(ln);
    mysqls_update_req_size(res,ln-4);
  }
  /* new value's smaller */
  else {
    size_t origLen = finalRes.tc_length(), ln=0;

    /* move contents after new val */
    ln = origLen-valOffs-szOrigVal ;
    memmove(tr+szNewVal,tr+szOrigVal,ln);

    /* set new value (plus lenenc-int part) */
    lenenc_str_set(res+valOffs,newVal) ;

    /* update final size */
    ln = origLen-(szOrigVal-szNewVal);
    finalRes.tc_update(ln);
    mysqls_update_req_size(res,ln-4);
  }
  return 0;
}

int agg_hooks::do_accumulate(xa_item *xai, int nCol, 
  tContainer &finalRes /* one record in res set */
  )
{
  char *res = finalRes.tc_data();
  size_t sz = finalRes.tc_length();
  char *pTxBuff = xai->m_txBuff.tc_data() ;
  size_t szTxb  = xai->m_txBuff.tc_length();
  char outVal[64], out1[64];
  size_t szOut = 64, szOut1 = 64;
  size_t valOffs = 0;
  int colType = 0;
  int cmd = xai->get_cmd();
  char tmp[64] = "";

  /* fetch targeting column value from buffer'ed record */
  if (get_col_val(xai,nCol,pTxBuff,szTxb,(uint8_t*)out1,&szOut1,0,colType)) {
    return -1;
  }

  /* fetch targeting column value from res set */
  if (get_col_val(xai,nCol,res,sz,(uint8_t*)outVal,&szOut,&valOffs,colType)) {
    return -1;
  }

  /* do accumulation by column types and write it back to res */
  switch (colType) {
    case MYSQL_TYPE_TINY:  
      {
        uint8_t s = *(uint8_t*)out1 + *(uint8_t*)outVal ;

        if (cmd==com_query) sprintf(tmp,"%d",s);
        else *(uint8_t*)(res+valOffs) = s;
      }
      break;
    case MYSQL_TYPE_SHORT:  
      {
        uint16_t s= byte2_2_ul(out1) + byte2_2_ul(outVal);

        if (cmd==com_query) sprintf(tmp,"%d",s);
        else ul2store(s,(res+valOffs));
      }
      break ;
    case MYSQL_TYPE_LONG:
      {
        uint32_t s= byte4_2_ul(out1) + byte4_2_ul(outVal);

        if (cmd==com_query) sprintf(tmp,"%u",s);
        else ul4store(s,(res+valOffs));
      }
      break ;
    case MYSQL_TYPE_LONGLONG:
      {
        long long s= byte8_2_ul(out1) + byte8_2_ul(outVal);

        if (cmd==com_query) sprintf(tmp,"%lld",s);
        else ul8store_be(s,(res+valOffs));
      }
      break ;
    case MYSQL_TYPE_FLOAT:
      {
        float s= byte4_2_float(out1) + byte4_2_float(outVal);

        if (cmd==com_query) sprintf(tmp,"%f",s);
        else float4store(s,(res+valOffs));
      }
      break ;
    case MYSQL_TYPE_DOUBLE:
      {
        //double s= byte8_2_double(out1) + byte8_2_double(outVal);
        double s= byte8_2_double_be(out1) + byte8_2_double_be(outVal);

        if (cmd==com_query) sprintf(tmp,"%lf",s);
        else double8store(s,(res+valOffs));
      }
      break;
    default:
      log_print("unsupport-ed column type %d\n", colType);
      return -1;
  }

  if (cmd==com_query) {
    fill_new_txt_val(finalRes,valOffs,tmp);
  }

  return 0;
}

int agg_hooks::do_cmp(xa_item *xai, int nCol, 
  tContainer &finalRes, /* one record in res set */
  int agt /* aggregation type */
  )
{
  char *res = finalRes.tc_data();
  size_t sz = finalRes.tc_length();
  char *pTxBuff = xai->m_txBuff.tc_data() ;
  size_t szTxb  = xai->m_txBuff.tc_length();
  char outVal[64], out1[64];
  size_t szOut = 64, szOut1 = 64;
  size_t valOffs = 0;
  int colType = 0;
  int cmd = xai->get_cmd();
  char tmp[64] = "";
  auto __cmp = [&](auto op0, auto op1) {
    return agt==agt_max?(op0<op1):
      agt==agt_min?(op0>op1):
      false ;
  } ;

  /* fetch targeting column value from buffer'ed record */
  if (get_col_val(xai,nCol,pTxBuff,szTxb,(uint8_t*)out1,&szOut1,0,colType)) {
    return -1;
  }

  /* fetch targeting column value from res set */
  if (get_col_val(xai,nCol,res,sz,(uint8_t*)outVal,&szOut,&valOffs,colType)) {
    return -1;
  }

  /* do accumulation by column types and write it back to res */
  switch (colType) {
    case MYSQL_TYPE_TINY:  
      {
        uint8_t s = *(uint8_t*)outVal ;

        if (__cmp(*(uint8_t*)out1, s)) {

          if (cmd==com_query) sprintf(tmp,"%d",s);
          else *(uint8_t*)(res+valOffs) = s;
        }
      }
      break;
    case MYSQL_TYPE_SHORT:  
      {
        uint16_t s= byte2_2_ul(outVal);

        if (__cmp(byte2_2_ul(out1), s)) {

          if (cmd==com_query) sprintf(tmp,"%d",s);
          else ul2store(s,(res+valOffs));
        }
      }
      break ;
    case MYSQL_TYPE_LONG:
      {
        uint32_t s= byte4_2_ul(outVal);

        if (__cmp(byte4_2_ul(out1), s)) {

          if (cmd==com_query) sprintf(tmp,"%u",s);
          else ul4store(s,(res+valOffs));
        }
      }
      break ;
    case MYSQL_TYPE_LONGLONG:
      {
        uint64_t s=  byte8_2_ul(outVal);

        if (__cmp(byte8_2_ul(out1), s)) {

          if (cmd==com_query) sprintf(tmp,"%lu",s);
          else ul8store_be(s,(res+valOffs));
        }
      }
      break ;
    case MYSQL_TYPE_FLOAT:
      {
        float s= byte4_2_float(outVal);

        if (__cmp(byte4_2_float(out1), s)) {

          if (cmd==com_query) sprintf(tmp,"%f",s);
          else float4store(s,(res+valOffs));
        }
      }
      break ;
    case MYSQL_TYPE_DOUBLE:
      {
        //double s= byte8_2_double(outVal);
        double s= byte8_2_double_be(outVal);

        if (__cmp(byte8_2_double_be(out1), s)) {

          if (cmd==com_query) sprintf(tmp,"%f",s);
          else double8store(s,(res+valOffs));
        }
      }
      break;
    default:
      log_print("unsupport-ed column type %d\n", colType);
      return -1;
  }

  return 0;
}

int agg_hooks::do_aggregates(tContainer &finalRes, agg_params *params)
{
  safeClientStmtInfoList &stmts = params->stmts ;
  xa_item *xai = params->xai;
  int cfd = xai->get_client_fd();
  tSqlParseItem *sp = 0;

  /* get 'parse item' */
  if (stmts.get_curr_sp(cfd,sp)) {
    log_print("found no 'sp' item cid %d\n", cfd);
    return -1;
  }

  /* deal with each aggregation columns */
  for (auto pai=sp->m_aggs.next(true);pai;pai=sp->m_aggs.next()) {

    if (xai->m_txBuff.tc_length()==0) 
      continue ;

    /* aggregation type */
    switch (pai->type) {
      case agt_sum:
      case agt_count:
        do_accumulate(xai,pai->nCol,finalRes);
        break ;

      case agt_max:
      case agt_min:
        do_cmp(xai,pai->nCol,finalRes,pai->type);
        break ;

      default:
        log_print("unknown aggregation type %d\n", pai->type);
        return -1;
    } /* end switch */
  }

  return 0;
}

int agg_hooks::run_hook(char *res,size_t sz,void *p)
{
  agg_params *params = static_cast<agg_params*>(p);
  xa_item *xai = params->xai ;
  tContainer finalRes ;

  /* prpare the final res buffer */
  finalRes.tc_resize(sz);
  finalRes.tc_write(res,sz);

  /* process the aggregation columns */
  if (do_aggregates(finalRes,params)) {
    return -1;
  }

  mysqls_update_sn(finalRes.tc_data(),params->sn);

  //hex_dump(finalRes.tc_data(),finalRes.tc_length());

  /* save response packet into tx buffer */
  xai->m_txBuff.tc_copy(&finalRes);

  return 0;
}


namespace AGG_HOOK_ENV {

  bool is_agg_res(int cid, safeClientStmtInfoList &stmts) {

    tSqlParseItem *sp = 0;

    if (stmts.get_curr_sp(cid,sp) || !sp) {
      return false;
    }

    return sp->m_aggs.size()>0 ;
  } 

} ;

HMODULE_IMPL (
  agg_hooks,
  agg_items
);

