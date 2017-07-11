dnl $Id$
dnl config.m4 for extension app_config

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(app_config, for app_config support,
dnl Make sure that the comment is aligned:
dnl [  --with-app_config             Include app_config support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(app_config, whether to enable app_config support,
dnl Make sure that the comment is aligned:
[  --enable-app_config           Enable app_config support])

if test "$PHP_APP_CONFIG" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-app_config -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/app_config.h"  # you most likely want to change this
  dnl if test -r $PHP_APP_CONFIG/$SEARCH_FOR; then # path given as parameter
  dnl   APP_CONFIG_DIR=$PHP_APP_CONFIG
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for app_config files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       APP_CONFIG_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$APP_CONFIG_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the app_config distribution])
  dnl fi

  dnl # --with-app_config -> add include path
  dnl PHP_ADD_INCLUDE($APP_CONFIG_DIR/include)

  dnl # --with-app_config -> check for lib and symbol presence
  dnl LIBNAME=app_config # you may want to change this
  dnl LIBSYMBOL=app_config # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $APP_CONFIG_DIR/$PHP_LIBDIR, APP_CONFIG_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_APP_CONFIGLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong app_config lib version or lib not found])
  dnl ],[
  dnl   -L$APP_CONFIG_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(APP_CONFIG_SHARED_LIBADD)

  PHP_NEW_EXTENSION(app_config, app_config.c, $ext_shared)
fi
