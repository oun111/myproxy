
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <vector>
#include <stack>
#include <string>
#include "json_parser.h"
#include "dbug.h"



SimpleJsonParser::SimpleJsonParser(void)
{
}

SimpleJsonParser::~SimpleJsonParser(void)
{
  reset();
}

int SimpleJsonParser::reset(void)
{
  jsonKV_t *p = m_root ;
  std::stack<int> stk ;
  std::stack<jsonKV_t*> stk1 ;
  int np=-1;
  int dir=0 ; /* traverse direction: 0 down, 1 up */

  while (p||np>=0) {
    /* store the tree node */
    if (!dir) {
      stk1.push(p);
    }
    /* traverse to next node */
    np = np<((int)p->list.size()-1)?(np+1):-1;
    if (np<0) {
      /* drop the children list */
      {
        for (uint16_t i=0;i<p->list.size();i++) 
          delete p->list[i];
      }
      /* 
       * get parent 
       */
      dir = 1, p = 0;
      if (!stk1.empty()) 
        stk1.pop();
      if (!stk1.empty()) 
        p = stk1.top();
      /* 
       * get child position pointer 
       */
      np = -1;
      if (!stk.empty()) {
        np = stk.top();
        stk.pop();
      }
    } else {
      stk.push(np);
      /* get to child */
      p  = p->list[np] ;
      np = -1;
      dir= 0;
    }
  } /* end while */
  delete m_root ;
  return 0;
}

SimpleJsonParser::jsonKV_t* SimpleJsonParser::new_node(char *key)
{
  jsonKV_t *node = new jsonKV_t ;

  node->key = key;
  return node ;
}

SimpleJsonParser::jsonKV_t* SimpleJsonParser::find(jsonKV_t *parent, char *key)
{
  jsonKV_t *p = parent ;
  std::stack<int> stk ;
  std::stack<jsonKV_t*> stk1 ;
  int np=-1;
  int dir=0 ; /* traverse direction: 0 down, 1 up */
  int level = 0;

  while (p||np>=0) {
    /* find key in tree node */
    if (!dir) {
      stk1.push(p);
      if (p->key==key)
        return p ;
    }
    /* traverse to next node */
    np = np<((int)p->list.size()-1)?(np+1):-1;
    if (np<0) {
      level --;
      /* 
       * get parent 
       */
      dir = 1, p = 0;
      if (!stk1.empty()) 
        stk1.pop();
      if (!stk1.empty()) 
        p = stk1.top();
      /* 
       * get child position pointer 
       */
      np = -1;
      if (!stk.empty()) {
        np = stk.top();
        stk.pop();
      }
    } else {
      level ++;
      stk.push(np);
      /* get to child */
      p  = p->list[np] ;
      np = -1;
      dir= 0;
    }
  } /* end while */
  return NULL;
}

void SimpleJsonParser::dump(jsonKV_t *node)
{
  jsonKV_t *p = node ;
  std::stack<int> stk ;
  std::stack<jsonKV_t*> stk1 ;
  int np=-1;
  int dir=0 ; /* traverse direction: 0 down, 1 up */
  int level = 0;

  while (p||np>=0) {
    /* print the tree node */
    if (!dir) {
      stk1.push(p);
      {
        printd("%d ",level);
        for (int i=0;i<level;i++)
          printd("%s","-");
      }
      printd("%s", p->key.c_str());
      if (p->type==keyList) 
        printd("(%zu)\n",p->list.size());
      else if (p->type==keyValue) 
        printd(" => %s\n", p->value.c_str());
      else printd("\n");
    }
    /* traverse to next node */
    np = np<((int)p->list.size()-1)?(np+1):-1;
    if (np<0) {
      level --;
      /* 
       * get parent 
       */
      dir = 1, p = 0;
      if (!stk1.empty()) 
        stk1.pop();
      if (!stk1.empty()) 
        p = stk1.top();
      /* 
       * get child position pointer 
       */
      np = -1;
      if (!stk.empty()) {
        np = stk.top();
        stk.pop();
      }
    } else {
      level ++;
      stk.push(np);
      /* get to child */
      p  = p->list[np] ;
      np = -1;
      dir= 0;
    }
  } /* end while */
}

int SimpleJsonParser::eliminate_comments(std::string &s)
{
  int level = 0;
  uint16_t p = 0, sp=0;

  for (;p<(s.size()-1);p++) {
    if (s[p]=='/' && s[p+1]=='*' && !level++) {
      sp = p ;
      p += 2;
    } 
    if (s[p]=='*' && s[p+1]=='/' && !--level) {
      p+=2;
      s.erase(sp,p-sp);
      p = sp ;
    }
  }
  return 0;
}

int SimpleJsonParser::parse(std::string &s)
{
  int pos=0 ;
  std::string tkn ;
  jsonKV_t *p = 0, *tmp=0;
  std::stack<jsonKV_t*> stk ;
  bool bEval = false, bKey = false, 
    /* it's comment */
    bComm = false;

  eliminate_comments(s);
  /* tree root */
  m_root = new_node((char*)"root");
  p = m_root ;
  stk.push(p);
  /* traverse and create the analyzing tree */
  while ((pos=lex_read(s,tkn,pos))>=0 && 
     pos<(int)s.size()) {
    if (tkn=="{" || tkn=="[") {
      p = stk.top();
      p->type = keyList ;
      bEval = false ;
    } else if (tkn=="}") {
      stk.pop();
    } else if (tkn==":") {
      bEval = true ;
      bKey  = false ;
    } else if (tkn=="," || tkn=="]") {
      if (bKey) { 
        p = stk.top();
        p->type=keyOnly;
        /* pop out the key-only onde */
        stk.pop();
        bKey = false; 
        /* end of array? pop it out! */
        if (tkn=="]")
          stk.pop();
      }
    } else if (tkn==commKW) {
      bComm = true ;
    } else {
      if (!bComm) {
        /* it's the value */
        if (bEval) {
          p = stk.top();
          stk.pop();
          p->value = tkn;
          p->type  = keyValue ;
        } else {
          /* it's a key */
          tmp = new_node((char*)tkn.c_str());
          p = stk.top();
          p->list.push_back(tmp);
          stk.push(tmp);
          bKey = true;
        }
      }
      bEval = false ;
      bComm = false ;
    } /* end else */
  }
#if 0
  /* XXX: test */
  {
    dump(m_root);
    printd("*************\n");
    dump(find(m_root,(char*)"DataNodes"));
    printd("*************\n");
    dump(find(m_root,(char*)"Schemas"));
  }
#endif
  return 0;
}

int SimpleJsonParser::lex_read(std::string &s, 
  std::string &tkn, uint16_t pos)
{
  uint16_t p = pos, p0=0;

  /* 
   * eliminate spaces 
   */
  for (;p<s.size()&&isspace(s[p]);p++) ;
  if (p>=s.size())
    return p;
  /* 
   * get a normal token 
   */
  if (s[p]=='\"') {
    for (++p,p0=p;p<s.size()&&s[p]!='\"';p++);
    if (p>=s.size())
      return p;
    tkn = s.substr(p0,p-p0);
    return p+1 ;
  }
  /* get a symbol */
  if (s[p]=='{' || s[p]=='}' || s[p]=='[' ||
     s[p]==']' || s[p]==':' || s[p]==',') {
    tkn = s[p] ;
    return p+1 ;
  }
  return -1;
}

