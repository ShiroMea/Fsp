#include "base.h"

bool ENABLE_COLOR = false;
/* 默认不使用颜色 */
char *PRONAME = NULL;

char DATA_PATH[256] = "";

/* txt --> fsp */
static int pack_fsp(const int txt,gzFile fsp)
{
	int count = 0;
	void *buffer = NULL;

	if ((buffer = calloc(1,BUFSIZ)) == NULL) {
		fprintf(stderr,"E:%s:%s\n",__func__,strerror(errno));
		return 1;
	}

	while ((count = read(txt,buffer,BUFSIZ)) > 0) {
		if (gzwrite(fsp,buffer,count) != count)
			fprintf(stderr,"W:%s:写入数据丢失\n",__func__);
	}

	cfree(buffer);

	return 0;
}

/* fsp --> txt */
static int pack_txt(const int txt,gzFile fsp)
{
	int count = 0;
	void *buffer = NULL;

	if ((buffer = calloc(1,BUFSIZ)) == NULL) {
		fprintf(stderr,"E:%s:%s\n",__func__,strerror(errno));
		return 1;
	}

	while ((count = gzread(fsp,buffer,BUFSIZ)) > 0) {
		if (write(txt,buffer,count) != count)
			fprintf(stderr,"W:%s:写入数据时有丢失\n",__func__);
	}

	cfree(buffer);

	return 0;
}

static int mk_newname(const char *filename,char *buffer,size_t size)
{
	CHECK_ARGUMENT(filename == NULL || buffer == NULL,1);

	char *seek = NULL;
	const char *extens_name = NULL;

	memset(buffer,'\0',size);

	if ((extens_name = find_extension(filename)) == NULL)
		return 1;

	if (strcmp(extens_name,"fsp") == 0) {
		strncpy(buffer,filename,size);
		if ((seek = strrchr(buffer,'.')) == NULL)
			return 1;
		*(seek+1) = '\0';
		strncat(buffer,"txt",3);
	} else if (strcmp(extens_name,"txt") == 0) {
		strncpy(buffer,filename,size);
		if ((seek = strrchr(buffer,'.')) == NULL)
			return 1;
		*(seek+1) = '\0';
		strncat(buffer,"fsp",3);
	} else 
		return 1;

	return 0;
}

/* 刷新一块指定大小的屏幕
 * row 行数
 * col 列数
 */
void refresh_screen(const int row,const int col)
{
	int count = 0;

	_MOVEUP(row);
	while (count < row) {
		printf("%*s\n",col," ");
		count++;
	}
	_MOVEUP(row);
	return ;
}

int set_datapath(const char *user)
{
	const char *path = NULL;
	mode_t mode = 0;

	if ((user != NULL) && (*user != '\0')) {
		if ((mode = check_type(user,S_IFDIR)) != S_IFDIR) {
			fprintf(stderr,"E: %s 是%s\n",user,get_type(mode));
			return 1;
		}
		path = user;
	} else if ((path = getenv("FSP")) == NULL) {
		fprintf(stderr,"E:没有找到 $FSP 环境变量\n");
		memset(DATA_PATH,'\0',sizeof(DATA_PATH));
		return 1;
	}

	strncpy(DATA_PATH,path,sizeof(DATA_PATH));
	sro_delenter(DATA_PATH);

	if (*DATA_PATH == '\0') {
		fprintf(stderr,"E:无效的 $FSP 环境变量\n");
		return 1;
	}
	return 0;
}

mode_t check_type(const char *target,mode_t mode)
{
	CHECK_ARGUMENT(target == NULL,-1);

	struct stat buf;

	if (stat(target,&buf) < 0 ) {
		fprintf(stderr,"E:打开 %s 失败:%s\n",target,strerror(errno));
		return -1;
	}

	switch (mode) {
		case S_IFREG:
			if (S_ISREG(buf.st_mode))
				return S_IFREG;
			break;
		case S_IFDIR:
			if (S_ISDIR(buf.st_mode))
				return S_IFDIR;
			break;
		case S_IFBLK:
			if (S_ISBLK(buf.st_mode))
				return S_IFBLK;
			break;
		case S_IFCHR:
			if (S_ISCHR(buf.st_mode))
				return S_IFCHR;
			break;
		case S_IFLNK:
			if (S_ISLNK(buf.st_mode))
				return S_IFLNK;
			break;
		case S_IFIFO:
			if (S_ISFIFO(buf.st_mode))
				return S_IFIFO;
			break;
		case S_IFSOCK:
			if (S_ISSOCK(buf.st_mode))
				return S_IFSOCK;
			break;
		default:
			fprintf(stderr,"E:错误的类型 %d \n",mode);
			return -1;
	}

	return buf.st_mode;
}

