/*
  author: liuhengloveyou@gmail.com
  date  : 2013-10-09

  C语言读写配置文件实现

  每行为一条记录, 格式如下：
  key = value # 注释

  '#'字符后的所有内容都作为注释忽略
  key和value的头尾空白会被砍掉, 中间可以有空格
*/

#ifndef C_CONFIG_L
#define C_CONFIG_L

#ifdef __cplusplus
extern "C" {
#endif


struct config
{
	char *name;
	char *value;
	struct config *next;
};
typedef struct configl
{
	char *fn;
	struct config *confs;
} config_t;

extern config_t *confRead(const char *file_name);
extern int confWrite(config_t *one);
extern void confDestory(config_t *one);
extern const char *confGet(const config_t *one, const char *key);
extern int confSet(config_t *one, const char *key, const char *val);
extern void confDel(config_t *one, const char *key);

extern int init_conf();
extern int my_confGet(config_t *conf_hd, char *key);
extern int my_confSet(config_t *conf_hd, char *key, int val);
extern config_t * my_confRead();

#ifdef __cplusplus
}
#endif

#endif
