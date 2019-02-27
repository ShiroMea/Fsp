#include "base.h"

INDEX *HEAD;
/* 全局的数据链表的头节点 */

unsigned long int MEMBER = 1;
/* 成员计数从1开始计算 */

unsigned long int MAX_LINE = 10;
/* 默认最大行数10行 */

bool IGNORE_CASE = false;
/* 默认关闭忽略大小写 */

static unsigned int width = 0; 
/* 存储长度最长的名称 read_index函数设置 */

/* 从数据头中获取名称
 * data   保存数据头的缓冲区
 * buffer 保存名称的缓冲区
 * 成功:返回0
 * 失败:返回1 (数据头非法)
 */
static int get_func(const char *data,char *buffer)
{
	int count = 0;
	char *seek = NULL;
	char temp[256] = "";

	if ((seek = strrchr(data,FLAG_NOTE)) != NULL) {
		count = seek-data;
		strncpy(temp,data,count);
		strncpy(buffer,temp+strlen(FLAG_FUNC),count);
	} else {
		strncpy(buffer,data+strlen(FLAG_FUNC),255);
	}

	if (*buffer == '\0')
		return 1;

	return 0;
}

/* 从数据头中获取描述
 * 参数同 get_func
 * 成功:返回0
 * 失败:返回1 (将"暂无描述"拷贝到buffer中)
 */
static int get_note(const char *data,char *buffer)
{
	char *seek = NULL;
	if ((seek = strrchr(data,FLAG_NOTE)) != NULL) {
		/* 测试FLAG_NOTE后面有没有注释 */
		if (*(seek+1) != '\0') {
			strncpy(buffer,seek+1,255);
			return 0;
		}
	}

	strcpy(buffer,"(暂无描述)");

	return 1;
}

/* 截断过长的字符串
 * string  需要输出的字符串
 * col     终端的列数
 * 返回:string的首地址
 */
static const char *cut_string(char *string,int col)
{
	CHECK_ARGUMENT(string == NULL,NULL);

	int count = 0;
	int size = strlen("白");

	while (*(string+count) != '\0' && count < col-size) {
		/* 测试是否是一个汉字 */
		if (isascii(*(string+count)) != 0)
			count++;
		else
			count += size;
	}

	if (*(string+count) != '\0') {
		*(string+count) = '\0';
		strncat(string+count,"...",3);
	}

	return string;
}

/* 关键字高亮输出
 * output 进行输出的字符串
 * len    关键字长度
 * color  颜色种子
 */
static int color_output(const char *output,int len,SEED color)
{
	char *str = NULL;

	if (ENABLE_COLOR == true) {
		if ((str = sro_color(output,color_start,len,color)) == NULL) {
			fprintf(stderr,"E:%s:发生了错误\n",__func__);
			return 1;
		}
		printf("%s\n",cut_string(str,get_terminal_size()[1]));
	} else if (ENABLE_COLOR == false) {
		if ((str = strdup(output)) == NULL) {
			fprintf(stderr,"E:%s:发生了错误\n",__func__);
			return 1;
		}
		printf("%s\n",cut_string(str,get_terminal_size()[1]));
	}

	free(str);
	return 0;
}

/* 进行数据匹配
 * str   合成的字符串
 * key   搜索关键字
 * doit  进行对比的函数
 * 失败:返回-1
 * 匹配:返回0
 * 不匹配:返回1
 */
static int indexcmp(const char *str,const char *key,\
		int (*doit)(const char *s1,const char *s2))
{
	int ok = 1;
	char *key_bak = NULL;
	char *str_bak = NULL;

	if ((key_bak = strdup(key)) == NULL) {
		fprintf(stderr,"E:%s:%s\n",__func__,strerror(errno));
		return -1;
	}

	if ((str_bak = strdup(str)) == NULL) {
		fprintf(stderr,"E:%s:%s\n",__func__,strerror(errno));
		free(key_bak);
		return -1;
	}

	if (IGNORE_CASE == false) {
		if ((*doit)(str,key) == 0)
			ok = 0;
	} else if (IGNORE_CASE == true) {
		if ((*doit)(sro_strlwr(str_bak),sro_strlwr(key_bak)) == 0)
			ok = 0;
	}

	free(key_bak);
	free(str_bak);

	return ok;
}

/* 找到跳转命令并返回进行搜素的关键字
 * 如果指令非法返回NULL
 */
