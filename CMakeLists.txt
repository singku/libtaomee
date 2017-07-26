cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_INSTALL_PREFIX "/usr/")
find_package(glib REQUIRED)
find_package(ssl REQUIRED)
set(CMAKE_USE_RELATIVE_PATHS  true )	

set (SRC_LIST 
	./tm_shm/tm_shm.c
	./project/stat_agent/msglog.c
	./timer.c
	./crypt/qdes.c
	./conf_parser/config.c
	./inet/mcast.c
	./inet/tcp.c
	./inet/inet_utils.c
	./interprocess/shmq.c
	./interprocess/lock.c
	./log.c
	./time/time.c
	./tm_dirty/tm_dirty.c
    ./tm_dirty/utf8_punc.c
    ./tm_dirty/acsm.c
    ./tlog/fileutils.c
    ./tlog/netutils.c
    ./tlog/tlog.c    
    ./attr_api/attr_api.c
    ./attr_api/hash_table.c
    ./attr_api/shm.c
    ./attr_api/api_util.c

	./lua/lapi.c
	./lua/lauxlib.c
	./lua/lbaselib.c
	./lua/lcode.c
	./lua/ldblib.c
	./lua/ldebug.c
	./lua/ldo.c
	./lua/ldump.c
	./lua/lfunc.c
	./lua/lgc.c
	./lua/linit.c
	./lua/liolib.c
	./lua/llex.c
	./lua/lmathlib.c
	./lua/lmem.c
	./lua/loadlib.c
	./lua/lobject.c
	./lua/lopcodes.c
	./lua/loslib.c
	./lua/lparser.c
	./lua/lstate.c
	./lua/lstring.c
	./lua/lstrlib.c
	./lua/ltable.c
	./lua/ltablib.c
	./lua/ltm.c
	./lua/lundump.c
	./lua/lvm.c
	./lua/lzio.c
	)
set (HEADER_LIST 
	./tm_shm/tm_shm.h
	./timer.h
	./bitmanip/bitmanip.h
	./hash_algo.h
	./project/stat_agent/msglog.h
	./project/utilities.h
	./project/constants.h
	./project/types.h
	./utils.h
	./random/random.h
	./crypt/qdes.h
	./conf_parser/config.h
	./inet/tcp.h
	./inet/mcast.h
	./inet/ipaddr_cat.h
	./inet/inet_utils.h
	./interprocess/shmq.h
	./interprocess/lock.h
	./interprocess/filelock.h
	./dataformatter/bin_str.h
	./list.h
    ./log_old.h
	./log.h
	./time/time.h
	./atomic.h
	./tm_dirty/tm_dirty.h
    ./tm_dirty/utf8_punc.h
    ./tm_dirty/acsm.h
    ./tlog/fileutils.h
    ./tlog/netutils.h
    ./tlog/tlog_decl.h
    ./tlog/tlog.h
    ./tlog/tlog_macros.h
    ./attr_api/attr_api.h

	./lua/lauxlib.h
	./lua/luaconf.h
	./lua/lua.h
	./lua/lualib.h
   	)
SET (LUA_HEADER_LIST
)

INCLUDE_DIRECTORIES( "./" "../" "./lua/" ${GLIB_INCLUDE_DIR} )
ADD_LIBRARY(taomee ${SRC_LIST}	)
#target_link_libraries (taomee "lua" )
ADD_DEFINITIONS( "-g -DDEBUG -DUSE_TLOG -Wall  -MMD  -funroll-loops -D__USE_STRING_INLINES -fhosted  -fPIC  -DLUA_USE_LINUX -D_GNU_SOURCE" )
INSTALL (TARGETS taomee DESTINATION lib)
FOREACH (HEADER_FILE ${HEADER_LIST})
 	STRING( REGEX REPLACE "\\.(.*)/.*\\.h$" "\\1" DIR_NAME ${HEADER_FILE} ) 
	INSTALL (FILES ${HEADER_FILE} DESTINATION  include/libtaomee/${DIR_NAME}	)	
ENDFOREACH(HEADER_FILE)

FOREACH (LUA_HEADER_FILE ${LUA_HEADER_LIST})
	message( ${LUA_HEADER_FILE})
	INSTALL (FILES ${LUA_HEADER_FILE} DESTINATION  include/	)	
ENDFOREACH(LUA_HEADER_FILE)
	

