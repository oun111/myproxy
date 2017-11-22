
#include <stdint.h>
#include <vector>
#include <string>
#include "connstr.h"
#include "attrib_list.h"
#include "dbug.h"
#include "zas.h"



/* 
 * class attribute_list
 */

attribute_list::attribute_list(char *s):
num_arg(0)
{
}

attribute_list::~attribute_list(void)
{
  reset();
}

void attribute_list::reset_counters(void)
{
  num_arg = num_sph = 0;
}

void attribute_list::reset(void)
{
  uint16_t i = 0;
  a_attr *pa ;

  i_map.clear();
  /* clean up attribute list */
  for (i=0;i<attr_list.size();i++) {
    if (!(pa=attr_list[i])) {
      continue ;
    }
    if (pa->sd.priv) {
      free(pa->sd.priv);
    }
    free(pa) ;
  }
  attr_list.clear();
  /* release the backup attr list */
  for (i=0;i<backup_attr_list.size();i++) {
    if (!(pa=backup_attr_list[i])) {
      continue ;
    }
    if (pa->sd.priv) {
      free(pa->sd.priv);
    }
    free(pa) ;
  }
  backup_attr_list.clear();
}

int attribute_list::argv_save(void *val, int sz, int argp)
{
  uint16_t i=0; 

  if (argp<(int)i_map.size() && 
      (i=i_map[argp])<count()) {
    save_priv(i,val,sz);
    //printd("argp: %d, i: %d, count: %d\n", argp, i, count());
    return 1;
  }
  return 0 ;
}

/* save argument's private data */
int attribute_list::save_priv(int idx, void *data, int sz)
{
  uint32_t size = 0, m_sz = 0 ;

  if (idx>=(int)attr_list.size()) {
    return 0;
  }
  /* 64bytes is large enough for an integer or
   *   floating point variable */
  size = (attr_list[idx]->type!=tChar)?
   64:(sz>=LONG_DATABUF_LEN)?LONG_DATABUF_LEN:sz;
  /* the effectual data size */
  m_sz = sz>=LONG_DATABUF_LEN?LONG_DATABUF_LEN:sz ;
  attr_list[idx]->sd.priv = realloc(attr_list[idx]
    ->sd.priv,size) ;
  memset(attr_list[idx]->sd.priv,0,size);
  memcpy(attr_list[idx]->sd.priv,data,m_sz);
  attr_list[idx]->sd.sz = m_sz ;
  return 1;
}

/* get argument's private data */
void* attribute_list::get_priv(int idx)
{
  if (idx>=(int)attr_list.size()) {
    return 0;
  }
  return (char*)attr_list[idx]->sd.priv;
}

/* get argument's type */
int attribute_list::get_pi(int idx)
{
  return idx<(int)attr_list.size()?
    attr_list[idx]->sd.pi:0;
}

/* get argument's type */
uint16_t attribute_list::get_type(int idx)
{
  return idx<(int)attr_list.size()?
    attr_list[idx]->type:0;
}

uint16_t attribute_list::get_pos2(int idx)
{
  return idx<(int)i_map.size()? 
    i_map[idx]:0 ;
}

char* attribute_list::get_name(int idx)
{
  return idx<(int)attr_list.size()? 
    attr_list[idx]->sd.name:0 ;
}

/* get argument's type by argument inputing order */
uint16_t attribute_list::get_type2(int idx)
{
  /* get attribute list item by 'i_map' order */
  return idx<(int)i_map.size() && 
    i_map[idx]<(int)attr_list.size()?
    attr_list[i_map[idx]]->type:0 ;
}

/* get argument's size by argument inputing order */
uint32_t attribute_list::get_size2(int idx)
{
  return idx<(int)i_map.size() && 
    i_map[idx]<(int)attr_list.size()?
    attr_list[i_map[idx]]->size:0 ;
}

/* setup shadowing placeholder relationship */
int attribute_list::sph_setup(a_attr *pa, char *name)
{
  int i=0; 

  if (!pa) {
    return 0;
  }
  pa->sd.pi  = -1;
  strncpy(pa->sd.name,name,TKNLEN);
  for (;i<num_arg; i++) {
    /* setup relationship with 2 place holders */
    if (!strcmp(name,attr_list[i]->sd.name) &&
      attr_list[i]->sd.pi<0) {
      pa->sd.pi = i ;
      num_sph++ ;
    }
  }
  return 1;
}

