# Fsp
查找函数描述的一个小工具

对该程序进行编译的时候您需要确保已经安装zlib1g-dev(debian)这个软件包和libshiro
执行的时候如果提示“找不到FSP环境变量"的话，请到 ~/.bashrc 中添加 exprot FSP=<该程序的data目录>
example:
export FSP="/home/ShiroMea/fsp/data"
