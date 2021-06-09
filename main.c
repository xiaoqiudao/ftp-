#include "ftp_client.h"
#include <signal.h>

FTPClient* ftp;

void sigint(int signum)
{
	bye_FTPClient(ftp);
	exit(EXIT_SUCCESS);
}

int main(int argc,const char* argv[])
{
	ftp = create_FTPClient();
	if(2 == argc)
	{
		if(connect_FTPClient(ftp,argv[1],21))
		{
			puts("连接服务端失败，请检查ip地址");
			return EXIT_SUCCESS;
		}
	}
	else if(3 == argc)
	{
		if(connect_FTPClient(ftp,argv[1],atoi(argv[2])))
		{
			puts("连接服务端失败，请检查ip地址");
			return EXIT_SUCCESS;
		}
	}
	else
	{
		puts("Use：ftp ip [port]");
		return EXIT_SUCCESS;
	}

	char str[BUF_SIZE];
	printf("请输入用户名：");
	gets(str);
	user_FTPClient(ftp,str);
	printf("请输入密码：");
	gets(str);
	if(pass_FTPClient(ftp,str))
	{
		puts("用户名或密码错误，请重新登陆");	
		return EXIT_SUCCESS;
	}

	for(;;)
	{
		printf("ftp>");
		gets(str);
		
		if(0 == strcmp("bye",str))
		{
			bye_FTPClient(ftp);
			destroy_FTPClient(ftp);
			return EXIT_SUCCESS;
		}
		else if(0 == strcmp("pwd",str))
			pwd_FTPClient(ftp);
		else if(0 == strcmp("ls",str))
			list_FTPClient(ftp);
		else if(0 == strncmp("cd",str,2))
			cwd_FTPClient(ftp,str+3);
		else if(0 == strncmp("get",str,3))
			get_FTPClient(ftp,str+4);
		else if(0 == strncmp("put",str,3))
			put_FTPClient(ftp,str+4);
		else
			puts("未定义指令!");
	}
}