const char *get_type(mode_t mode)
{
	static char type[20] = "";

	switch (mode & S_IFMT) {
		case S_IFREG:
			strncpy(type,"普通文件",sizeof(type));
			break;
		case S_IFDIR:
			strncpy(type,"目录",sizeof(type));
			break;
		case S_IFBLK:
			strncpy(type,"区块装置",sizeof(type));
			break;
		case S_IFCHR:
			strncpy(type,"字符装置",sizeof(type));
			break;
		case S_IFLNK:
			strncpy(type,"符号连接",sizeof(type));
			break;
		case S_IFIFO:
			strncpy(type,"先进先出",sizeof(type));
			break;
		case S_IFSOCK:
			strncpy(type,"Socket",sizeof(type));
			break;
		default:
			strncpy(type,"未知",sizeof(type));
	}
	return type;

}

gzFile open_gz(const char *filename,const char *flag)
{
	CHECK_ARGUMENT(filename == NULL,(gzFile)NULL);

	gzFile temp = NULL;
	mode_t mode = 0;

	if (strchr(flag,'w') == NULL) {
		if ((mode = check_type(filename,S_IFREG)) != S_IFREG) {
			if (mode != -1)
				fprintf(stderr,"E: %s 是%s\n",filename,get_type(mode));
			return ((gzFile)NULL);
		}
	}

	if ((temp = gzopen(filename,flag)) == (gzFile)NULL) {
		fprintf(stderr,"E:打开/创建文件 %s 失败:%s\n",filename,strerror(errno));
		return ((gzFile)NULL);
	}

	return ((gzFile)temp);
}

int open_fd(const char *filename,int flag)
{
	CHECK_ARGUMENT(filename == NULL,-1);

	int fd = 0;
	mode_t mode = 0;

	if ((flag & O_CREAT) != O_CREAT) {
		if ((mode = check_type(filename,S_IFREG)) != S_IFREG) {
			if (mode != -1)
				fprintf(stderr,"E: %s 是%s\n",filename,get_type(mode));
			return -1;
		}
	}

	umask(S_IXUSR | S_IXGRP | S_IXOTH);

	if ((fd = open(filename,flag,(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ))) \
			== -1) {
		fprintf(stderr,"E:打开/创建文件 %s 失败:%s\n",filename,strerror(errno));
		return -1;
	}

	return fd;
}

const char *find_extension(const char *filename)
{
	CHECK_ARGUMENT(filename == NULL,NULL);

	char *seek = NULL;

	if ((seek = strrchr(filename,'.')) == NULL)
		return NULL;

	if (*(seek+1) != '\0')
		return (seek+1);
	return seek;
}

const int *get_terminal_size(void)
{
	static int size[2] = {0};

	struct winsize win;

	if (ioctl(STDOUT_FILENO,TIOCGWINSZ,&win) == -1) {
		/*
		fprintf(stderr,"W:获取终端尺寸失败:%s\n",strerror(errno));
		fprintf(stderr,"W:使用缺省值:row=10,col=70\n");
		*/
		size[0] = 10;
		size[1] = 70;
	}

	size[0] = win.ws_row;
	size[1] = win.ws_col;

	return size;
}

