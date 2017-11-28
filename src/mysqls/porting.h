
#ifndef __PORTING_H__
#define __PORTING_H__


#if 0
/* 
 * copy from $(mysql source)/include/mysql.h 
 */
enum mysql_option 
{
  MYSQL_OPT_CONNECT_TIMEOUT, MYSQL_OPT_COMPRESS, MYSQL_OPT_NAMED_PIPE,
  MYSQL_INIT_COMMAND, MYSQL_READ_DEFAULT_FILE, MYSQL_READ_DEFAULT_GROUP,
  MYSQL_SET_CHARSET_DIR, MYSQL_SET_CHARSET_NAME, MYSQL_OPT_LOCAL_INFILE,
  MYSQL_OPT_PROTOCOL, MYSQL_SHARED_MEMORY_BASE_NAME, MYSQL_OPT_READ_TIMEOUT,
  MYSQL_OPT_WRITE_TIMEOUT, MYSQL_OPT_USE_RESULT,
  MYSQL_OPT_USE_REMOTE_CONNECTION, MYSQL_OPT_USE_EMBEDDED_CONNECTION,
  MYSQL_OPT_GUESS_CONNECTION, MYSQL_SET_CLIENT_IP, MYSQL_SECURE_AUTH,
  MYSQL_REPORT_DATA_TRUNCATION, MYSQL_OPT_RECONNECT,
  MYSQL_OPT_SSL_VERIFY_SERVER_CERT, MYSQL_PLUGIN_DIR, MYSQL_DEFAULT_AUTH,
  MYSQL_OPT_BIND,
  MYSQL_OPT_SSL_KEY, MYSQL_OPT_SSL_CERT, 
  MYSQL_OPT_SSL_CA, MYSQL_OPT_SSL_CAPATH, MYSQL_OPT_SSL_CIPHER,
  MYSQL_OPT_SSL_CRL, MYSQL_OPT_SSL_CRLPATH,
  MYSQL_OPT_CONNECT_ATTR_RESET, MYSQL_OPT_CONNECT_ATTR_ADD,
  MYSQL_OPT_CONNECT_ATTR_DELETE,
  MYSQL_SERVER_PUBLIC_KEY,
  MYSQL_ENABLE_CLEARTEXT_PLUGIN,
  MYSQL_OPT_CAN_HANDLE_EXPIRED_PASSWORDS,

  /* MariaDB options */
  MYSQL_PROGRESS_CALLBACK=5999,
  MYSQL_OPT_NONBLOCK,
  MYSQL_OPT_USE_THREAD_SPECIFIC_MEMORY,
};
#endif

/* 
 * copy from $(mysql source)/include/mysql-com.h 
 */
enum clt_caps {
  client_long_password = 1, /* new more secure passwords */
  client_found_rows = 2,    /* found instead of affected rows */
  client_long_flag = 4,  /* get all column flags */
  client_connect_with_db = 8, /* one can specify db on connect */
  client_no_schema = 16,  /* don't allow database.table.column */
  client_compress = 32,  /* can use compression protocol */
  client_odbc = 64,   /* odbc client */
  client_local_files = 128, /* can use load data local */
  client_ignore_space = 256, /* ignore spaces before '(' */
  client_protocol_41 = 512,  /* new 4.1 protocol */
  client_interactive = 1024,  /* this is an interactive client */
  client_ssl = 2048,  /* switch to ssl after handshake */
  client_ignore_sigpipe = 4096,  /* ignore sigpipes */
  client_transactions = 8192,  /* client knows about transactions */
  client_reserved = 16384,  /* old flag for 4.1 protocol  */
  client_secure_connection = 32768,  /* new 4.1 authentication */
  client_multi_statements = (1ul << 16), /* enable/disable multi-stmt support */
  client_multi_results = (1ul << 17), /* enable/disable multi-results */
  client_ps_multi_results = (1ul << 18), /* multi-results in ps-protocol */
  client_plugin_auth = (1ul << 19), /* client supports plugin authentication */
  client_connect_attrs = (1ul << 20), /* client supports connection attributes */
  /* enable authentication response packet to be larger than 255 bytes. */
  client_plugin_auth_lenenc_client_data = (1ul << 21),
  /* don't close the connection for a connection with expired password. */
  client_can_handle_expired_passwords = (1ul << 22),
  client_progress = (1ul << 29),   /* client support progress indicator */
  client_ssl_verify_server_cert = (1ul << 30),
  client_remember_options = (1ul << 31),
} ;

/* 
 * copy from $(mysql source)/include/mysql-com.h 
 */
#define CLIENT_BASIC_FLAGS  (\
  client_long_password | \
  client_found_rows | \
  client_long_flag | \
  client_connect_with_db | \
  client_no_schema | \
  /*client_compress |*/ \
  client_odbc | \
  client_local_files | \
  client_ignore_space | \
  client_protocol_41 | \
  client_interactive | \
  /*client_ssl |*/ \
  client_ignore_sigpipe | \
  client_transactions | \
  client_reserved | \
  client_secure_connection | \
  client_multi_statements | \
  client_multi_results | \
  client_ps_multi_results | \
  /*client_ssl_verify_server_cert |*/ \
  client_remember_options | \
  client_progress | \
  client_plugin_auth | \
  client_plugin_auth_lenenc_client_data | \
  client_connect_attrs \
)


/* 
 * porting from $(mysql source)/include/mysql_com.h 
 */

