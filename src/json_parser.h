
#ifndef __JSON_PARSER_H__
#define __JSON_PARSER_H__

#include <vector>

class SimpleJsonParser {
protected:
  /* object types */
  enum eObjectType {
    keyValue,
    keyList,
    keyOnly
  } ;

  /* key/value pair */
  using jsonKV_t = struct tJsonParserKeyVal {
    std::string key;
    /* 0: key-value, 1: key-object, 2: key only */
    uint8_t type ;
    /* optional object/value */
    std::vector<struct tJsonParserKeyVal*> list ;
    std::string value;
  } ;

  /* the 'comment' keyword */
  const char *commKW = "__comment";

protected:
  /* the json output tree */
  jsonKV_t *m_root ;

public:
  SimpleJsonParser(void);
  ~SimpleJsonParser(void);

public:
  /* create the json parsing tree */
  int parse(std::string&);

protected:

  /* lexically read a token */
  int lex_read(std::string&,std::string&,uint16_t);

  /* create new json tree node */
  jsonKV_t* new_node(char*) ;

  int eliminate_comments(std::string&);

  int reset(void);

  void dump(jsonKV_t*);

  jsonKV_t* find(jsonKV_t*,char*);
} ;

#endif /* __JSON_PARSER_H__*/

