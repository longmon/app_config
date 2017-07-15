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

#ifndef PHP_APP_CONFIG_H
#define PHP_APP_CONFIG_H

extern zend_module_entry app_config_module_entry;
#define phpext_app_config_ptr &app_config_module_entry

#define PHP_APP_CONFIG_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_APP_CONFIG_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_APP_CONFIG_API __attribute__ ((visibility("default")))
#else
#	define PHP_APP_CONFIG_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define MAX_EVENTS 128

#define SOCK_PATHNAME "/tmp/app_config_server.sock"

#define APP_CONFIG_INIT_ARRAY(x, y) ALLOC_HASHTABLE(x);zend_hash_init(x, y, NULL, NULL, 1)


typedef struct {
  char key[256];
  char namespace[256];
} Req;

PHP_FUNCTION(app_config_init);

PHP_FUNCTION(app_config_load);

PHP_FUNCTION(app_config_start);

PHP_FUNCTION(app_config_get);

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(app_config)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(app_config)
*/

/* In every utility function you add that needs to use variables 
   in php_app_config_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as APP_CONFIG_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define APP_CONFIG_G(v) TSRMG(app_config_globals_id, zend_app_config_globals *, v)
#else
#define APP_CONFIG_G(v) (app_config_globals.v)
#endif

void app_config_server();

int app_config_load_conf( zval *z_array, const char *namespace );

int make_socketpair_pipe();

void sub_process_handler();

void set_process_title( const char *title );

int unix_socket_accept( int fd );

int unix_socket_listen( const char *pathname );

int make_socket_nonblock( int sockfd );

void daemonize();

int unix_socket_get(Req req, zval *retval);

int config_get( const char *key, const char *namespace, void **pDest );

HashTable* check_key( const char *key, const char *delim );

void hashtable_print( HashTable *ht );

#endif	/* PHP_APP_CONFIG_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
