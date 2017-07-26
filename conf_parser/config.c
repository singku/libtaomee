#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <libtaomee/hash_algo.h>
#include <libtaomee/list.h>

#include "config.h"


#define INIKEY_HASHBITS	8
#define CONFIG_VAL_SIZE 4096

static list_head_t ini_key_head[1 << INIKEY_HASHBITS];

static int has_init;

lua_State* L=NULL;


//是否使用lua 做配置文件
static int g_config_use_lua=0;

typedef struct config_pair {
	struct list_head list;
	char* val;
	char  key[];
} config_pair_t;

lua_State*  set_config_use_lua( int is_set )
{
	g_config_use_lua=(is_set!=0);
	L = lua_open();
	luaL_openlibs(L );
	return L;
}

//------------------------------------------------------------
// Static Functions
//
static inline struct list_head*
config_key_hash(const char* key) 
{	
	return &ini_key_head[r5hash(key) & ((1 << INIKEY_HASHBITS) - 1)];
}

static inline int hash_key(const char *key)
{
	return r5hash(key) & ((1 << INIKEY_HASHBITS) - 1);
}

/*
// return -1 on error, 0 on success
static int config_put_value(const char* key, const char* val) 
{
	int len = strlen(key) + 1;
	int val_len = strlen(val) + 1;
	if (val_len > CONFIG_VAL_SIZE)
		return -1;

	struct config_pair* mc = malloc(sizeof(struct config_pair) + len);
	if (!mc) {
		return -1;
	}
	memcpy(mc->key, key, len);
	mc->val = (char*)malloc(CONFIG_VAL_SIZE);
	//memcpy(mc->val, val, strlen(val) + 1);
	//mc->val = strdup(val);
	if (!mc->val) {
		free(mc);
		return -1;
	}
	memcpy(mc->val, val, val_len);
	list_add(&mc->list, config_key_hash(key));

	return 0;
}
*/

// return -1 on error key or val is NULL or no key exist,or update failed 0 on success
int config_update_value(const char* key, const char* val)
{
	int val_len = strlen(val) + 1;
	if (key == NULL || val == NULL || val_len > CONFIG_VAL_SIZE)
		return -1;

	int hash = hash_key(key);
	list_head_t* p = ini_key_head[hash].next;
	while (p != &ini_key_head[hash]) {
		config_pair_t* mc = list_entry(p, config_pair_t, list);
		if (strlen(mc->key) == strlen(key) && strcmp(mc->key, key) == 0 
			&& (strlen(mc->val) != strlen(val) || strcmp(mc->val, val) != 0)) {
			memcpy(mc->val, val, val_len);//key相等，val不相等，则update
            return 0;
#if 0
			char *tmp = mc->val;//保存原来的地址
			mc->val = strdup(val);//尝试分配新空间
			if (!mc->val) {//如果分配失败，则还原
				mc->val = tmp;
				return -1;
			} else {
				free(tmp);//成功则释放原来的
				return 0;
			}
#endif
		}
		p = p->next;
	}
	return -1;
}

// return -1 on error key or val is NULL or conflict key, 0 on success
int config_append_value(const char* key, const char* val) 
{
	int val_len = strlen(val) + 1;
	if (key == NULL || val == NULL || val_len > CONFIG_VAL_SIZE)
		return -1;
	
	int hash = hash_key(key);
	list_head_t* p = ini_key_head[hash].next;
	while (p != &ini_key_head[hash]) {
		config_pair_t* mc = list_entry(p, config_pair_t, list);
		if (strlen(mc->key) == strlen(key) && strcmp(mc->key, key) == 0) {
			return -1;
		}
		p = p->next;
	}

	int len = strlen(key) + 1;
	struct config_pair* mc = malloc(sizeof(struct config_pair) + len);
	if (!mc) {
		return -1;
	}
	memcpy(mc->key, key, len);
	mc->val = (char*)malloc(CONFIG_VAL_SIZE);
	//mc->val = strdup(val);
	if (!mc->val) {
		free(mc);
		return -1;
	}
	memcpy(mc->val, val, val_len);
	list_add(&mc->list, &ini_key_head[hash]);

	return 0;
}
static int  config_reset_or_add_value( const char *  k,  const char *  v){
	if(config_append_value(k, v) == -1) {
		if(config_update_value(k, v) == -1) {
			return -1;
		}
	}
	return 0;
}


// return -1 on error, 0 on success
static int parse_config(char* buffer)
{
	static const char myifs[256]
			= { [' '] = 1, ['\t'] = 1, ['\r'] = 1, ['\n'] = 1, ['='] = 1 };

	char*  field[2];
	char*  start = buffer;
	size_t len   = strlen(buffer);
	while (buffer + len > start) {
		char* end = strchr(start, '\n');
		if (end) {
			*end = '\0';
		}
		if ((*start != '#') && (str_split(myifs, start, field, 2) == 2)) {
			if (config_reset_or_add_value(field[0], field[1]) == -1) {
				return -1;
			}
		}
		if (end) {
			start = end + 1;
		} else {
			break;
		}
	}
	return 0;
}
//------------------------------------------------------------

