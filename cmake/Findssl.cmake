FIND_PATH( SSL_INCLUDE_DIR openssl/md5.h 
  /usr/include/
	DOC "The directory where md5.h resides")
MESSAGE(STATUS "Looking for ssl - found:${SSL_INCLUDE_DIR}")
#/usr/include/ssl-2.0/


FIND_LIBRARY( SSL_LIBRARY
	NAMES ssl 
	PATHS
	/usr/lib/
	DOC "The SSL library")

IF (SSL_INCLUDE_DIR )
	SET( SSL_FOUND 1 CACHE STRING "Set to 1 if Foo is found, 0 otherwise")
ELSE (SSL_INCLUDE_DIR )
	SET( SSL_FOUND 0 CACHE STRING "Set to 1 if Foo is found, 0 otherwise")
ENDIF (SSL_INCLUDE_DIR )

MARK_AS_ADVANCED( SSL_FOUND )

IF(SSL_FOUND)
	MESSAGE(STATUS "找到了 ssl 库")
ELSE(SSL_FOUND)
	MESSAGE(FATAL_ERROR "没有找到 ssl :请安装：sudo apt-get install  libssl-dev ,然后重新 安装 ")
ENDIF(SSL_FOUND)

