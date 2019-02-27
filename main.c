#include "base.h"

static const struct option long_opt[] = {
	{"index",optional_argument,NULL,'i'},
	{"pack",required_argument,NULL,'z'},
	{"updata",no_argument,NULL,'u'},
	{"path",required_argument,NULL,'d'},
	{"line",required_argument,NULL,'n'},
	{"ignore",no_argument,NULL,'l'},
	{"file",no_argument,NULL,'f'},
/*	{"test",no_argument,NULL,'t'}, */
	{"help",no_argument,NULL,'h'},
	{"version",no_argument,NULL,'v'},
	{NULL,0,NULL,0}
	
};

static const char *short_opt = "i::z:ud:n:lfhv";

const char *OPTARGU[4] = {NULL};
/* 存储带参数选项的参数指针 */

enum OPTARGU_INDEX {
	/* 参数选项在OPTARGU 中的下标 */
	index_index = 0,
	pack_index,
	path_index,
	line_index,
};

static enum ENABLE_OPT {
	/* 需要执行实际操作选项的掩码 */
	INDEX_MASK  = 1,
	PACK_MASK   = INDEX_MASK * 2,
	UPDATE_MASK = PACK_MASK  * 2,
} opt_mask = 0;

static int help(int status)
{
	if (status) {
		fprintf(stderr,"%s:试试 %s --help 获取帮助\n",PRONAME,PRONAME);
	} else {
		printf("Usage:%s [选项] [<关键字...>]\n"\
				"主选项:\n"\
				"-i,--index[=关键字]   从目录中搜索[关键字],\
(缺省关键字输出完整的目录)\n"\
				"-z,--pack <数据文件>  将程序的数据文件进行压缩/解压\n"\
				"-u,--update           更新索引文件中的数据\n"\
				"辅助选项:\n"\
				"-d,--path <数据目录>  手动指定数据文件位置\n"\
				"-n,--line <行数>      指定目录浏览的行数\n"\
				"-l,--ignore           搜索的时候忽略大小写\n"\
				"-f,--file             显示数据文件详情\n"\
				/*"-t,--test           测试的功能\n"\*/
				"-h,--help             显示这个帮助然后退出\n"\
				"-v,--version          显示程序版本然后退出\n"\
				,PRONAME);
	}

	return status;
}

static void version(void)
{
	sro_printf("#B2S#B3h#B4i#B5r#B6o#B0\n");
#ifdef FSP_VERSION
	sro_printf("%s Version #F3%s#F0\n",PRONAME,FSP_VERSION);
#else
	sro_printf("%s Version (#F2Not defined#F0)\n",PRONAME);
#endif

	return ;
}


int main(int argc,char *argv[])
{
	setlocale(LC_ALL,"");
	SET_PRONAME(argv[0]);
	SUPPORT_COLOR;

	int opt = 0,retc = 0;

	if (argc < 2)
		return (help(EXIT_FAILURE));

	while((opt = getopt_long(argc,argv,short_opt,long_opt,NULL)) != -1) {
		switch (opt) {
			case 'i':
				opt_mask |= INDEX_MASK;
				if (optarg != NULL)
					OPTARGU[index_index] = optarg;
				else
					OPTARGU[index_index] = NULL;
				break;
			case 'z':
				opt_mask |= PACK_MASK;
				OPTARGU[pack_index] = optarg;
				break;
			case 'u':
				opt_mask |= UPDATE_MASK;
				break;
			case 'd':
				OPTARGU[path_index] = optarg;
				break;
			case 'n':
				set_maxline(optarg);
				OPTARGU[line_index] = optarg;
				break;
			case 'l':
				IGNORE_CASE = true;
				break;
			case 'f':
				if (set_datapath(OPTARGU[path_index]) != 0)
					return EXIT_FAILURE;

				if ((HEAD = open_index()) == (INDEX *)NULL)
					return EXIT_FAILURE;
				retc = print_info(HEAD);
				index_recovery(HEAD);
				return retc;
				break;
			/*
			case 't':
				if (set_datapath(OPTARGU[path_index]) != 0)
					return EXIT_FAILURE;

				if ((index = open_index()) == (INDEX *)NULL)
					return EXIT_FAILURE;

				input_search(index);
				index_recovery(index);
				return 0;
				break;
			*/
			case 'h':
				return help(EXIT_SUCCESS);
				break;
			case 'v':
				version();
				return 0;
				break;
			default:
				return help(EXIT_FAILURE);
				break;
		}
	}

	if (opt_mask == 0) {

		if (set_datapath(OPTARGU[path_index]) != 0)
			return EXIT_FAILURE;

		if ((HEAD = open_index()) == (INDEX *)NULL)
			return EXIT_FAILURE;

		retc = search_argv(HEAD,argv);

		index_recovery(HEAD);

	} else {

		if ((opt_mask & INDEX_MASK) == INDEX_MASK) {

			if (set_datapath(OPTARGU[path_index]) != 0)
				return EXIT_FAILURE;

			if ((HEAD = open_index()) == (INDEX *)NULL)
				return EXIT_FAILURE;

			if (OPTARGU[index_index] == NULL)
				retc = browse_index(HEAD);
			else
				retc = search_index(HEAD,OPTARGU[index_index]);

			index_recovery(HEAD);

		} else if ((opt_mask & PACK_MASK) == PACK_MASK) {
			retc = pack(OPTARGU[pack_index]);
		} else if ((opt_mask & UPDATE_MASK) == UPDATE_MASK) {

			if (set_datapath(OPTARGU[path_index]) != 0)
				return EXIT_FAILURE;
			retc = update_index();

		}
	}

	return retc;
}
