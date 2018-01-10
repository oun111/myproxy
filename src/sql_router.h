
#ifndef __SQL_ROUTER_H__
#define __SQL_ROUTER_H__

#include "container_impl.h"
#include <set>

class sql_router {

protected:
  /* the datanode usages */
  safeDataNodeList &m_nodes ;

  /* routing rule handlers */
  using ruleHandler = int (sql_router::*)(SHARDING_VALUE*,std::set<uint8_t>&);
  using ruleHandler_t = struct tRoutingRuleHandler {
    sql_router::ruleHandler ha ;
  } ;
  ruleHandler_t *m_handlers;

protected:
  int register_rule_handlers(void);
  void unregister_rule_handlers(void);

  /* rule handlers */
  int ha_rule_rangeMap(SHARDING_VALUE*,std::set<uint8_t>&);
  int ha_rule_modN(SHARDING_VALUE*,std::set<uint8_t>&);
  int ha_rule_dummy(SHARDING_VALUE*,std::set<uint8_t>&);

  int calc_intersection(std::set<uint8_t>&,std::set<uint8_t>&);

  /* calculates routing by doing mod(N) */
  int get_route_by_modN(int,sv_t*,int&,int);

  /* get route to related tables in statement */
  int get_related_table_route(tSqlParseItem*,std::set<uint8_t>&);

  /* get route by sharding column values */
  int get_sharding_route(tSqlParseItem*,std::set<uint8_t>&);

public:
  sql_router(safeDataNodeList&) ;
  ~sql_router(void) ;

public:
  /* calculate sql routing infomations */
  int get_route(int, tSqlParseItem*,std::set<uint8_t>&);
  /* get route to configured data nodes */
  int get_full_route_by_conf(tSqlParseItem*,std::set<uint8_t>&);
  /* get route to all data nodes */
  int get_full_route(std::set<uint8_t>&);
} ;


#endif /* __SQL_ROUTER_H__ */