static const char *find_command(const char *buffer)
{
	unsigned int seek = 0;
	unsigned int count = 0;
	unsigned int member = 0;

	/* 后期添加执行的命令将在此处添加 */
	const char command[1][9] = {
		"fsp:goto",
	};

	member = sizeof(command) / sizeof(command[0]);

	for (count = 0;count < member;count++) {

		if (strncmp(buffer,command[count],strlen(command[count])))
			continue;
		else
			break;
	}

	/* 没有找到匹配的命令 */
	if (count == member)
		return NULL;

	seek = strlen(command[count]);

	for (;buffer[seek] ==  ' ';seek++);

	return (buffer + seek);
}

/* 将指定的节点读取出来
 * node    需要进行读取的节点
 * 成功:返回0
 * 失败:返回1
 */
static int read_node(const INDEX node)
{
	int lines = 0,lenght = 0;
	char buffer[BUFSIZ] = "";
	char *output = NULL;
	const char *key = NULL;
	gzFile gz = (gzFile)NULL;

	if ((gz = open_gz(node.file,"r")) == (gzFile)NULL)
		return 1;

	if (gzseek(gz,node.offset,SEEK_SET) == -1) {
		fprintf(stderr,"E:%s:设置偏移量失败\n",__func__);
		gzclose(gz);
		return 1;
	}

	while (!gzeof(gz) && gzgets(gz,buffer,sizeof(buffer)) != NULL) {

		if (strncmp(buffer,FLAG_FUNC,strlen(FLAG_FUNC)) == 0)
			break;

		if ((key = find_command(buffer)) != NULL) {

			sro_delenter(buffer);
			search_node(HEAD,key);
			goto END;
		}

		lines++;
		if (lines == get_terminal_size()[0]) {
			printf(" 更多\r");
			if (sro_getch() == 'q')
				goto END;
			lines = 0;
		}

		/* 颜色可用 */
		if (ENABLE_COLOR == true) {

			lenght = find_mark(buffer,']');

			if (buffer[0] == '[' && lenght != -1) {
				output = sro_color(buffer,0,lenght+1,GREEN_F);
				if (output == NULL)
					printf("%s",del_sequence(buffer));
				else {
					printf("%s",del_sequence(output));
					free(output);
					output = NULL;
				}
			} else if (buffer[0] == '[')
				printf("%s",del_sequence(buffer));
			else
				printf("%s",buffer);

		/* 颜色不可用 */
		} else if (ENABLE_COLOR == false) {

			if (buffer[0] == '[')
				printf("%s",del_sequence(buffer));
			else
			   	printf("%s",buffer);

		}

		memset(buffer,'\0',sizeof(buffer));
		fflush(stdout);
	}

END:
	gzclose(gz);
	return 0;
}

/* head    数据链表的头节点
 * number  返回查找到的结果数量
 * keyword 查找的关键字
 * count   最大数据结果
 * 返回值:
 * 返回查找到的第一个名称
 */
/*
static const char *count_search(INDEX *head,int *number,\
		const char *keyword,int count)
{
	int i = 0;
	char buffer[BUFSIZ] = "";
	const char *func = NULL;

	while (head != (INDEX *)NULL) {
		snprintf(buffer,sizeof(buffer),"%-*s %s",width,head->func,head->note);

		if (indexcmp(buffer,keyword,sro_fuzzy) == 0) {
			if (i == 0)
				func = head->func;

			if (count > 0) {
				color_output(buffer,strlen(keyword),RED_F);
				count--;
			} else {
				break;
			}
			i++;
		}

		head = head->next;
	}
	*number = i;
	return func;
}
*/

/*
static void down(int count)
{
	while (count-- > 0)
		printf("\n");
}
*/

/* 获取某个数据文件中的词条数量 
 * data  为数据链表的头节点
 * name  为数据文件名称
 * 返回值:正常返回词条数量,出现错误或没有找到返回0
 */
static unsigned long get_member(INDEX *data,const char *name)
{

	char *tmp = NULL;
	bool success = false;
	unsigned long int member = 0;

	while (data != (INDEX *)NULL) {
		tmp = ((tmp = strrchr(data->file,'/')) != NULL)?tmp + 1:NULL;

		if (strncmp(name,tmp,strlen(name)) == 0) {
			member++;
			success = true;
		}

		data = data->next;
	}

	return (success == true)?member:0;
}

/* 获取文件名称
 * data  数据链表的头节点
 * 该函数调用一次返回一个文件名,直到链表到底
 * 返回值:返回一个文件名,链表到底或没有找到返回NULL
 */
