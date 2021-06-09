CC=gcc
STD=-std=gnu99
FLAG=-Wall -Werror
OBJS=main.o ftp_client.o tools.o
TARGE=ftp

all:$(OBJS)
	$(CC) $(OBJS) -o $(TARGE) && ./$(TARGE) 127.0.0.1

%.o:%.c
	$(CC) $(FLAG) $(STD) -c $< -o $@
	
clean:
	rm -rf $(OBJS) $(TARGE)
	rm -rf *.h.gch
