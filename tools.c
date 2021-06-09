#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <getch.h>
#include "tools.h"

size_t file_size(int fd)
{
	return lseek(fd,0,SEEK_END);
}

void file_io(int ifd,int ofd)
{
	int ret = 0;
	char buf[4096] = {};
	while((ret = read(ifd,buf,sizeof(buf))))
	{
		write(ofd,buf,ret);
	}
}

char* get_file_mdtm(int fd,char* mdtm)
{
	struct stat buf;
	fstat(fd,&buf);
	struct tm* tm = localtime(&buf.st_mtime);
	sprintf(mdtm,"%04d%02d%02d%02d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	return mdtm;
}

void set_file_mdtm(const char* file,char* mdtm)
{
	struct tm tm = {};
	sscanf(mdtm,"%4d%2d%2d%2d%2d%2d",&tm.tm_year,&tm.tm_mon,&tm.tm_mday,&tm.tm_hour,&tm.tm_min,&tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon -= 1;
	struct utimbuf utimbuf;
	utimbuf.actime = time(NULL);
	utimbuf.modtime = mktime(&tm);
	if(utime(file,&utimbuf))
		printf("修改失败!\n");
	else
		printf("修改成功!\n");
}

bool yes_or_no(void)
{
	for(;;)
	{
		char cmd = getch();
		if('y' == cmd || 'Y' == cmd)
		{
			printf("Yes\n");
			return true;
		}
		if('n' == cmd || 'N' == cmd)
		{
			printf("No\n");
			return false;
		}
	}
}