static const char *get_name(INDEX *data)
{
	char *name = NULL;
	static INDEX *seek;
	static char old_name[256];

	if (seek == (INDEX *)NULL)
		seek = data;

	while (seek != (INDEX *)NULL) {
		name = ((name = strrchr(seek->file,'/')) != NULL)?name + 1:NULL;

		if (strcmp(name,old_name)) {
			strncpy(old_name,name,sizeof(old_name));
			return name;
		}

		seek = seek->next;
	}

	return NULL;
}

int del_emptynode(INDEX *head)
{
	CHECK_ARGUMENT(head == (INDEX *)NULL,1);

	INDEX *prev = NULL;

	while (head != NULL) {

		if (*(head->func) == '\0') {
			if (prev != NULL)
				prev->next = head->next;
			free(head);
			head = prev;
			MEMBER--;
		}
	
		prev = head;
		if (head != NULL)
			head = head->next;
	}

	return 0;
}

INDEX *read_index(const char *filename)
{
	CHECK_ARGUMENT(filename == NULL,(INDEX *)NULL);

	gzFile gz = (gzFile)NULL;

	if ((gz = open_gz(filename,"r")) == (gzFile)NULL) {
		return ((INDEX *)NULL);
	}

	unsigned long long int line = 0;
	char data[BUFSIZ] = "",buffer[256] = "";
	INDEX *head = (INDEX *)NULL, *temp = head, *next = head;

	if ((head = (INDEX *)calloc(1,sizeof(INDEX))) == (INDEX *)NULL) {
		fprintf(stderr,"E:%s:%s\n",__func__,strerror(errno));
		gzclose(gz);
		return ((INDEX *)NULL);
	}

	temp = head;

	line = 1;
	MEMBER = 1;
	while (!gzeof(gz) && gzgets(gz,data,sizeof(data)) != NULL) {

		if (strncmp(data,FLAG_FUNC,strlen(FLAG_FUNC)) != 0) {
			line++;
			continue;
		}

		sro_delenter(data);
		if (get_func(data,buffer) == 1) {
			fprintf(stderr,"W:%s 中第 %lld 行无效的数据头\n",filename,line);
			continue;
		}

		strncpy(temp->func,buffer,sizeof(temp->func));
		if (strlen(buffer) > width)
			width = strlen(buffer);
		memset(buffer,'\0',sizeof(buffer));

		get_note(data,buffer);
		strncpy(temp->note,buffer,sizeof(temp->note));
		memset(buffer,'\0',sizeof(buffer));

		if ((next = (INDEX *)calloc(1,sizeof(INDEX))) == (INDEX *)NULL) {
			fprintf(stderr,"E:%s:%s\n",__func__,strerror(errno));
			index_recovery(head);
			gzclose(gz);
			return ((INDEX *)NULL);
		}

		strncpy(temp->file,filename,sizeof(temp->file));
		temp->offset = gztell(gz);
		temp->next = next;
		temp = next;

		line++;
		MEMBER++;

		memset(data,'\0',sizeof(data));
	}

	del_emptynode(head);

	if (MEMBER == 0) {
		fprintf(stderr,"E:%s 是无效的数据文件\n",filename);
		gzclose(gz);
		return (INDEX *)NULL;
	}

	gzclose(gz);
	return ((INDEX *)head);
}

void index_recovery(INDEX *head)
{ 
	if (head == NULL) {
		errno = EINVAL;
		fprintf(stderr,"E:%s:%s\n",__func__,strerror(errno));
		return ;
	}

	INDEX *temp = (INDEX *)NULL;

	while (head != (INDEX *)NULL) {
		temp = head->next;
		if (head != (INDEX *)NULL)
			free(head);
		head = temp;
	}

	return ;
}

int creat_index(gzFile index_file,const INDEX *head)
{
	CHECK_ARGUMENT((index_file == NULL || head == NULL),1);

	int count = 0;

	while (head != NULL) {
		count = gzwrite(index_file,(void *)head,sizeof(INDEX));

		if (count != sizeof(INDEX))
			fprintf(stderr,"W:写入索引数据有丢失\n");

		head = head->next;
	}

	return 0;
}

