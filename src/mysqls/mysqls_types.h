
#ifndef __MYSQLS_TYPES_H__
#define __MYSQLS_TYPES_H__

/* 
 * copy from $(mysql source)/include/mysql_com.h 
 */
enum enum_field_types { MYSQL_TYPE_DECIMAL, MYSQL_TYPE_TINY,
			MYSQL_TYPE_SHORT,  MYSQL_TYPE_LONG,
			MYSQL_TYPE_FLOAT,  MYSQL_TYPE_DOUBLE,
			MYSQL_TYPE_NULL,   MYSQL_TYPE_TIMESTAMP,
			MYSQL_TYPE_LONGLONG,MYSQL_TYPE_INT24,
			MYSQL_TYPE_DATE,   MYSQL_TYPE_TIME,
			MYSQL_TYPE_DATETIME, MYSQL_TYPE_YEAR,
			MYSQL_TYPE_NEWDATE, MYSQL_TYPE_VARCHAR,
			MYSQL_TYPE_BIT,
                        /*
                          mysql-5.6 compatibility temporal types.
                          They're only used internally for reading RBR
                          mysql-5.6 binary log events and mysql-5.6 frm files.
                          They're never sent to the client.
                        */
                        MYSQL_TYPE_TIMESTAMP2,
                        MYSQL_TYPE_DATETIME2,
                        MYSQL_TYPE_TIME2,
                        
                        MYSQL_TYPE_NEWDECIMAL=246,
			MYSQL_TYPE_ENUM=247,
			MYSQL_TYPE_SET=248,
			MYSQL_TYPE_TINY_BLOB=249,
			MYSQL_TYPE_MEDIUM_BLOB=250,
			MYSQL_TYPE_LONG_BLOB=251,
			MYSQL_TYPE_BLOB=252,
			MYSQL_TYPE_VAR_STRING=253,
			MYSQL_TYPE_STRING=254,
			MYSQL_TYPE_GEOMETRY=255

};

/* max length of a user name */
#define MAX_NAME_LEN 128*3

  /*
   * the column's definition 
 */
typedef struct mysql_column_t {
  char schema[MAX_NAME_LEN];
  /* the owner table of this column */
  char tbl[MAX_NAME_LEN];
  /* alias of owner table */
  char tbl_alias[MAX_NAME_LEN];
  /* column name */
  char name[MAX_NAME_LEN];
  /* alias of column */
  char alias[MAX_NAME_LEN];
  /* character set of this column */
  uint16_t charset ;
  /* maximum length of this column */
  uint32_t len;
  /* column type */
  uint8_t type ;
  /* column flags */
  uint16_t flags ;
  /* FIXME: the max shown digit, what's this? */
  uint8_t decimal;
}__attribute__((packed)) MYSQL_COLUMN, MYSQL_FIELD;

#endif /* __MYSQLS_TYPES_H__*/
