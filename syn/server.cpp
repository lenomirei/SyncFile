#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> //inet_ntoa()函数的头文件
#include <unistd.h>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <signal.h>
#include <pthread.h>
#include <poll.h>



#include "cJSON.h"





using namespace std;



#define BUFFSIZE 512
#define JSONSIZE 2048
#define DEFAULTPORT 8001
#define FILELISTSIZE 1024
#define SIGSIZE 17 
#define USERLIMIT 5



struct netneed
{
  int sockfd;
  struct sockaddr_in server_addr;
};
int FileTransport(const char * filepath,int sockConn)
{

  printf("tranporting file%s\n",filepath);

  int filefd=open(filepath,O_RDONLY);
  if(filefd<0)
  {
    printf("file open error!\n");
    return -1;
  }
  char sendBuf[JSONSIZE]={'\0'};
  char jsonbuf[JSONSIZE]={'\0'};
  while(1)
  {
    char *sendjson;
    cJSON *transgram=cJSON_CreateObject();

    int len = read(filefd,sendBuf,BUFFSIZE);

    cJSON_AddItemToObject(transgram,"length",cJSON_CreateNumber(len));
    cJSON_AddItemToObject(transgram,"datapack",cJSON_CreateString(sendBuf));
    sendjson=cJSON_Print(transgram);

    //printf("the length of sendjson is %d\n",strlen(sendjson)+1);
    //printf("WTF the json is %s \n",sendjson);
    strcpy(jsonbuf,sendjson);


    int temp=send(sockConn,jsonbuf,JSONSIZE,0);


    recv(sockConn,sendBuf,JSONSIZE,0);


    memset(sendBuf,'\0',JSONSIZE);
    if(len < 1)
    {
      break;
    }
  }
  return 0;
}
int ReceiveFile(char *filepath,int clientfd)
{
 
  printf("downloading file %s......\n",filepath);

  

  char recvBuf[JSONSIZE]={'\0'};
  int filefd;
  long long totallength = 0;
  filefd = open(filepath,O_RDWR|O_CREAT,0777);
  while(1)
  {

    cJSON *transgram;

    int test=recv(clientfd,recvBuf,JSONSIZE,0);


    send(clientfd,"acknowledge",JSONSIZE,0);



    transgram=cJSON_Parse(recvBuf);
    int len=cJSON_GetObjectItem(transgram,"length")->valueint;

    if(len==0)
    {
      break;
    }
    write(filefd ,cJSON_GetObjectItem(transgram,"datapack")->valuestring ,len);
    memset(recvBuf,'\0',JSONSIZE);
    totallength+=len;
  }
  int num=lseek(filefd,0,SEEK_END);
 ftruncate(filefd,totallength);
}


void SendFileList(int sockConn,struct FileList& fl)
{
  char *data=NULL;
  char sendfilelistjson[FILELISTSIZE]={'\0'};

  cJSON *filepath=cJSON_CreateArray();
  cJSON *filesize=cJSON_CreateArray();
  for(int i=0;i<fl.size;++i)
  {
    cJSON_AddItemToArray(filepath,cJSON_CreateString(fl.FilePath[i].c_str()));
  }
  data=cJSON_Print(filepath);

  strcpy(sendfilelistjson,data);

  int tem=send(sockConn,sendfilelistjson,FILELISTSIZE,0);



  recv(sockConn,sendfilelistjson,FILELISTSIZE,0);//ackowledge receive

  memset(sendfilelistjson,'\0',FILELISTSIZE);

  char *size;  
  for(int i=0;i<fl.size;++i)
  {
    cJSON_AddItemToArray(filesize,cJSON_CreateNumber(fl.FileSize[i]));
  }
  size=cJSON_Print(filesize);

  strcpy(sendfilelistjson,size);

  send(sockConn,sendfilelistjson,FILELISTSIZE,0);
}


struct FileList
{
  FileList()
    :size(0)
  {
    FilePath.resize(10);
    FileSize.resize(10);
  }
  void Add(const char *filepath,long long filesize)
  {
    FilePath[size]=filepath;
    FileSize[size]=filesize;
    size++;
  }
  void Delete(const char *filepath)
  {


    vector<string>::iterator it;
    vector<long long>::iterator itsize;
    it = FilePath.begin();
    itsize=FileSize.begin();
    for(;it!=FilePath.end();)
    {
      printf("%s   ",it->c_str());
      printf("%d\n",*itsize);
      if(*it==filepath)
      {
        it=FilePath.erase(it);
        itsize=FileSize.erase(itsize);
        break;
      }
      else
      {
        it++;
        itsize++;
      }
    }
    size--;
  }
  void change(const char *filepath,long long filesize)
  {
  }
  bool operator==(FileList &fl)
  {
    if(size!=fl.size)
    {
      return false;
    }
    for(int i=0;i<size;++i)
    {
      if(FilePath[i]!= fl.FilePath[i] || FileSize[i]!=fl.FileSize[i] )
      {
        return false;
      }
    }
    return true;
  }
    vector<string> FilePath;
    vector<long long> FileSize;
    size_t size;
};


