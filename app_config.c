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
ZEND_FUNCTION(app_config_init)
{
	set_process_title("app_config_server");
	//make_socketpair_pipe();
	app_config_server();
}

PHP_FUNCTION(app_config_get)
{
	char *data = "app_config_get";
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

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(app_config)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
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

/* {{{ app_config_functions[]
 *
 * Every user visible function must have an entry in app_config_functions[].
 */
const zend_function_entry app_config_functions[] = {
	PHP_FE(app_config_init,	NULL)		/* For testing, remove later. */
	PHP_FE(app_config_get,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in app_config_functions[] */
};
/* }}} */

/* {{{ app_config_module_entry
 */
zend_module_entry app_config_module_entry = {
	STANDARD_MODULE_HEADER,
	"app_config",
	app_config_functions,
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
	printf("server starting ...\n");
	int unix_sock_id = unix_socket_listen("/tmp/app_config_server.sock");
	printf("unix_sock_id:%d\n", unix_sock_id);
	unix_socket_accept(unix_sock_id);
}

int make_socketpair_pipe()
{
	if( socketpair(AF_UNIX, SOCK_STREAM, 0, sp) < 0 ) {
		return -1;
	}
	return 0;
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
	printf("accepting...\n");

	int i,n, sock, e;
	int client;
	struct sockaddr_in in_addr;
	socklen_t in_len;
	in_len = sizeof(in_addr);
	char buffer[256];
	memset(buffer, 0, sizeof(buffer));
	while(1)
	{
		n = epoll_wait( epoll_fd, evs, MAX_EVENTS, 1);
		for( i = 0; i < n; i++ )
		{
			sock = evs[i].data.fd;
			e 	 = evs[i].events;
			printf("sock:%d\n", sock);
			if( e & (EPOLLERR | EPOLLHUP) ){
				printf("error:%d\n", e);
				//epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);
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
						count = recv( sock, buffer, 256, 0);
						if( count == -1 ){
							if( errno != EAGAIN ){
								close(sock);
								//epoll_ctl( epoll_fd, EPOLL_CTL_DEL, sock, NULL);
							}
							break;
						}
						if( count == 0 ){
							close(sock);
							//epoll_ctl( epoll_fd, EPOLL_CTL_DEL, sock, NULL);
							break;
						}
						printf("data:%s\n", buffer);
						char *ret = "123345";
						write(sock, ret, strlen(ret));
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