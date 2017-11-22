
#ifndef __ATTRIB_LIST_H__
#define __ATTRIB_LIST_H__

#include <stdio.h>
#include <stdint.h>
#include "connstr.h"


class attribute_list {

public:
  /* the argument attribute */
  using a_attr = struct tArgAttr {
    /* argument type */
    uint16_t type;
    /* argument size */
    uint32_t size ;
    /* 
     * yzhou added 2015.7.9: for shadowing placeholder 
     */
    struct tShadowPD {
      /* the argument name string */
      char name[TKNLEN];
      /* point to the original argument index */
      int pi ;
      /* private data */
      void *priv;
      /* size of private data */
      int sz;
    } sd ;
  } ;

public:
  /* the argument types */
  enum atypes {
    tInt, tLong, tBigint, tChar, tDouble, tLdouble, tDatetime, tNone
  } ;
#define type_str(t) \
    t==attribute_list::tInt?"int":\
    t==attribute_list::tLong?"long":\
    t==attribute_list::tBigint?"big int":\
    t==attribute_list::tChar?"char":\
    t==attribute_list::tDouble?"double":\
    t==attribute_list::tLdouble?"long double":\
    t==attribute_list::tDatetime?"datetime":\
    "none"
public:
  /* the attribute list of arguments */
  std::vector<a_attr*> attr_list ;
  /* attribute list before adaption */
  std::vector<a_attr*> backup_attr_list ;
protected:
  //std::string *sql_ptr ;
  /* total argument count */
  int num_arg ;
  /* total shadowing placeholder count */
  int num_sph ;
  /* TODO: the argument index mappings */
  std::vector<int> i_map ;

protected:
#if 0
  /* calculate predefined char argument length in sql */
  int char_size(int, uint16_t, uint16_t&);
  /* get argument attributes */
  int get(int,string::size_type&);
#endif
  /* save private data into attribute list, such
   *   as argument data */
  int save_priv(int,void*,int);

public:
  attribute_list(char*s=NULL) ;
  ~attribute_list(void) ;
  /* release the attribute list */
  void reset(void);
  void reset_counters(void);
  /* get argument count */
  int count(void);
  /* save argument value */
  int argv_save(void*,int,int);
  /* get private data */
  void* get_priv(int);
  /* get argument's type */
  uint16_t get_type(int);
  /* get argument's position by
   *  its inputing order */
  uint16_t get_pos2(int);
  /* get argument's name */
  char* get_name(int);
  /* get argument's type by 
   *  arguments' inputing order*/
  uint16_t get_type2(int);
  /* get argument's size by 
   *  arguments' inputing order*/
  uint32_t get_size2(int);
  /* get shadowing argument's 'parent index' */
  int get_pi(int);
  /* setup shadowing placeholder structure */
  int sph_setup(a_attr*, char*);
  /* get shadowing placeholder count */
  int sph_count(void);
  /* get place holder attribtes */
  int add_attr(char*,char*,char*);
  /* create place-holder order mappings */
  int build_order_map(void);
  /* re-create place-holder order mappings */
  int rebuild_order_map(void);
  /* object evaluations */
  //attribute_list& operator = (attribute_list &lst);
} ;


#endif /* __ATTRIB_LIST_H__*/
