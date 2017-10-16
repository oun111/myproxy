
#ifndef __CWPR_H__
#define __CWPR_H__

#include "zas.h"
#include "connstr.h"
#include "dbug.h"

extern "C" {

extern int rlogon(char *dsn);

extern int prepare_stmt(char *stmt);

//====================================================
//
// place-holders insertion methods
//
//====================================================

extern int insert_str(const char *arg);

extern int insert_const_str(unsigned char *arg);

extern int insert_big_int(long long arg);

extern int insert_long(long arg);

extern int insert_int(int arg);

extern int insert_unsigned(unsigned arg);

extern int insert_short(short arg);

extern int insert_unsigned_short(unsigned short arg);

extern int insert_double(double arg);

extern int insert_long_double(long double arg);

extern int insert_float(float arg);

extern int insert_zas_datetime(zas_datetime &arg);

//====================================================
//
// result set fetching methods
//
//====================================================

extern int fetch_str(char *val);

extern int fetch_big_int(long long &val);

extern int fetch_long(long &val);

extern int fetch_int(int &val);

extern int fetch_unsigned(unsigned &val);

extern int fetch_short(short &val);

extern int fetch_unsigned_short(unsigned short &val);

extern int fetch_double(double &val);

extern int fetch_long_double(long double &val);

extern int fetch_float(float &val);

extern int fetch_zas_datetime(zas_datetime &val);

extern int is_eof(void);

}  

#endif /* __CWPR_H__*/

