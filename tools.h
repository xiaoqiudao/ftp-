#ifndef TOOLS_H
#define TOOLS_H
#include <stdbool.h>

#define ERROR(func) printf("line:%d func:%s %s:%m\n",__LINE__,__func__,func)

size_t file_size(int fd);

void file_io(int ifd,int ofd);

char* get_file_mdtm(int fd,char* mdtm);

void set_file_mdtm(const char* file,char* mdtm);

bool yes_or_no(void);

#endif//TOOLS_H
