#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <signal.h>
#include <pthread.h>
#include <poll.h>
#include <zlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <time.h>

#include <iostream>
#include <string>
#include <vector>

#include <mysql/mysql.h>
#include <sys/types.h> 
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> //inet_ntoa()函数的头文件
extern "C"
{
#include <b64/cencode.h>
#include <b64/cdecode.h>
}

#include "cJSON.h"

#define _DEBUG_

using namespace std;

#define BUFFSIZE 8188
#define JSONSIZE 8192
#define DEFAULTPORT 8001
#define FILELISTSIZE 1024
#define SIGSIZE 17 
#define USERLIMIT 5
#define SIZE 100
#define UPDATERATE 3
#define DEFAULTIP "127.0.0.1"
