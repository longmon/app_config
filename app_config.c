/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_app_config.h"
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <errno.h>

/* If you declare any globals in php_app_config.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(app_config)
*/

/* True global resources - no need for thread safety here */
static int le_app_config;
static int sp[2] = {0};
static HashTable *config_array;

static zend_class_entry *app_config_class_entry_ptr;
static zend_class_entry app_config_class_entry;
/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("app_config.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_app_config_globals, app_config_globals)
    STD_PHP_INI_ENTRY("app_config.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_app_config_globals, app_config_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_app_config_compiled(string arg)
   Return a string to confirm that the module is compiled in */
ZEND_METHOD(app_config, __construct)
{
	APP_CONFIG_INIT_ARRAY(config_array, 1024);
	set_process_title("app_config_server");
}

ZEND_METHOD(app_config, get)
{
	char *key = NULL;
	char *namespace = NULL;
	int nlen;
	int len;
	if( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &key, &len, &namespace, &nlen ) == FAILURE ){
		return;
	}
	Req req;
	memset(&req, 0, sizeof( Req));
	memcpy(req.key, key, strlen(key));
	if( !namespace ){
		namespace = "default";
	}
	memcpy(req.namespace, namespace, strlen(namespace) );
	req.type = 1;
	char retval[1024] = {0};
	int ret = unix_socket_get( req, &retval, 1024 ); //todo: buffer may be  to small
	if( ret < 0 ){
		printf("unix_socket_get:[%d]%s\n", errno, strerror(errno));
		RETURN_NULL();
	}
	RETURN_STRING(retval, 1);
}

ZEND_METHOD(app_config, set)
{
	char *key = NULL;
	char *val = NULL;
	char *namespace = NULL;
	int klen,nlen,vlen;
	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|s", &key, &klen, &val,&vlen, &namespace, &nlen) == FAILURE ){
		return;
	}
	Req req;
	memset( &req, 0, sizeof(req));
	memcpy(req.key, key, klen);
	if( !namespace ){
		namespace = "default";
	}
	memcpy(req.namespace, namespace, strlen(namespace));
	req.type = 2;
	memcpy(req.data, val, vlen );
	if( unix_socket_send(req, val, vlen ) == SUCCESS ){
		RETURN_TRUE;
	}
	RETURN_FALSE;
}

ZEND_METHOD(app_config,load)
{
	zval *z_array = NULL;
	char *namespace = NULL;
	int len;
	if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|s", &z_array, &namespace, &len) == FAILURE ){
		return;
	}
	if( !namespace ){
		namespace = "default";
	}
	app_config_load_conf( z_array, namespace );
	RETURN_TRUE;
}

ZEND_METHOD(app_config, start)
{
	app_config_server();
}

ZEND_FUNCTION(app_config_version)
{
	RETURN_STRING("0.1",1);
}