/**
  Is raised when a multi-statement transaction
  has been started, either explicitly, by means
  of BEGIN or COMMIT AND CHAIN, or
  implicitly, by the first transactional
  statement, when autocommit=off.
*/
#define SERVER_STATUS_IN_TRANS     1
#define SERVER_STATUS_AUTOCOMMIT   2	/* Server in auto_commit mode */
#define SERVER_MORE_RESULTS_EXISTS 8    /* Multi query - next query exists */
#define SERVER_QUERY_NO_GOOD_INDEX_USED 16
#define SERVER_QUERY_NO_INDEX_USED      32
/**
  The server was able to fulfill the clients request and opened a
  read-only non-scrollable cursor for a query. This flag comes
  in reply to COM_STMT_EXECUTE and COM_STMT_FETCH commands.
*/
#define SERVER_STATUS_CURSOR_EXISTS 64
/**
  This flag is sent when a read-only cursor is exhausted, in reply to
  COM_STMT_FETCH command.
*/
#define SERVER_STATUS_LAST_ROW_SENT 128
#define SERVER_STATUS_DB_DROPPED        256 /* A database was dropped */
#define SERVER_STATUS_NO_BACKSLASH_ESCAPES 512
/**
  Sent to the client if after a prepared statement reprepare
  we discovered that the new statement returns a different 
  number of result set columns.
*/
#define SERVER_STATUS_METADATA_CHANGED 1024
#define SERVER_QUERY_WAS_SLOW          2048

/**
  To mark ResultSet containing output parameter values.
*/
#define SERVER_PS_OUT_PARAMS            4096

/**
  Set at the same time as SERVER_STATUS_IN_TRANS if the started
  multi-statement transaction is a read-only transaction. Cleared
  when the transaction commits or aborts. Since this flag is sent
  to clients in OK and EOF packets, the flag indicates the
  transaction status at the end of command execution.
*/
#define SERVER_STATUS_IN_TRANS_READONLY 8192


/**
  Server status flags that must be cleared when starting
  execution of a new SQL statement.
  Flags from this set are only added to the
  current server status by the execution engine, but 
  never removed -- the execution engine expects them 
  to disappear automagically by the next command.
*/
#define SERVER_STATUS_CLEAR_SET (SERVER_QUERY_NO_GOOD_INDEX_USED| \
                                 SERVER_QUERY_NO_INDEX_USED|\
                                 SERVER_MORE_RESULTS_EXISTS|\
                                 SERVER_STATUS_METADATA_CHANGED |\
                                 SERVER_QUERY_WAS_SLOW |\
                                 SERVER_STATUS_DB_DROPPED |\
                                 SERVER_STATUS_CURSOR_EXISTS|\
                                 SERVER_STATUS_LAST_ROW_SENT)

/* 
 * copy from $(mysql source)/include/mysql_com.h 
 */
enum enum_server_command
{
  com_sleep, com_quit, com_init_db, com_query, com_field_list,
  com_create_db, com_drop_db, com_refresh, com_shutdown, com_statistics,
  com_process_info, com_connect, com_process_kill, com_debug, com_ping,
  com_time, com_delayed_insert, com_change_user, com_binlog_dump,
  com_table_dump, com_connect_out, com_register_slave,
  com_stmt_prepare, com_stmt_execute, com_stmt_send_long_data, com_stmt_close,
  com_stmt_reset, com_set_option, com_stmt_fetch, com_daemon,
  /* don't forget to update const char *command_name[] in sql_parse.cc */

  /* must be last */
  com_end
};

static const char* mysql_cmd2str[] =
{
  "com_sleep", "com_quit", "com_init_db", "com_query", "com_field_list",
  "com_create_db", "com_drop_db", "com_refresh", "com_shutdown", "com_statistics",
  "com_process_info", "com_connect", "com_process_kill", "com_debug", "com_ping",
  "com_time", "com_delayed_insert", "com_change_user", "com_binlog_dump",
  "com_table_dump", "com_connect_out", "com_register_slave",
  "com_stmt_prepare", "com_stmt_execute", "com_stmt_send_long_data", "com_stmt_close",
  "com_stmt_reset", "com_set_option", "com_stmt_fetch", "com_daemon",
  "com_end"
} ;


/* 
 * copy from $(mysql source)/include/mysql_com.h 
 */
#define NOT_NULL_FLAG	1		/* Field can't be NULL */
#define PRI_KEY_FLAG	2		/* Field is part of a primary key */
#define UNIQUE_KEY_FLAG 4		/* Field is part of a unique key */
#define MULTIPLE_KEY_FLAG 8		/* Field is part of a key */
#define BLOB_FLAG	16		/* Field is a blob */
#define UNSIGNED_FLAG	32		/* Field is unsigned */
#define ZEROFILL_FLAG	64		/* Field is zerofill */
#define BINARY_FLAG	128		/* Field is binary   */

/* The following are only sent to new clients */
#define ENUM_FLAG	256		/* field is an enum */
#define AUTO_INCREMENT_FLAG 512		/* field is a autoincrement field */
#define TIMESTAMP_FLAG	1024		/* Field is a timestamp */
#define SET_FLAG	2048		/* field is a set */
#define NO_DEFAULT_VALUE_FLAG 4096	/* Field doesn't have default value */
#define ON_UPDATE_NOW_FLAG 8192         /* Field is set to NOW on UPDATE */
#define NUM_FLAG	32768		/* Field is num (for clients) */
#define PART_KEY_FLAG	16384		/* Intern; Part of some key */
#define GROUP_FLAG	32768		/* Intern: Group field */
#define UNIQUE_FLAG	65536		/* Intern: Used by sql_yacc */
#define BINCMP_FLAG	131072		/* Intern: Used by sql_yacc */
#define GET_FIXED_FIELDS_FLAG (1 << 18) /* Used to get fields in item tree */
#define FIELD_IN_PART_FUNC_FLAG (1 << 19)/* Field part of partition func */

#endif /*  __PORTING_H__ */

