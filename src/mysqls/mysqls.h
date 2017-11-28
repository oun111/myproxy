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

/* generates a 'client connects in' notice */
//extern int mysqls_gen_conn_in_notice(char*);

/* generates the 'sync priv' request */
//int mysqls_gen_sync_priv(char*,char*,size_t);

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
extern int mysqls_gen_error1(char*,char*, 
  int,uint32_t,char*);
extern int do_err_response(uint32_t, char*,int,int , ...);
extern int do_ok_response(uint32_t,int,char[/*MAX_PAYLOAD*/]);
extern uint8_t mysqls_is_error(char*,size_t);

/* generates dummy response for query command */
extern int mysqls_gen_dummy_qry_resp(char*,uint16_t, 
  int, uint32_t);

/* generates response to query for '@@version_commet' */
extern int mysqls_gen_ver_cmt_qry_resp(char*,uint16_t,
  int,uint32_t);

extern int mysqls_gen_simple_qry_resp(char*, uint16_t, 
  int ,uint32_t, char*, char**, size_t);

/* generates dummy response for query field command */
extern int mysqls_gen_dummy_qry_field_resp(char*,uint16_t, 
  int, uint32_t);

extern int mysqls_gen_desc_tbl_resp(char*,uint16_t, 
  int,uint32_t,char**,size_t*[],size_t);

/* get serial number of a packet */
extern int mysqls_extract_sn(char*);

/* get body position */
extern char* mysqls_get_body(char*);

extern int mysqls_sprintf(char*,int,va_list) ;

extern size_t mysqls_calc_col_def(void*, size_t);

extern size_t mysqls_calc_common_ok_res(void);

extern int mysqls_gen_common_query_resp_hdr(char*, 
  size_t, int*, void*, int);

extern int mysqls_gen_normal_proto_hdr(char*,size_t,int);

extern int mysqls_gen_res_end(char*,uint32_t,uint16_t);

extern int mysqls_extract_column_count(char*,size_t) ;

extern int mysqls_extract_column_def(char*,size_t, 
  void*, size_t,char*) ;
extern char* mysqls_save_one_col_def(void *pc, char *ptr, char *def_db);

extern int mysqls_extract_prepared_info(char *inb, 
  size_t sz, int *stmtid, int *nCol, int *nPh) ;

extern int mysqls_get_col_full_name(void*,int,char**,
  char**,char**,uint8_t*);

extern int mysqls_extract_next_rec(char*, int, 
  char**, int*);

extern bool mysqls_is_digit_type(uint8_t);

extern void mysqls_update_sn(char*,uint8_t);

extern uint8_t mysqls_get_cmd(char *inb);

extern void mysqls_set_cmd(char *inb, uint8_t cmd);

extern int mysqls_get_command(char*,size_t);

extern size_t mysqls_calc_stmt_prep_res_size(void*,size_t, 
  void*, size_t);

extern size_t mysqls_gen_stmt_prep_res(char*,int,
  void*,size_t,void*,size_t,int,int);

extern int mysqls_get_stmt_prep_stmt_id(char*,size_t,int*);

extern int mysqls_get_param_count(char*,size_t,uint16_t*);

extern int mysqls_update_stmt_prep_stmt_id(char*,size_t,int);

extern int mysqls_update_stmt_newbound_flag(char*,size_t,uint8_t);

extern size_t mysqls_calc_err_res(char*);

extern size_t mysqls_get_body_size(char*);

extern size_t mysqls_get_req_size(char*);

extern void mysqls_update_req_size(char*,size_t);

#ifdef __cplusplus
}
#endif

#endif /*  __MYSQLS_H__ */

