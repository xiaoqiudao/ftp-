#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ftp_client.h"
#include "tools.h"

FTPClient* create_FTPClient(void)
{
	FTPClient* ftp = malloc(sizeof(FTPClient));
	ftp->sock_fd = socket(AF_INET,SOCK_STREAM,0);
	if(0 > ftp->sock_fd)
	{
		free(ftp);
		ERROR("socket");
		return NULL;
	}

	ftp->buf = malloc(BUF_SIZE);
	return ftp;
}

void destroy_FTPClient(FTPClient* ftp)
{
	close(ftp->sock_fd);
	close(ftp->data_fd);
	free(ftp->buf);
	free(ftp);
}

static int connect_server(int sock,const char* ip,short port)
{
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	if(connect(sock,(SP)&addr,sizeof(addr)))
	{
		ERROR("connect");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

int connect_FTPClient(FTPClient* ftp,const char* ip,short port)
{
	if(connect_server(ftp->sock_fd,ip,port))
	{
		return EXIT_FAILURE;
	}

	return recv_FTPClient(ftp,220);
}

void send_FTPClient(FTPClient* ftp)
{
	int ret = send(ftp->sock_fd,ftp->buf,strlen(ftp->buf),0);
	if(0 >= ret)
	{
		ERROR("send");
		exit(EXIT_FAILURE);
	}
	printf("bits:%d send:%s",ret,ftp->buf);
}

int recv_FTPClient(FTPClient* ftp,int check)
{
	// 清理缓冲区
	memset(ftp->buf,0,BUF_SIZE);

	int ret = recv(ftp->sock_fd,ftp->buf,BUF_SIZE,0);
	if(0 >= ret)
	{
		ERROR("recv");
		exit(EXIT_FAILURE);
	}
	
	int status = 0;
	sscanf(ftp->buf,"%d",&status);
	printf("bits:%d recv:%s",ret,ftp->buf);
	return status==check ? EXIT_SUCCESS : EXIT_FAILURE;
}

int user_FTPClient(FTPClient* ftp,const char* user)
{
	sprintf(ftp->buf,"USER %s\n",user);
	send_FTPClient(ftp);
	return recv_FTPClient(ftp,331);
}

int pass_FTPClient(FTPClient* ftp,const char* pass)
{
	sprintf(ftp->buf,"PASS %s\n",pass);
	send_FTPClient(ftp);
	if(recv_FTPClient(ftp,230)) return EXIT_FAILURE;
	
	sprintf(ftp->buf,"OPTS UTF8 ON\n");
	send_FTPClient(ftp);
	if(recv_FTPClient(ftp,200)) return EXIT_FAILURE;

	pwd_FTPClient(ftp);
	return EXIT_SUCCESS;
}

void pwd_FTPClient(FTPClient* ftp)
{
	sprintf(ftp->buf,"PWD\n");
	send_FTPClient(ftp);
	if(recv_FTPClient(ftp,257)) return;

	sscanf(ftp->buf,"%*d %*c%s",ftp->server_path);
	*strchr(ftp->server_path,'"') = '/';
}

void cwd_FTPClient(FTPClient* ftp,const char* path)
{
	sprintf(ftp->buf,"CWD %s\n",path);
	send_FTPClient(ftp);
	recv_FTPClient(ftp,250);
	pwd_FTPClient(ftp);
}

int pasv_FTPClient(FTPClient* ftp)
{
	sprintf(ftp->buf,"PASV\n");
	send_FTPClient(ftp);
	if(recv_FTPClient(ftp,227)) return EXIT_FAILURE;
	
	char ip[16]={},ip1,ip2,ip3,ip4;
	short port,port1,port2;
	
	sscanf(strchr(ftp->buf,'(')+1,
		"%hhd,%hhd,%hhd,%hhd,%hd,%hd",
		&ip1,&ip2,&ip3,&ip4,&port1,&port2);
	sprintf(ip,"%hhd.%hhd.%hhd.%hhd",ip1,ip2,ip3,ip4);
	port = port1*256+port2;

	ftp->data_fd = socket(AF_INET,SOCK_STREAM,0);
	if(0 > ftp->data_fd)
	{
		ERROR("socket");
		return EXIT_FAILURE;
	}

	return connect_server(ftp->data_fd,ip,port);
}

void list_FTPClient(FTPClient* ftp)
{
	if(pasv_FTPClient(ftp)) return;

	sprintf(ftp->buf,"LIST -al\n");
	send_FTPClient(ftp);
	if(recv_FTPClient(ftp,150)) return;

	// 接收数据
	file_io(ftp->data_fd,STDOUT_FILENO);
	close(ftp->data_fd);
	recv_FTPClient(ftp,226);
}

void put_FTPClient(FTPClient* ftp,const char* file)
{
	ftp->file_fd = open(file,O_RDONLY);
	if(0 > ftp->file_fd)
	{
		printf("%s 文件不存在，请检查文件名\n",file);
		return;
	}
	
	// 获取客户端文件的最后修改时间
	get_file_mdtm(ftp->file_fd,ftp->client_mdtm);
	// 获取客户端文件的和文件大小
	ftp->client_size = file_size(ftp->file_fd);
	
	// 备份文件名
	strcpy(ftp->file_name,file);

	// 设置传输模式
	sprintf(ftp->buf,"TYPE I\n");
	send_FTPClient(ftp);
	recv_FTPClient(ftp,220);
	
	// 获取文件大小
	sprintf(ftp->buf,"SIZE %s\n",file);
	send_FTPClient(ftp);
	if(EXIT_SUCCESS == recv_FTPClient(ftp,213))
	{
		sscanf(ftp->buf,"%*d %d",&ftp->server_size);
		// 获取文件最后修改时间
		sprintf(ftp->buf,"MDTM %s\n",file);
		send_FTPClient(ftp);
		if(recv_FTPClient(ftp,213)) return; 
		strncpy(ftp->server_mdtm,ftp->buf+4,14);
		
		// 是否是同一个
		printf("stime %s \n",ftp->server_mdtm);
		printf("ctime %s \n",ftp->client_mdtm);
		if(0 == strcmp(ftp->server_mdtm,ftp->client_mdtm) && 
			ftp->server_size < ftp->client_size)
		{
			lseek(ftp->file_fd,ftp->server_size,SEEK_END);
			sprintf(ftp->buf,"REST %d\n",ftp->server_size);
			if(0 == recv_FTPClient(ftp,350))
			{
				printf("断点续传\n");
				lseek(ftp->file_fd,ftp->server_size,SEEK_SET);
			}
		}	
	}
	
	// 开启PAVS模式
	if(pasv_FTPClient(ftp))
	{
		close(ftp->file_fd);
		return;
	}

	// 下载文件
	sprintf(ftp->buf,"STOR %s\n",file);
	send_FTPClient(ftp);
	if(recv_FTPClient(ftp,150))
	{
		close(ftp->file_fd);
		close(ftp->data_fd);
		return;
	}
	// 接收文件
	ftp->is_get = true;
	file_io(ftp->file_fd,ftp->data_fd);
	close(ftp->file_fd),close(ftp->data_fd);
	recv_FTPClient(ftp,226);
	sprintf(ftp->buf,"MDTM %s %s/%s\n",ftp->client_mdtm,ftp->server_path,file);
	send_FTPClient(ftp);
	recv_FTPClient(ftp,213);
	ftp->is_get = false;
}

void get_FTPClient(FTPClient* ftp,const char* file)
{
	// 备份文件名
	strcpy(ftp->file_name,file);

	// 设置传输模式
	sprintf(ftp->buf,"TYPE I\n");
	send_FTPClient(ftp);
	recv_FTPClient(ftp,220);
	
	// 获取文件大小
	sprintf(ftp->buf,"SIZE %s\n",file);
	send_FTPClient(ftp);
	if(recv_FTPClient(ftp,213)) return;
	sscanf(ftp->buf,"%*d %d",&ftp->server_size);

	// 获取文件最后修改时间
	sprintf(ftp->buf,"MDTM %s\n",file);
	send_FTPClient(ftp);
	if(recv_FTPClient(ftp,213)) return; 
	strncpy(ftp->server_mdtm,ftp->buf+4,14);

	ftp->file_fd = open(file,O_WRONLY);
	if(0 < ftp->file_fd)
	{
		// 获取客户端文件的最后修改时间
		get_file_mdtm(ftp->file_fd,ftp->client_mdtm);
		// 获取客户端文件的和文件大小
		ftp->client_size = file_size(ftp->file_fd);
		
		// 是否是同一个
		printf("stime %s \n",ftp->server_mdtm);
		printf("ctime %s \n",ftp->client_mdtm);
		
		if(0 == strcmp(ftp->server_mdtm,ftp->client_mdtm))
		{
			if(ftp->server_size > ftp->client_size)
			{
				lseek(ftp->file_fd,0,SEEK_END);
				sprintf(ftp->buf,"REST %d\n",ftp->client_size);
				send_FTPClient(ftp);
				if(0 == recv_FTPClient(ftp,350))
				{
					printf("断点续传\n");
					lseek(ftp->file_fd,0,SEEK_SET);
				}
			}
			else
			{
				printf("%s 已经下载完成，请不要重复下载！\n",file);
				close(ftp->file_fd);
				return;
			}
		}
		else
		{
			// 是否覆盖
			printf("当前目录下已有同名文件，是否放弃下载(y/n)?");
			if(yes_or_no())
			{
				close(ftp->file_fd);
				return;	
			}
			ftruncate(ftp->file_fd,ftp->server_size);
		}
	}
	else
	{
		ftp->file_fd = open(file,O_WRONLY|O_CREAT,0644);
		if(0 > ftp->file_fd)
		{	
			ERROR("open");
			return;
		}	
	}
	
	// 开启PAVS模式
	if(pasv_FTPClient(ftp))
	{
		close(ftp->file_fd);
		return;
	}

	// 下载文件
	sprintf(ftp->buf,"RETR %s\n",file);
	send_FTPClient(ftp);
	if(recv_FTPClient(ftp,150))
	{
		close(ftp->file_fd);
		close(ftp->data_fd);
		return;
	}
	
	
	// 接收文件
	ftp->is_get = true;
	file_io(ftp->data_fd,ftp->file_fd);
	close(ftp->file_fd),close(ftp->data_fd);
	recv_FTPClient(ftp,226);
	set_file_mdtm(ftp->file_name,ftp->server_mdtm);
	ftp->is_get = false;
}

void bye_FTPClient(FTPClient* ftp)
{
	if(ftp->is_get)
	{
		close(ftp->file_fd),close(ftp->data_fd);
		recv_FTPClient(ftp,226);
		set_file_mdtm(ftp->file_name,ftp->server_mdtm);
	}
	
	if(ftp->is_put)
	{
		close(ftp->file_fd),close(ftp->data_fd);
		recv_FTPClient(ftp,226);
		sprintf(ftp->buf,"MDTM %s %s/%s\n",ftp->client_mdtm,ftp->server_path,ftp->file_name);
		send_FTPClient(ftp);
		recv_FTPClient(ftp,213);
	}
	
	sprintf(ftp->buf,"QUIT\n");
	send_FTPClient(ftp);
	recv_FTPClient(ftp,221);
}
