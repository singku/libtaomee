/**
 *============================================================
 *  @file      config.h
 *  @brief     用于解析配置文件。配置文件的格式为：\n
 *    ============================================\n
 *             \#key        value       |  #号打头的行被视为注释 \n
 *             log_dir      ./log \n
 *             log_size     32000000 \n
 *    ============================================
 * 
 *  compiler   gcc4.1.2
 *  platform   Linux
 *
 *  copyright:  TaoMee, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */

#ifndef LIBTAOMEE_CONFIG_H_
#define LIBTAOMEE_CONFIG_H_

#include  <libtaomee/lua/lua.h>
#include  <libtaomee/lua/lauxlib.h>
#include  <libtaomee/lua/lualib.h>

/**
  * @brief 解析配置文件file_name。成功返回后就可以调用config_get_inval或者config_get_strval
  *         来获取配置文件的内容了。
  * @param file_name 配置文件路径名。
  * @see config_get_intval, config_get_strval
  * @return 成功则返回0，失败则返回-1。
  */
int   config_init(const char* file_name);

/**
  * @brief 根据配置文件file_name更新已有的配置。成功返回后就可以调用config_get_inval或者config_get_strval
  *         来获取配置文件的内容了。
  * @param file_name 配置文件路径名。
  * @see config_get_intval, config_get_strval
  * @return 成功则返回0，失败则返回-1。
  */
int   config_update(const char* file_name);

/**
  * @brief  清除解析配置文件时分配的内存。调用该函数后则再也不能使用config_get_inval或者config_get_strval
  *         来获取配置文件的内容了，同时config_get_strval之前返回的所有指针都不能继续使用。
  * @see config_get_intval, config_get_strval
  * @return void
  */
void  config_exit();

// return -1 on error
int   mmap_config_file(const char* file_name, char** buf);
// return number of words splitted
int   str_split(const char* ifs, char* line, char* field[], int n);

/**
  * @brief 获取配置项key的整型值。如果找不到key，则返回值为def。如果key对应的值是非数字字符，则行为未定义。
  *        故请小心校验配置文件和代码。
  * @param key 配置项key。
  * @param def 如果配置文件中没有配置项key，则返回def。
  * @see config_get_strval
  * @return 配置项key对应的整型值，或者def。
  */
int   config_get_intval(const char* key, int def);
/**
  * @brief 获取配置项key的字符创型的值。
  * @param key 配置项key。
  * @see config_get_intval
  * @return 配置项key对应的字符串型的型值。这个指针在调用config_exit之前一直都是有效可用的。
  */
char* config_get_strval(const char* key);

/** 
 * @brief update a config item based on key and val
 * @param key which item you want to update
 * @param val coressponded new val
 * @return -1 key or val is NULL || no key exist || or update failed
 * @return 0  on success
 */
int config_update_value(const char* key, const char* val);

/**
 * @brief add a new item to configuration
 * @param key which item you want to add
 * @param val corresponded val with key
 * @return -1 key or val is NULL || conflict key || malloc failed
 * @return  0 on success
 *
 */
int config_append_value(const char* key, const char* val); 


/**
 * @brief 返回配置文件中的配置项总数,如果没有初始化配置文件，则返回0
 *
 */
int config_dump_get_count();

/**
 * @brief 重设dump loop计数索引 使之回到初始.
 *
 */
void config_dump_loop_reset();

/**
 * @brief dump配置项，每调用一次，返回一个配置，key和val的地址别保存在key,val指针中。
 * @param key, 保存key的地址， val，保存val的地址
 * @return 0, 表示没有下一项了，已经全部返回。继续调用将从头开始返回. 1，表示可以继续调用返回配置项.
 */
int config_dump_loop_next(char **key, char **val);

lua_State*  set_config_use_lua( int is_set );

/*
inline const char * config_get_strval_with_def(const char* key,const char* def ){
	const char *  v= config_get_strval(key)  ;
	if ( v!=NULL){
		return  v;
	}else{
		return def;
	}
}
*/

#endif // LIBTAOMEE_CONFIG_H_
  