/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_app_config_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_app_config_init_globals(zend_app_config_globals *app_config_globals)
{
	app_config_globals->global_value = 0;
	app_config_globals->global_string = NULL;
}
*/
/* }}} */
const zend_function_entry app_config_methods[] = {
	ZEND_FE( app_config_version, NULL )
	PHP_FE_END
};
const zend_function_entry app_config_class_methods[] = {
	ZEND_ME(app_config, __construct, 	NULL, 	ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)		
	ZEND_ME(app_config, get, 			NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(app_config, set, 			NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(app_config, load, 			NULL,	ZEND_ACC_PUBLIC)
	ZEND_ME(app_config, start, 			NULL,  	ZEND_ACC_PUBLIC)
	PHP_FE_END
};
/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(app_config)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	INIT_CLASS_ENTRY( app_config_class_entry, "app_config", app_config_class_methods);
	app_config_class_entry_ptr = zend_register_internal_class( &app_config_class_entry  TSRMLS_CC);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(app_config)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(app_config)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(app_config)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(app_config)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "app_config support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* {{{ app_config_module_entry
 */
zend_module_entry app_config_module_entry = {
	STANDARD_MODULE_HEADER,
	"app_config",
	app_config_methods,
	PHP_MINIT(app_config),
	PHP_MSHUTDOWN(app_config),
	PHP_RINIT(app_config),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(app_config),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(app_config),
	PHP_APP_CONFIG_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_APP_CONFIG
ZEND_GET_MODULE(app_config)
#endif

void app_config_server()
{
	daemonize();
	int unix_sock_id = unix_socket_listen(SOCK_PATHNAME);
	unix_socket_accept(unix_sock_id);
}

int make_socketpair_pipe()
{
	if( socketpair(AF_UNIX, SOCK_STREAM, 0, sp) < 0 ) {
		return -1;
	}
	return 0;
}


int unix_socket_listen( const char *pathname ) {
	int sockfd;
	int len;
	struct sockaddr_un un;
	memset( &un, 0, sizeof(un) );
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, pathname);
	unlink( pathname );

	len = offsetof( struct sockaddr_un, sun_path) + strlen(pathname);

	if( (sockfd = socket( AF_UNIX, SOCK_STREAM, 0)) < 0 ) {
		return -1;
	}

	if( bind( sockfd, (struct sockaddr_un *)&un, len) < 0 ){
		return -2;
	}
	if( listen(sockfd, 5) < 0 ){
		return -3;
	}
	return sockfd;
}

int unix_socket_accept( int fd ) {
	int epoll_fd;
	struct epoll_event ev;
	struct epoll_event *evs;
	int s;
	epoll_fd = epoll_create1(0);
	if( make_socket_nonblock(fd) < 0 ){
		return -1;
	}
	ev.data.fd = fd;
	ev.events = EPOLLIN|EPOLLET;
	s = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
	if( s < 0 ){
		perror("epoll_ctl");
		return -2;
	}
	evs = calloc(MAX_EVENTS, sizeof ev);

	int i,n, sock, e;
	int client;
	struct sockaddr_in in_addr;
	socklen_t in_len;
	in_len = sizeof(in_addr);
	char buffer[1156];
	while(1)
	{
		n = epoll_wait( epoll_fd, evs, MAX_EVENTS, 1);
		for( i = 0; i < n; i++ )
		{
			sock = evs[i].data.fd;
			e 	 = evs[i].events;
			if( e & (EPOLLERR | EPOLLHUP) ){
				close(sock);
				continue;
			}
			if( e & EPOLLIN )
			{
				if( sock == fd ){
					while(1){
						client = accept( sock, (struct sockaddr*)&in_addr, &in_len );
						if( client == -1 ){
							if( errno == EAGAIN || errno == EWOULDBLOCK ){
								break;
							}else{
								printf("accept error[%d]:%s\n", errno, strerror(errno));
								break;
							}
						}
						make_socket_nonblock(client);
						ev.data.fd = client;
						ev.events = EPOLLIN|EPOLLET;
						epoll_ctl( epoll_fd, EPOLL_CTL_ADD, client, &ev);
					}
				} else {
					int done = 0;
					int count = 0;
					while(1){
						count = 0;
						count = read( sock, (void*)&buffer, 1156);
						if( count == -1 ){
							if( errno != EAGAIN ){
								epoll_ctl( epoll_fd, EPOLL_CTL_DEL, sock, NULL);
								close(sock);
							}
							break;
						}
						if( count == 0 ){
							epoll_ctl( epoll_fd, EPOLL_CTL_DEL, sock, NULL);
							close(sock);
							break;
						}
						char key[65]={0};
						char namespace[64]={0};
						char udata[1024] = {0};
						int optype = 0;
						memcpy(key, buffer, 64);
						memcpy(namespace, buffer+64, 64);
						memcpy( &optype, buffer+128, 4);
						memcpy(udata, buffer+132, 1024);
						printf("op:%s\n", optype);
						if( strcmp(optype,"1") == 0 ){
							app_config_value_struct ret;
							memset(&ret, 0, sizeof(ret));
							int cfget;
							if( (cfget = config_get(key, namespace, &ret) ) < 0 ) {
								if( write(sock, "NULL", sizeof("NULL") ) < 0 ){
									break;
								}
							} else {
								if( write(sock, ret.data, ret.len) < 0 ){
									break;
								}
							}
						} else {
							if( config_set(key, namespace, udata ) == SUCCESS ){
								write( sock, "1", sizeof("1") );
							} else {
								write( sock, "0", sizeof("0") );
							}
							break;
						}
					}
				}
			}
		}
	}
	free(evs);
	close(epoll_fd);
	close(fd);
	return 0;
}

int config_get( const char *key, const char *namespace, app_config_value_struct *pDest )
{
	zval *array;
	if( FAILURE == zend_hash_find( config_array, namespace, strlen(namespace)+1, (void**)&array) ){
		return -1;
	}
	//hashtable_print(Z_ARRVAL_P(array));
	zval **retval = NULL;
	if( zend_hash_find( Z_ARRVAL_P(array), key, strlen(key)+1, (void **)&retval ) == SUCCESS ) {
		app_config_value_struct *data = app_config_packet(retval);
		memcpy(pDest, data, sizeof(app_config_value_struct) );
		efree(data);
		return 0;
	}
	
	return -2;
}

int config_set( const char *key, const char *namespace, char *data)
{
	zval *array;
	if( FAILURE == zend_hash_find( config_array, namespace, strlen(namespace)+1, (void**)&array) ){
		return -1;
	}
	if( zend_hash_update(Z_ARRVAL_P(array), key, strlen(key)+1, data, sizeof(*data), NULL ) == FAILURE ){
		return -2;
	}
	zend_hash_update(config_array, namespace, sizeof(namespace), (void *)&array, sizeof(array), NULL);
	return 0;
}

void daemonize()
{
	int pid,n;
	pid = fork();
	if( pid ){
		exit(0);
	}else if( pid < 0 ){
		printf("fork error\n");
		exit(1);
	}
	setsid();
	if( pid = fork() ){
		exit(0);
	}else if( pid < 0 ){
		printf("fork child proccess failed\n");
		exit(1);
	}
	for( n = 0; n < NOFILE; n++ ){
		//close(n);
	}
	chdir("/tmp");
	umask(0);
	return;
}

int make_socket_nonblock( int sockfd )
{
	int flag, s;
	flag = fcntl(sockfd, F_GETFL, 0);
	if( flag == -1 ){
		perror("fcntl");
		return -1;
	}
	flag = flag | O_NONBLOCK;
	s = fcntl(sockfd, F_SETFL, flag );
	if( s == -1 ){
		perror("fcntl");
		return -2;
	}
	return 0;
}

int unix_socket_connect()
{
	struct sockaddr_un un;
	memset( &un, 0, sizeof un);
	un.sun_family = AF_UNIX;
	strncpy(un.sun_path, SOCK_PATHNAME, strlen(SOCK_PATHNAME) );
	int client = socket(AF_UNIX, SOCK_STREAM, 0 );
	if( client < 0 ){
		return -1;
	}
	if( connect( client, (struct sockaddr*)&un, sizeof(un) )  == -1 ){
		return -2;
	}
	
	return client;
}

int unix_socket_get( Req req, void *buffer, int bufferLen)
{
	int client = unix_socket_connect();

	if( client < 0 ){
		return -5;
	}

	if( write(client, (void*)&req, sizeof(req)) == -1 ){
		printf("err:%d,%s\n", errno, strerror(errno));
		return -3;
	}
	if(  recv(client, buffer, bufferLen, 0 ) < 0 ){
		return -4;
	}
	return 0;
}

int unix_socket_send( Req req, char *buffer, int bufferLen )
{
	int client = unix_socket_connect();
	if( client < 0 ){
		return -5;
	}
	if( write(client, (void *)&req, sizeof(req) ) == -1 ) {
		return -3;
	}
	if( write(client, (void *)buffer, bufferLen ) == -1 ){
		return -4;
	}
	char ret;
	if( recv( client, (void *)&ret, 1, 0) == -1 ){
		return -5;
	}
	return 0;
}

int app_config_load_conf( zval *array, const char *namespace )
{
	if( !config_array ) 
	{
		return -1;
	}
	zval *origin;
	if( SUCCESS == zend_hash_find( config_array, namespace, sizeof(namespace), (void **)&origin ) )
	{
		zend_hash_merge(Z_ARRVAL_P(origin), Z_ARRVAL_P(array), NULL, NULL, sizeof(zval *), 1);
		zend_hash_update(config_array, namespace, sizeof(namespace), (void *)&origin, sizeof(origin), NULL);
	} else {
		zend_hash_add( config_array, namespace, sizeof(namespace), array, sizeof(array), NULL);
	}
	return 0;
}

app_config_value_struct * app_config_packet( zval **val )
{
	app_config_value_struct *valu = emalloc(sizeof(app_config_value_struct));
	memset(valu, 0, sizeof(app_config_value_struct));

	switch( Z_TYPE_PP(val) )
	{
		case IS_NULL:
			valu->type = IS_NULL;
			convert_to_string_ex(val);
			valu->len = Z_STRLEN_PP(val);
			valu->data = Z_STRVAL_PP(val);
			break;
		case IS_LONG:
			valu->type = IS_LONG;
			convert_to_string_ex(val);
			valu->len = Z_STRLEN_PP(val);
			valu->data = Z_STRVAL_PP(val);
			break;
		case IS_BOOL:
			valu->type = IS_BOOL;
			convert_to_string_ex(val);
			valu->len = Z_STRLEN_PP(val);
			valu->data = Z_STRVAL_PP(val);
			break;
		case IS_STRING:
			valu->type = IS_STRING;
			valu->len = Z_STRLEN_PP(val);
			valu->data = Z_STRVAL_PP(val);
			break;
		case IS_ARRAY:
			valu->type = IS_ARRAY;
			char *ret = app_config_array_to_string(val);
			valu->len = strlen(ret);
			valu->data = ret;
			break;
		case IS_OBJECT:
			return;
			break;
		case IS_RESOURCE:
			return;
			break;
		default:
			return;
	}
	return valu;
}

char* app_config_array_to_string( zval **val )
{
	int count,i;
	zval **item;
	char *key;
	int idx;
	char *retval = NULL;
	int curlen = 0;
	int retlen = 0;
	if( Z_TYPE_PP(val) != IS_ARRAY )
	{
		return ;
	}
	HashTable *ht = Z_ARRVAL_PP(val);
	count = zend_hash_num_elements( ht );
	zend_hash_internal_pointer_reset( ht );
	retval = app_config_realloc( retval, &curlen, 64);
	for ( i = 0; i < count; ++i)
	{
		char *tmpv = NULL;
		char tmpk[64] = {0};
		int  need = 0;
		zend_hash_get_current_data( ht, (void **)&item );
		if( Z_TYPE_PP(item) == IS_ARRAY ){
			tmpv = "Array";
		} else {
			convert_to_string_ex(item);
			tmpv = Z_STRVAL_PP(item);
		}
		if( zend_hash_get_current_key(ht, &key, &idx, NULL ) == HASH_KEY_IS_STRING ){
			memcpy( tmpk, key, strlen(key) );
		} else {
			itoa(i, tmpk, 10);
		}
		zend_hash_move_forward(ht);
		need = 6+strlen(tmpk)+strlen(tmpv);
		if( curlen <= (retlen+need) ){
			retval = app_config_realloc( retval, &curlen, need );
		}
		retlen += sprintf( retval+retlen, "\"%s\":\"%s\",", tmpk, tmpv );
	}
	char *retval1 = emalloc(strlen(retval)+1);
	memset(retval1, 0, strlen(retval)+1 );
	sprintf(retval1,"{%s", retval);
	retval1[strlen(retval)] = '}';
	efree(retval);
	return retval1;
}

/** 重新申请内存，并把原内容复制过去 */
char* app_config_realloc( char *buffer, int *currentLen, int requireLen ) {
	int newLen = ( *currentLen+requireLen) * 1.5 ;
	char *tmp = emalloc( newLen );
	memset(tmp, 0, newLen );
	if( *currentLen > 0 ) {
		memcpy(tmp, buffer, *currentLen );
		efree(buffer);
	}
	*currentLen = newLen;
	return tmp;
}

HashTable* check_key( const char *key, const char *delim )
{
  HashTable *keyTable = NULL;
  ALLOC_HASHTABLE(keyTable);
  zend_hash_init(keyTable, 8, NULL, NULL, 1);
  if( key == '\0' ){
    return keyTable;
  }
  char *tmp = NULL;
  char *token;
  tmp = strdup(key);
  for( token = strsep(&tmp, delim); token != NULL; token = strsep(&tmp, delim) ){ 
    printf("token:%s", token);
    if( strcmp(token, delim) != 0  ){
      zend_hash_next_index_insert( keyTable, token, strlen(token), NULL);
    }
  }
  printf("\n");
  return keyTable;
}

void hashtable_print( HashTable *ht )
{
  int count,i;
  zval **item;
  char *key;
  int idx;
  count = zend_hash_num_elements( ht );
  zend_hash_internal_pointer_reset( ht );
  printf("count:%d\n", count);

  for( i = 0; i < count; i++ ){
    zend_hash_get_current_data( ht, (void**)&item);
    if( Z_TYPE_PP(item) == IS_ARRAY ){
      return hashtable_print(Z_ARRVAL_PP(item));
    }
    convert_to_string_ex(item);
    if( zend_hash_get_current_key(ht, &key, &idx, NULL ) == HASH_KEY_IS_STRING ){
      printf("%s => %s\n", key, Z_STRVAL_PP(item) );
    } else {
      printf("%d => %s\n", i, Z_STRVAL_PP(item));
    }
    zend_hash_move_forward(ht);
  }
  zend_hash_internal_pointer_reset( ht );
  return;
}

void sub_process_handler()
{
	unix_socket_listen("/tmp/app_config_server.sock");
}

void set_process_title( const char *title ) {
	zval *retval = NULL;
	zval *func = NULL;
	zval *argv[1];
	MAKE_STD_ZVAL(retval);
	MAKE_STD_ZVAL(func);
	ZVAL_STRING(func, "cli_set_process_title", 1);
	MAKE_STD_ZVAL(argv[0]);
	ZVAL_STRING(argv[0], title, 1 );
	if( call_user_function(EG(function_table), NULL, func, retval, 1, argv TSRMLS_CC) == FAILURE ){
		return;
	}
	zval_ptr_dtor(&argv[0]);
	zval_ptr_dtor(&func);
	zval_ptr_dtor(&retval);
}


zval * app_config_json_encode( zval val )
{
	printf("app_config_json_encode\n");
	zval *retval = NULL;
	zval *func = NULL;
	zval *argv[1];
	MAKE_STD_ZVAL(retval);
	MAKE_STD_ZVAL(func);
	ZVAL_STRING(func, "json_encode", 1);
	MAKE_STD_ZVAL(argv[0]);
	argv[0] = &val;
	if( call_user_function(EG(function_table), NULL, func, retval, 1, argv TSRMLS_CC) == FAILURE ) {
		printf("call_user_function %s\n", Z_STRVAL_P(func));
		return;
	}
	printf("675 retval:%s\n", Z_STRVAL_P(retval) );
	zval_ptr_dtor(&func);
	return retval;
}