INDEX *resume_index(gzFile index_file)
{
	CHECK_ARGUMENT(index_file == NULL,(INDEX *)NULL);

	INDEX *head = (INDEX *)NULL,*temp = head,*next = head;

	if ((head = (INDEX *)calloc(1,sizeof(INDEX))) == (INDEX *)NULL) {
		fprintf(stderr,"E:%s\n",strerror(errno));
		return ((INDEX *)NULL);
	}

	temp = head;
	MEMBER = 1;
	while (gzread(index_file,temp,sizeof(INDEX)) > 0) {

		if ((next = (INDEX *)calloc(1,sizeof(INDEX))) == (INDEX *)NULL) {
			fprintf(stderr,"E:%s\n",strerror(errno));
			index_recovery(head);
			return ((INDEX *)NULL);
		}

		if (strlen(temp->func) > width)
			width = strlen(temp->func);

		temp->next = next;
		temp = next;

		MEMBER++;
	}

	del_emptynode(head);
	return head;
}

int update_index(void)
{
	DIR *dirp = NULL;
	struct dirent *dir = NULL;
	const char *suffix = NULL;
	char buffer[256] = "";
	gzFile index_file = (gzFile)NULL;
	INDEX *index = (INDEX *)NULL;
	
	if (*DATA_PATH == '\0') {
		fprintf(stderr,"E:数据目录没有设置\n");
		return 1;
	}

	if ((dirp = opendir(DATA_PATH)) == NULL) {
		fprintf(stderr,"E:打开 %s 目录失败:%s\n",DATA_PATH,strerror(errno));
		return 1;
	}

	snprintf(buffer,sizeof(buffer),"%s/%s",DATA_PATH,INDEX_FILE);
	if ((index_file = open_gz(buffer,"w")) == (gzFile)NULL)
		return 1;

	while ((dir = readdir(dirp)) != NULL) {

		if (dir->d_type == DT_REG) {
			if ((suffix = find_extension(dir->d_name)) == NULL)
				continue;
			if (strcmp(suffix,"fsp") == 0) {
				snprintf(buffer,sizeof(buffer),"%s/%s",DATA_PATH,dir->d_name);
				if ((index = read_index(buffer)) == (INDEX *)NULL)
					continue;
				if (creat_index(index_file,index) == 1)
					fprintf(stderr,"E:将 %s 添加到索引失败\n",dir->d_name);
				index_recovery(index);
			}
			memset(buffer,'\0',sizeof(buffer));
		}
	}

	gzclose(index_file);
	closedir(dirp);
	return 0;
}

int browse_index(INDEX *head)
{
	CHECK_ARGUMENT(head == (INDEX *)NULL,1);

	int count = 0;
	char buffer[BUFSIZ] = "";

	if (ENABLE_COLOR == true)
		sro_printf("#S5<Space>更多 <q>退出 一共:%u个#S0\n",MEMBER);

	while (head != (INDEX *)NULL) {

		snprintf(buffer,sizeof(buffer),"%-*s %s",width,head->func,head->note);

		if (ENABLE_COLOR == true) {

			if (pause_output(&count) == 1)
				break;
			printf("%s\n",cut_string(buffer,get_terminal_size()[1]));

		} else {
			printf("%s\n",buffer);
		}
		count++;
		memset(buffer,'\0',sizeof(buffer));
		head = head->next;
	}
	return 0;
}

int search_index(INDEX *head,const char *key)
{
	CHECK_ARGUMENT(head == (INDEX *)NULL,1);

	bool ok = false;
	int count = 0;
	char buffer[BUFSIZ] = "";

	if (ENABLE_COLOR == true)
		sro_printf("#S5<Space>更多 <q>退出 一共:%u个#S0\n",MEMBER);

	while (head != (INDEX *)NULL) {

		snprintf(buffer,sizeof(buffer),"%-*s %s",width,head->func,head->note);

		if (indexcmp(buffer,key,sro_fuzzy) == 0) {
			if (pause_output(&count) == 1)
				break;
			color_output(buffer,strlen(key),\
					(IGNORE_CASE == false)?HIGHT_S|RED_F:HIGHT_S|PURPLE_F);
			count++;
			ok = true;
		}

		head = head->next;
		memset(buffer,'\0',sizeof(buffer));
	}

	if (ok == false) {
		fprintf(stderr,"E:没有找到与 %s 有关的内容\n",key);
		return 1;
	}

	return 0;
}

