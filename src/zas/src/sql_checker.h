
#ifndef __SQL_CHECKER_H__
#define __SQL_CHECKER_H__


#if USE_STL_REGEX==1
  #include <regex>
#else
  #include <regex.h>
#endif

#include "sql_tree.h"

class sql_checker : protected sql_tree {

protected:
  const char* const_ptn_ep = "^[+-]?[-a-zA-Z\\_\\*\\@]+\\w*$|"
    "^[\\`][a-zA-Z\\_\\*\\@]+\\w*[\\`]$";
  const char* const_ptn_cs = "^[\\'][^']*[\\']$|^[\"][^\"]*[\"]$";
  const char* const_ptn_cd = "^[+-]?[0-9]+[\\.]?[0-9]+$|"
    "^[+-]?[0-9]+$|^[+-]?0[Xx][0-9a-fA-F]+$";

  using __regex_t__ =   
#if USE_STL_REGEX==1
    std::regex
#else
    regex_t
#endif
  ;

  __regex_t__ r_ep;
  __regex_t__ r_cs;
  __regex_t__ r_cd;

protected:
  int do_check(__regex_t__ &r, char *item);

public:
  sql_checker(void);
  ~sql_checker(void);

public:
  int do_checking(stxNode*);

private:
  inline int check_stmt_list(stxNode*);
  inline int check_endpoint(stxNode*);
  inline int check_expr(stxNode*);
} ;


#endif /* __SQL_CHECKER_H__*/

