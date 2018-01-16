/*
 *  Module Name:   mysqls
 *
 *  Description:   implements a tiny mysql server libary
 *
 *  Author:        yzhou
 *
 *  Last modified: Oct 11, 2016
 *  Created:       Sep 21, 2016
 *
 *  History:       refer to 'HISTROY.TXT'
 *
 */ 
#ifndef __MYSQLS_H__
#define __MYSQLS_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>
#include "mysqld_error.h"

#ifdef __VER_STRING__
#undef __VER_STRING__
#endif
/* module version string */
#define __VER_STRING__  "v0.00.2"


#ifdef __cplusplus
extern "C" {
#endif


/* total auth plugin data len */
#define AP_LENGTH  21

/* max length of a password */
#define MAX_PWD_LEN  512

typedef struct mysqls_global_var_t
{
  uint8_t char_set ;
  uint16_t server_status ;
} MYSQLS_VAR;

/* check packet by size */
extern bool mysqls_is_packet_valid(char*,size_t);

/* check if it's eof packet */
extern bool mysqls_is_eof(char*,size_t);

/* generate initial greeting packet */
extern int mysqls_gen_greeting(uint32_t,
  int,uint32_t,char*,char*,char*,size_t);

/* generates random string */
extern void mysqls_gen_rand_string(char*,size_t sz);

/* test if input packet is login request */
extern bool mysqls_is_login_req(char*,size_t);

/* parse login request */
extern int mysqls_parse_login_req(char*,size_t, 
  char*,char*,size_t*,char*) ;

/* authenticates user passwords */
extern int mysqls_auth_usr(char*,char*,
  char*,size_t,char*,size_t);

/* generates an Ok response packet */
extern int mysqls_gen_ok(char*,uint16_t,uint32_t,uint64_t,uint64_t);

/* generates an Error response packet */
extern int mysqls_gen_error(char*,uint16_t, 
  int,uint32_t,va_list);
extern int do_err_response(uint32_t, char*,int,int , ...);
extern int do_ok_response(uint32_t,int,char[/*MAX_PAYLOAD*/]);
extern uint8_t mysqls_is_error(char*,size_t);

/* generates response for query field command */
extern int mysqls_gen_qry_field_resp(char *inb, uint16_t sql_stat, 
  int char_set,uint32_t sn, char *db, char *tbl, char **rows, size_t numRows);

/* get serial number of a packet */
extern int mysqls_extract_sn(char*);

/* get body position */
extern char* mysqls_get_body(char*);

extern int mysqls_sprintf(char*,int,va_list) ;

extern int mysqls_extract_column_count(char*,size_t) ;

extern char* mysqls_save_one_col_def(void *pc, char *ptr, char *def_db);

extern int mysqls_extract_prepared_info(char *inb, 
  size_t sz, int *stmtid, int *nCol, int *nPh) ;

extern int mysqls_get_col_full_name(void*,int,char**,
  char**,char**,uint8_t*);

extern bool mysqls_is_digit_type(uint8_t);

extern void mysqls_update_sn(char*,uint8_t);

extern uint8_t mysqls_get_cmd(char *inb);

extern void mysqls_set_cmd(char *inb, uint8_t cmd);

extern int mysqls_get_stmt_prep_stmt_id(char*,size_t,int*);

extern int mysqls_update_stmt_prep_stmt_id(char*,size_t,int);

extern size_t mysqls_get_body_size(char*);

extern size_t mysqls_get_req_size(char*);

extern void mysqls_update_req_size(char*,size_t);

extern int mysqls_gen_normal_resp(char *inb, uint16_t sql_stat, 
  int char_set, uint32_t sn, char *db, char *tbl, char **cols, 
  size_t num_cols, char **rows, size_t num_rows);

#ifdef __cplusplus
}
#endif

#endif /*  __MYSQLS_H__ */

