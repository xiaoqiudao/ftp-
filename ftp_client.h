#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#define BUF_SIZE (100)

typedef struct sockaddr* SP;

typedef struct FTPClient
{
	char* buf;
	int sock_fd;
	int data_fd;
	int file_fd;
	char file_name[PATH_MAX];
	char client_path[PATH_MAX];
	char server_path[PATH_MAX];
	char client_mdtm[15];
	char server_mdtm[15];
	size_t server_size;
	size_t client_size;
	bool is_put;
	bool is_get;
}FTPClient;

FTPClient* create_FTPClient(void);

void destroy_FTPClient(FTPClient* ftp);

int connect_FTPClient(FTPClient* ftp,const char* ip,short port);

void send_FTPClient(FTPClient* ftp);

int recv_FTPClient(FTPClient* ftp,int check);

int user_FTPClient(FTPClient* ftp,const char* user);

int pass_FTPClient(FTPClient* ftp,const char* pass);

void cwd_FTPClient(FTPClient* ftp,const char* path);

void pwd_FTPClient(FTPClient* ftp);

void bye_FTPClient(FTPClient* ftp);

int pasv_FTPClient(FTPClient* ftp);

void list_FTPClient(FTPClient* ftp);

void put_FTPClient(FTPClient* ftp,const char* file);

void get_FTPClient(FTPClient* ftp,const char* file);

#endif//FTP_CLIENT_H
