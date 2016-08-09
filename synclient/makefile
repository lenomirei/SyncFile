server:client.cpp cJSON.c
	g++ -o client client.cpp cJSON.c -L. -lb64 -pthread -lz -lmysqlclient -I/usr/include/mysql/ -L/usr/lib64/mysql
.PHONY:clean
clean:
	rm -f client log
