.PHONY:all
all:server.cpp cJSON.c
	g++ -o server server.cpp cJSON.c -L. -lb64 -pthread -lz -lmysqlclient -I/usr/include/mysql/ -L/usr/lib64/mysql
	g++ -o usermanage usermanage.cpp -lmysqlclient -I/usr/include/mysql/ -L/usr/lib64/mysql
.PHONY:clean
clean:
	rm -f server usermanage