int attribute_list::add_attr(char *szName, 
  char *szType, char *szLen)
{
  int type, size ;
  a_attr *pa = 0;

  if (!strcasecmp(szType,"int") ||
     !strncasecmp(szType,"unsigned",8)) {
    /* including 'int', 'unsigned int' */
    size = 4 ;
    type = tInt ;
  } else if (!strcasecmp(szType,"long")) {
    size = 4 ;
    type = tLong ;
  } else if (!strcasecmp(szType,"bigint")) {
    size = 8 ;
    type = tBigint ;
  } else if (!strcasecmp(szType,"ldouble")) {
    size = 16 ;
    type = tLdouble ;
  } else if (!strcasecmp(szType,"double")) {
    size = 8 ;
    type = tDouble ;
  } else if (!strcasecmp(szType,"char")) {
    size = atoi(szLen);
    type = tChar ;
  } else if (!strcasecmp(szType,"datetime")) {
    size = sizeof(zas_datetime) ;
    type = tDatetime ;
  } else {
    printd("unrecognize type '%s'\n", szType);
    return 0;
  }
  /* allocate a new one */
  if (num_arg<(int)attr_list.size()) {
    pa = attr_list[num_arg];
  } else {
    pa = (a_attr*)malloc(sizeof(a_attr)) ;
    attr_list.push_back(pa);
    pa->sd.priv=NULL;
  }
  pa->size = size;
  pa->type = type;
  /* shadowing placeholder: setup structure */
  sph_setup(pa,szName);
  /* increse arg counter */
  num_arg++ ;
  return 1;
}

/* create order mapping of place holders */
int attribute_list::build_order_map(void)
{
  int i = 0;
  a_attr *pa = 0;

  /* save the original argument orders */
  for (i_map.clear();i<num_arg;i++) {
    if (attr_list[i]->sd.pi>=0) {
      continue ;
    }
    /* save attribute list */
    pa = (a_attr*)malloc(sizeof(a_attr));
    memcpy(pa,attr_list[i],sizeof(a_attr));
    backup_attr_list.push_back(pa);
    /* save argument orders */
    i_map.push_back(i);
  }
  return 1;
}

/* re-create the order mapping of 
 *  place holders */
int attribute_list::rebuild_order_map(void)
{
  uint16_t i=0,n=0,ln=0; 
  char scmp[TKNLEN], *pname ;

  /* update mappings */
  for (i=0; i<backup_attr_list.size();i++) {
    for (n=0;n<num_arg;n++) {
      pname = attr_list[n]->sd.name;
      /* mysql keyword escape symbol found */
      if (pname[0]=='`') {
        ln = strlen(pname)-2;
        strncpy(scmp,pname+1,ln);
        scmp[ln] = '\0';
      } else {
        /* normal symbol */
        strcpy(scmp,pname);
      }
      if (!strcasecmp(backup_attr_list[i]->sd.name,scmp) &&
         attr_list[n]->sd.pi<0) {
        i_map[i] = n ;
        break ;
      }
    }
    /* for those item(s) match nothing in attr_list,
     *  set flag to avoid value insertion on related
     *  arguments */
    if (n>=num_arg) {
      i_map[i] = num_arg ;
    }
  } /* end for */
  return 1;
}

int attribute_list::count(void)
{
  return num_arg;
}

int attribute_list::sph_count(void)
{
  return num_sph;
}

#if 0
attribute_list& attribute_list::operator = (attribute_list &lst)
{
  a_attr *pa = 0 ;

  reset();

  /* copy the 'attr_list' */
  for (size_t i=0;i<lst.attr_list.size();i++) {
    pa = (a_attr*)malloc(sizeof(*pa));
    memcpy(pa,lst.attr_list[i],sizeof(*pa));
    pa->sd.priv = malloc(lst.attr_list[i]->sd.sz);
    attr_list.push_back(pa);
  }

  /* copy the 'backup_attr_list' */
  for (size_t i=0;i<lst.backup_attr_list.size();i++) {
    pa = (a_attr*)malloc(sizeof(*pa));
    memcpy(pa,lst.backup_attr_list[i],sizeof(*pa));
    pa->sd.priv = malloc(lst.backup_attr_list[i]->sd.sz);
    backup_attr_list.push_back(pa);
  }

  /* copy the 'num_arg' & 'num_sph' */
  num_arg = lst.num_arg;
  num_sph = lst.num_sph;

  /* copy 'i_map' */
  i_map = lst.i_map ;

  return *this;
}
#endif