int pack(const char *filename)
{
	CHECK_ARGUMENT(filename == NULL,1);

	int fd = 0;
	gzFile gz = (gzFile)NULL;
	char buffer[2][256] = {""};
	const char *extens_name= NULL;

	int open_flag = 0;
	char *gzopen_flag = NULL;

	int (*doit)(const int fd,gzFile gz) = NULL;

	if ((extens_name = find_extension(filename)) == NULL) {
		fprintf(stderr,"E:`%s` 没有拓展名\n",filename);
		return 1;
	}

	if (strcmp(extens_name,"fsp") == 0) {
		/* 解压 */
		open_flag = O_WRONLY | O_CREAT | O_TRUNC;
		gzopen_flag = "r";
		mk_newname(filename,buffer[0],sizeof(buffer[0]));
		strncpy(buffer[1],filename,sizeof(buffer[1]));
		doit = pack_txt;
	} else if (strcmp(extens_name,"txt") == 0) {
		/* 压缩 */
		open_flag = O_RDONLY;
		gzopen_flag = "w";
		strncpy(buffer[0],filename,sizeof(buffer[0]));
		mk_newname(filename,buffer[1],sizeof(buffer[1]));
		doit = pack_fsp;
	} else {
		fprintf(stderr,"E:`%s` 不是有效的拓展名\n",extens_name);
		return 1;
	}

	if ((fd = open_fd(buffer[0],open_flag)) == -1)
		return 1;

	if ((gz = open_gz(buffer[1],gzopen_flag)) == (gzFile)NULL)
		return 1;

	if ((*doit)(fd,gz) != 0) {
		close(fd);
		gzclose(gz);
		return 1;
	}

	close(fd);
	gzclose(gz);

	return 0;
}

int pause_output(int *count)
{
	CHECK_ARGUMENT(count == NULL,-1);

	int ch = 0;

	if (*count == MAX_LINE) {

		ch = sro_getch();

		if (ch == 'q' || ch == 'Q')
			return 1;

		refresh_screen(MAX_LINE,get_terminal_size()[1]);
		*count = 0;
	}

	return 0;
}

INDEX *open_index(void)
{
	char buffer[BUFSIZ] = "";
	gzFile gz = (gzFile)NULL;
	INDEX *index = (INDEX *)NULL;

	snprintf(buffer,sizeof(buffer),"%s/%s",DATA_PATH,INDEX_FILE);

	if ((gz = open_gz(buffer,"r")) == (gzFile)NULL)
		return (INDEX *)NULL;

	if ((index = resume_index(gz)) == (INDEX *)NULL) {
		gzclose(gz);
		return (INDEX *)NULL;
	}

	gzclose(gz);
	return index;
}

void set_maxline(const char *strnum)
{
	if (strnum == NULL)
		return ;

	char *err = NULL;
	long int terminal_max = 0;
	unsigned long int line = 0;

	terminal_max = get_terminal_size()[1];

	line = strtoul(strnum,&err,0);

	/* 有时候会下溢 */
	if ((line == ERANGE) || (line > terminal_max) || (line == 0)) {
		fprintf(stderr,\
				"W:设置行数失败 参数 %s 为无效的参数(1~%ld)\n"\
				,strnum,terminal_max);
		return ;
	}

	MAX_LINE = line;

	return ;
}

int find_mark(const char *str,char mark)
{
	CHECK_ARGUMENT(str == NULL,0);

	const char *seek = NULL;

	if (strchr(str,mark) == NULL)
		return -1;

	seek = str;
	while (seek != NULL) {
		seek = strchr(seek,mark);

		if (seek == NULL)
			return -1;
		else if ((*seek == mark) && \
				(*(seek - 1) == '\\' && *(seek - 2) == '\\'))
			break;
		else if ((*seek == mark) && *(seek - 1) != '\\')
			break;

		seek++;
	}

	return seek-str;
}

const char *del_sequence(char *str)
{
	CHECK_ARGUMENT(str == NULL,NULL);

	int i = 0,j = 0;

	if (strchr(str,'\\') == NULL)
		return str;

	for (;*(str + j) != '\0';j++) {

		if (*(str + j) == '\\' && *(str + j + 1) != '\\')
			continue;
		else if (*(str + j) == '\\' && *(str + j + 1) == '\\') {
			*(str + i++) = *(str + j);
			j++;
		} else 
			*(str + i++) = *(str + j);
	}

	*(str + i) = '\0';
	return str;
}
