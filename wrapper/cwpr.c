
#include <memory>
#include "cwpr.h"
#include "../src/DAL/dal.h"

#define __mod_ver__ "0.0.1 alpha "

zas_connect g_conn((char*)"",dal_mysql) ;
//std::shared_ptr<zas_connect> g_conn ;
zas_stream g_stream(g_conn);
//std::shared_ptr<zas_stream> g_stream;

extern "C" {

int rlogon(char *dsn)
{
  if (!dsn) {
    printd("dsn/tns should not be NULL\n");
    return -1;
  }

#if 0
  g_conn = std::make_shared<zas_connect>();
  g_stream = std::make_shared<zas_stream>();
#endif

  try {
    if (!g_conn.rlogon(dsn))
      return -1;

  } catch (zas_exception &e) {
    printd("exception: %s\n"
      "err msg: %s\n"
      "code: %d\n", e.stm_text, e.msg, e.code);
    return -1;
  }
  printd("login ok\n");
  return 0;
}

int prepare_stmt(char *stmt)
{
  try {
    g_stream.open(0,stmt);

  } catch (zas_exception &e) {
    printd("exception: %s\n"
      "err msg: %s\n"
      "code: %d\n", e.stm_text, e.msg, e.code);
    return -1;
  }
  return 0;
}

static auto __do_insert = [&] (auto arg) {
  try {
    g_stream << arg;

  } catch (zas_exception &e) {
    printd("exception: %s\n"
      "err msg: %s\n"
      "code: %d\n", e.stm_text, e.msg, e.code);
    return -1;
  }
  return 0;
};

static auto __do_fetch = [&] (auto &val)  {
  try {
    g_stream >> val ;

  } catch (zas_exception &e) {
    printd("exception: %s\n"
      "err msg: %s\n"
      "code: %d\n", e.stm_text, e.msg, e.code);
    return -1;
  }
  return 0;
};


//====================================================
//
// place-holders insertion methods
//
//====================================================

int insert_str(const char *arg)
{
  return __do_insert(arg);
}

int insert_const_str(unsigned char *arg)
{
  return __do_insert(arg);
}

int insert_big_int(long long arg)
{
  return __do_insert(arg);
}

int insert_long(long arg)
{
  return __do_insert(arg);
}

int insert_int(int arg)
{
  return __do_insert(arg);
}

int insert_unsigned(unsigned arg)
{
  return __do_insert(arg);
}

int insert_short(short arg)
{
  return __do_insert(arg);
}

int insert_unsigned_short(unsigned short arg)
{
  return __do_insert(arg);
}

int insert_double(double arg)
{
  return __do_insert(arg);
}

int insert_long_double(long double arg)
{
  return __do_insert(arg);
}

int insert_float(float arg)
{
  return __do_insert(arg);
}

int insert_zas_datetime(zas_datetime &arg)
{
  return __do_insert(arg);
}

//====================================================
//
// result set fetching methods
//
//====================================================

int fetch_str(char *val)
{
  return __do_fetch(val);
}

int fetch_big_int(long long &val)
{
  return __do_fetch(val);
}

int fetch_long(long &val)
{
  return __do_fetch(val);
}

int fetch_int(int &val)
{
  return __do_fetch(val);
}

int fetch_unsigned(unsigned &val)
{
  return __do_fetch(val);
}

int fetch_short(short &val)
{
  return __do_fetch(val);
}

int fetch_unsigned_short(unsigned short &val)
{
  return __do_fetch(val);
}

int fetch_double(double &val)
{
  return __do_fetch(val);
}

int fetch_long_double(long double &val)
{
  return __do_fetch(val);
}

int fetch_float(float &val)
{
  return __do_fetch(val);
}

int fetch_zas_datetime(zas_datetime &val)
{
  return __do_fetch(val);
}

int is_eof(void)
{
  return !!g_stream.eof();
}

}  