int BindSocket(const char *address,int port,struct sockaddr_in& server_addr)
{
  int sockfd;
  bzero(&server_addr,sizeof(sockaddr_in));
  server_addr.sin_addr.s_addr=inet_addr(address);
  server_addr.sin_port=htons(port);
  server_addr.sin_family=AF_INET;
  if((sockfd=socket(PF_INET,SOCK_STREAM,0)) == -1)
  {
    printf("socket error");
    return -1;
  }
  if(bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr)) == -1)
  {
    printf("bind error ");
    return -1;
  }
  if(listen(sockfd,5) == -1)
  {
    printf("listen error");
    return -1;
  }
  return sockfd;
}


int ConnectToClient(int sockfd,struct sockaddr_in& server_addr)
{
  socklen_t len;
  int clientfd;
  clientfd=accept(sockfd,(struct sockaddr *)(&server_addr),&len);
  if(clientfd == -1)
  {
    printf("accept error%d:%s\n",errno,strerror(errno));
    return -1;
  }
  printf("connect success\n");
  return clientfd;
}


void ShutDownConnect(int clientfd)
{
  close(clientfd);
}

void CloseSocket(int sockfd)
{
  close(sockfd);
}





void* mainstream(void *net)
{
  struct netneed *tmp=(struct netneed *)(net);
  int sockfd=tmp->sockfd;
  struct sockaddr_in server_addr=tmp->server_addr;

  int sockConn=ConnectToClient(sockfd,server_addr);



  FileList *Serverfl=new FileList;
  //this is a test
  int filefd=open("./SyncFloderServer/test2.txt",O_RDONLY);
  Serverfl->Add("./SyncFloderServer/test2.txt",0);
  filefd=open("./SyncFloderServer/test6.txt",O_RDONLY);
  Serverfl->Add("./SyncFloderServer/test6.txt",0);
  filefd=open("./SyncFloderServer/test.pdf",O_RDONLY);
  Serverfl->Add("./SyncFloderServer/test.pdf",0);
  char signals[SIGSIZE]={'\0'};  
  //test is over
  
  //try to connect client
  //struct sockaddr_in server_addr;
  //int sockfd;
  //sockfd=BindSocket("192.168.84.128",DEFAULTPORT,server_addr);
  //int sockConn=ConnectToClient(sockfd,server_addr);


  while(1)
  {
    printf("wating for signals......\n\n");
    recv(sockConn,signals,SIGSIZE,0);



    cJSON *root=cJSON_Parse(signals);
    memset(signals,'\0',SIGSIZE);
    int sig=cJSON_GetObjectItem(root,"signal")->valueint;

  
    printf("recv signal success:%d\n",sig);

    if(sig==1)
    {
      //request for filelist
      printf("sync file list......\n");
      SendFileList(sockConn,*Serverfl);
      printf("sync filelist complete\n");
    }
    else if(sig==2)
    {
      //requesr for downloadfile
      for(int i=0;i<Serverfl->size;++i)
      {
        FileTransport(Serverfl->FilePath[i].c_str(),sockConn);

       // sleep(10) ;
      }


    }
    else if(sig==3)
    {
      char fileinforecv[BUFFSIZE]={'\0'};
      
      recv(sockConn,fileinforecv,BUFFSIZE,0);

      printf("the filepath is %s\n",fileinforecv);


      cJSON *fileinfo=cJSON_Parse(fileinforecv);
      char *filepath=cJSON_GetObjectItem(fileinfo,"filepath")->valuestring;
      int filesize=cJSON_GetObjectItem(fileinfo,"filesize")->valueint;
      Serverfl->Add(filepath,filesize);


      ReceiveFile(filepath,sockConn);



    }
    else if(sig == 4)
    {
      char filepath[JSONSIZE]={'\0'};
      recv(sockConn,filepath,JSONSIZE,0);

      printf("the filepath is %s\n",filepath);


      printf("before delete %d\n",Serverfl->size);
      Serverfl->Delete(filepath);
      printf("after delete %d\n",Serverfl->size);
      

      printf("__filelist delete__\n");

      remove(filepath);

      printf("delete complete\n");

    }
    else if(sig==5)
    {
      ShutDownConnect(sockConn);
      pthread_exit((void*)1);
    }
  }
}





int main()
{

  //try to connect client
  struct sockaddr_in server_addr;
  int sockfd;
  sockfd=BindSocket("192.168.84.128",DEFAULTPORT,server_addr);
  struct netneed net;
  net.sockfd=sockfd;
  net.server_addr=server_addr;

  

  struct pollfd fds[1];
  fds[0].fd=sockfd;
  fds[0].events=POLLIN | POLLERR;
  fds[0].revents=0;


  pthread_t *tid=new pthread_t[USERLIMIT];
  int usercount=0;


  printf("waiting for clients......\n");

  while(1)
  {
    int ret;
    ret=poll(fds,1,-1);
    if(ret<0)
    {
      printf("poll error\n");
      break;
    }
    if(fds[0].revents & POLLIN)
    {

      pthread_create(&tid[usercount],NULL,mainstream,&(net));
      usercount++;
    }
  }


  CloseSocket(sockfd);
  return 0;
}