int search_node(INDEX *head,const char *key)
{
	CHECK_ARGUMENT(head == (INDEX *)NULL,1);

	bool ok = false;
	bool again = false;

	while (head != (INDEX *)NULL) {

		if (indexcmp(head->func,key,strcmp) == 0) {
			if ((again == true)  && sro_choose("有重复继续显示 %s ?",key))
				break;
			ok = (read_node(*head) == 0)?true:false;
			again = true;
		}


		head = head->next;
	}

	if (ok == false) {
		fprintf(stderr,\
				"E:没有找到与 %s 有关的内容\n"\
				"T:试试执行: %s --index=%s\n"\
				,key,PRONAME,key);
		return 1;
	}

	return 0;
}

int search_argv(INDEX *head,char *const argv[])
{
	int retc = 0;
	bool again = false,doit = true;

	argv++;

	for (int i = 0;argv[i] != NULL;i++) {

		if (*argv[i] == '-')
			continue;

		doit = true;

		for (int count = sizeof(OPTARGU)/sizeof(char *);count >= 0;count--) {

			if (OPTARGU[count] == NULL)
				continue;

			if (strcmp(OPTARGU[count],argv[i]) == 0)
				doit = false;
		}

		if (again == true) {
			printf("按任意键继续...\r");
			sro_getch();
		}

		if (doit == true) {
			retc = search_node(head,argv[i]);
			again = true;
		}
	}

	return retc;
}

#if 0
int input_search(INDEX *head)
{
	char buffer[128] = "";
	const char *key = NULL;
	int count = 0,line = MAX_LINE;

	down(MAX_LINE + 1);
	count = 0;
	while (1) {

		if (! isascii(buffer[count] = sro_getch())) {
			printf("抱歉:该功能仅支持英文和数字\r");
			memset(buffer,'\0',sizeof(buffer));
			continue;
		}

		switch (buffer[count]) {
			/* \x7F是DEL的十六进制写法`man 7 ascii` */
			case '\x7F':
				if (count > 0) {
					buffer[count] = '\0';
					count--;
					if (isascii(buffer[count])) {
						buffer[count] = '\0';
						count--;
					} else {
						memset(buffer+count - 3,'\0',3);
					}
				}
				break;
			case '\n':
				sro_delenter(buffer);
				refresh_screen(line + 1,get_terminal_size()[1]);
				if (key != NULL) {
					search_node(head,key);
					down(line + 1);
				}
				count--;
				break;
			case ' ':
				return 1;
				break;
			default:
				break;
		}
		refresh_screen(line + 1,get_terminal_size()[1]);
		printf("搜索:%s\n",buffer);
		key = count_search(head,&line,buffer,MAX_LINE);
		count++;
	}
	return 0;
}
#endif

int print_info(INDEX *head)
{
	if (head == (INDEX *)NULL)
		return 1;

	int count = 0;
	int width = 0;
	char *progress = NULL;
	const char *temp = NULL;

	int *numbers = NULL;
	char (*names)[128] = {NULL};

	while (get_name(head) != NULL) count++;

	if ((names = (char (*)[128])calloc(count,128 * sizeof(char))) == NULL) {
		fprintf(stderr,"E:%s\n",strerror(errno));
		return 2;
	}

	if ((numbers = (int *)calloc(count,sizeof(int))) == NULL) {
		free(names);
		fprintf(stderr,"E:%s\n",strerror(errno));
		return 2;
	}

	for (count = 0;(temp = get_name(head)) != NULL;count++) {

		if (strlen(temp) > width)
			width = strlen(temp);

		if (temp != NULL) {
			strncpy(names[count],temp,sizeof(*names));
			numbers[count] = get_member(head,temp);
		}
	}

	PRINT_LINE(get_terminal_size()[1],'=');
	sro_printf("数据目录:#S1#F6%s#F0\n",DATA_PATH);
	sro_printf("#F2%s#F0中包含了#F3%d#F0个文件一共#F3%lu#F0个词条\n",\
			INDEX_FILE,count,MEMBER);
	PRINT_LINE(get_terminal_size()[1],'=');

	for (count--;count >= 0;count--) {
		double perc = ((double)numbers[count] / (double)MEMBER) * 100.0;

		progress = sro_progress("#.",20,perc);
		sro_printf("%-*s %-5d %-6.2f%% [#F7%s#F0]\n",width,names[count],\
				numbers[count],perc,(progress != NULL)?progress:"Error");

	}

	if (progress != NULL)
		free(progress);

	PRINT_LINE(get_terminal_size()[1],'=');

	free(names);
	free(numbers);

	return 0;
}
