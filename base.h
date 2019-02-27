/***********************
 *    FSP base API     *
 *      Shiro          *
 **********************/

#ifndef BASE_H
#define BASE_H

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <locale.h>
#include <getopt.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <zlib.h>
#include <shiro.h>
#include <color.h>

#define FLAG_NOTE '>'
#define FLAG_FUNC "__%"
#define INDEX_FILE "index.ifsp"

#define CHECK_ARGUMENT(expression,retval) \
	do {\
		if ((expression)) { \
			errno=EINVAL; \
			fprintf(stderr,"E:%s:%s\n",__func__,strerror(errno));\
			return ((retval)); }\
	} while (0)

#define SET_PRONAME(argv0) \
	do {\
		PRONAME = (PRONAME = strchr((argv0),'/'))?PRONAME+1:(argv0);\
	} while (0)

#define SUPPORT_COLOR \
	do {\
		if (isatty(STDOUT_FILENO) == 1) ENABLE_COLOR = true;\
		else ENABLE_COLOR = false;\
	} while (0)

#define PRINT_LINE(len,ch) \
	do {\
		for (int i = 0;(len) > i;i++) putchar((ch));\
		putchar('\n');\
	} while (0)

extern char *PRONAME;
/* 保存程序的名称 base.c */

extern char DATA_PATH[];
/* 保存FSP环境变量中的内容 base.c */

extern bool ENABLE_COLOR;
/* 终端是否支持颜色 base.c */

extern unsigned long int MEMBER;
/* 保存文件中的数据段数量 data.c */

extern unsigned long int MAX_LINE;
/* 浏览目录最大输出行数(默认10) data.c */

extern bool IGNORE_CASE;
/* 搜索目录的时候是否忽略大小写 data.c */

extern const char *OPTARGU[4];
/* 存储带参数选项的参数指针 main.c */

/* 程序的主体数据结构体 */
typedef struct node {
	z_off_t offset;       /* 数据段在文件中的偏移量 */
	char    func[128];    /* 数据段名称 */
	char    note[128];    /* 数据段备注 */
	char    file[256];    /* 保存数据文件位置 */
	struct node *next;    /* 下一个节点 */
} INDEX;

extern INDEX *HEAD;
/* 存储数据链表 */

/**************
 *   base.c
 **************/

int set_datapath(const char *user);
/* 从FSP环境变量中获取数据的位置
 * user:用户指定数据目录(可以为NULL)
 * 成功:设定全局变量DATA_PATH,返回0
 * 失败:返回1
 */

mode_t check_type(const char *target,mode_t mode);
/* 检测target类新是否是指定的类型
 * 成功:返回这个文件的类型
 * 失败:返回-1
 */

const char *get_type(mode_t mode);
/* 获取文件类型 */

int open_fd(const char *filename,int flag);
/* 打开或者创建普通文件
 * filename 文件名称
 * flag     打开的方式
 * 成功:返回文件描述符
 * 失败:返回-1
 */

gzFile open_gz(const char *filename,const char *flag);
/* 打开或者建立.gz类型的文件
 * filename 文件名称
 * flag     打开的方式
 * 成功:返回文件指针
 * 失败:返回NULL
 */
const char *find_extension(const char *filename);
/* 查找filename中的拓展命
 * filename  需要查找拓展命的文件名
 * 成功:查找到的拓展命
 * 失败:返回NULL
 */

const int *get_terminal_size(void);
/* 获取当前终端的大小 STDOUT_FILENO
 * get_terminal_size()[0] 行数
 * get_terminal_size()[1] 列数
 * 获取失败将使用默认值,行数=10 列数=70
 */

int pack(const char *filename);
/* 对数据进行打包解包
 * filename 要进行打包的文件
 * 成功:返回0
 * 失败:返回1
 */

int pause_output(int *count);
/* 暂停输出
 * count 为计数器指针
 * 输入'q':返回1
 */
void set_maxline(const char *strnum);
/* 设置索引输出行数(MAX_LINE)
 * strnum  为optget_long的optarg参数
 */

int find_mark(const char *str,char mark);
/* 在str中查找指定的字符(mark)
 * 会忽略转义的字符,如\(mark) 会被忽略
 * 返回找到的字符位置
 * 失败返回-1
 */
const char *del_sequence(char *str);
/* 删除字符中的转义字符'\'
 * 输出'\'的方法'\\'
 * 返回处理后的str
 */

void refresh_screen(const int row,const int col);

/**************
 *   data.c
 **************/

INDEX *read_index(const char *filename);
/* 从fsp文件中读取索引信息
 * filename  需要读取索引的文件
 * 成功:返回头节点
 * 失败:返回NULL
 */

void index_recovery(INDEX *index);
/* 回收索引 */

int creat_index(gzFile index_file,const INDEX *head);
/* 创建/更新索引文件
 * index_file 索引文件指针
 * head       索引头节点
 * 成功:返回0
 * 失败:返回1
 */

INDEX *resume_index(gzFile index_file);
/* 恢复索引
 * index_file 索引文件指针
 * 成功:返回索引头节点
 * 失败:返回NULL
 */

int update_index(void);
/* 更新 $FSP/index.ifsp 索引文件
 * 成功:返回0
 * 失败:返回1
 */

int browse_index(INDEX *head);
/* 浏览索引中的所有成员
 * head   索引的头节点
 */

int search_index(INDEX *head,const char *key);
/* 在索引中搜索
 * head  索引头节点
 * key   关键字
 * 成功:返回0
 * 失败:返回1
 */

int search_node(INDEX *head,const char *key);
/* 搜索节点并显示节点信息
 * head  索引头节点
 * key   关键字
 * 成功:返回0
 * 失败:返回1
 */

INDEX *open_index(void);
/* 打开索引文件
 * 成功:返回索引头节点
 * 失败:返回NULL
 */

int search_argv(INDEX *head,char *const argv[]);
/* 从main函数的argv中搜索关键字
 * 会跳过'-''--'开头的字符串,以及选项的参数
 * head   索引头节点
 * argv   main函数的argv参数
 * 成功:返回0
 * 失败:返回1
 */

/* int input_search(INDEX *head); */
/* 即时搜索(已作废) */

int print_info(INDEX *head);
/* 显示数据文件的统计信息
 * head  为数据链表的头节点
 * 返回值:成功返回0,出现错误返回1或2
 */

#endif