int lua_config_init(const char* file_name) {
	int ret=luaL_dofile(L, file_name);
	if(ret!=0){
		printf("error : %s\n", lua_tostring(L, -1));
		return -1;
	}

	lua_pushnil(L);
	while (lua_next(L, LUA_GLOBALSINDEX )) {
		if (lua_type(L, -1) ==LUA_TSTRING || lua_type(L, -1) ==LUA_TNUMBER ){

			const char * p_k=luaL_checkstring(L, -2);
			const char * p_v=luaL_checkstring(L, -1);

			if ( strcmp(p_k,"_VERSION") ){
				if (config_reset_or_add_value(p_k,p_v ) == -1) {
					return -1;
				}
			}

		}

		lua_pop(L, 1);
	}

	config_reset_or_add_value("_use_lua_config" ,"1"  );
	

	return  0;
}

int config_init(const char* file_name)
{

	int ret_code = -1;

	if (!has_init) {
		int i;
		for (i = 0; i < (1 << INIKEY_HASHBITS); i++) {
			INIT_LIST_HEAD(&ini_key_head[i]);
		}
		has_init = 1;
	}

	if(g_config_use_lua){
		return lua_config_init(file_name );
	}else{
		char* buf;
		int   len = mmap_config_file(file_name, &buf);
		if (len > 0) {
			ret_code = parse_config(buf);
			munmap(buf, len);
		}
	}

	return ret_code;
}

int config_update(const char* file_name)
{
	return config_init(file_name);
}

void config_exit()
{
	int i;
	if (!has_init)
		return;

	for (i = 0; i < (1 << INIKEY_HASHBITS); ++i) {
		list_head_t* p = ini_key_head[i].next;
		while (p != &ini_key_head[i]) {
			config_pair_t* mc = list_entry(p, config_pair_t, list);
			p = p->next;
			free(mc->val);
			free(mc);
		}
	}
	has_init = 0;
}

int mmap_config_file(const char* file_name, char** buf)
{
	int ret_code = -1;

	int fd = open(file_name, O_RDONLY);
	if (fd == -1) {
		return -1;
	}

	int len = lseek(fd, 0L, SEEK_END);
	lseek(fd, 0L, SEEK_SET);

	*buf = mmap(0, len + 1, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (*buf != MAP_FAILED) {
		read(fd, *buf, len);
		(*buf)[len] = 0;
		ret_code = len + 1;
	}
	close(fd);

	return ret_code;
}

/*
 * NULL IFS: default blanks
 * first byte is NULL, IFS table
 * returns number of words splitted up to on success
 */
int str_split(const char* ifs, char* line, char* field[], int n)
{
	static const char default_ifs[256]
			= { [' '] = 1, ['\t'] = 1, ['\r'] = 1, ['\n'] = 1, ['='] = 1 };

	if (ifs == 0) {
		ifs = default_ifs;
	}

	int i = 0;
	while (1) {
		while (ifs[(unsigned char)*line]) {
			++line;
		}
		if (*line == '\0') {
			break;
		}
		field[i++] = line;

		// remove tailing spaces
		if (i >= n) {
			line += strlen(line) - 1;
			while (ifs[(unsigned char)*line]) {
				--line;
			}
			line[1] = '\0';
			break;
		}

		// add a tailing '\0'
		while (*line && !ifs[(unsigned char)*line]) {
			++line;
		}
		if (!*line) {
			break;
		}
		*line++ = '\0';
	}

	return i;
}

int config_get_intval(const char* key, int def)
{

	char* val = config_get_strval(key);
	if (val == 0) {
		return def;
	}
	return atoi(val);
}


char* config_get_strval(const char* key)
{
	struct config_pair* mc;

	list_head_t* hlist = config_key_hash(key);
	list_for_each_entry(mc, hlist, list) {
		if (!strcmp(key, mc->key)) {
			return mc->val;
		}
	}

	return 0;
}

int config_dump_get_count()
{
	int i;
	list_head_t *p = NULL;
	int total_items = 0;

	if (!has_init)
		return 0;

	for (i = 0; i < (1 << INIKEY_HASHBITS); i++) {
		p = ini_key_head[i].next;
		while (p != &ini_key_head[i]) {
			p = p->next;
			total_items++;
		}
	}
	return total_items;
}

static int config_dump_loop_index = 0;
static int config_dump_loop_first_flag = 0;
static list_head_t *config_dump_loop_iter = NULL;

void config_dump_loop_reset() 
{
	config_dump_loop_index = 0;
	config_dump_loop_first_flag = 0;
}
int config_dump_loop_next(char **key, char **val)
{
	if (config_dump_loop_first_flag++ == 0){
		config_dump_loop_iter = ini_key_head[0].next;//first config item
	}

retry:
	if (config_dump_loop_iter != &ini_key_head[config_dump_loop_index]) {
		config_pair_t *mc = list_entry(config_dump_loop_iter, config_pair_t, list);
		*key = mc->key;
		*val = mc->val;
		config_dump_loop_iter = config_dump_loop_iter->next;
		return 1;
	} else if (++config_dump_loop_index >= (1 << INIKEY_HASHBITS)) {
		//round back reset
		config_dump_loop_index = 0;
		config_dump_loop_first_flag = 0;
		return 0;
	} else {
		config_dump_loop_iter = ini_key_head[config_dump_loop_index].next;
		goto retry;
	}
}
