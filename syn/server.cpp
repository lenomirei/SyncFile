#include "head.h"
#include "Compress.h"
#include "code.h"
#include "FileTransport.h"


vector<int> lock;


struct netneed
{
  int sockfd;
  struct sockaddr_in server_addr;
};







struct FileList
{
  FileList()
    :size(0)
  {
    FilePath.resize(10);
    FileSize.resize(10);
  }
  int Add(const char *filepath,long long filesize)
  {
    printf("the filepath is %s\n",filepath);

    for(int i=0;i<size;++i)
    {
      printf("the FileList is %s\n",FilePath[i].c_str());
      if(strcmp(FilePath[i].c_str(),filepath)==0)
      {
        while(lock[i]==1)
        {
          sleep(2);
          printf("someon is using this file ,I must wait for unlock%d\n",i);
        }
        lock[i]=1;

        printf("I get the lock%d\n",i);

        return i;
      }
    }
    FilePath[size]=filepath;
    FileSize[size]=filesize;
    lock[size]=1;
    size++;
    return (size-1);
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
    printf("bind error %d:%s\n ",errno,strerror(errno));
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




FileList *Serverfl=new FileList;

void* mainstream(void *net)
{
  struct netneed *tmp=(struct netneed *)(net);
  int sockfd=tmp->sockfd;
  struct sockaddr_in server_addr=tmp->server_addr;

  int sockConn=ConnectToClient(sockfd,server_addr);



  //this is a test
  //int filefd=open("./SyncFloderServer/test2.txt",O_RDONLY);
  //Serverfl->Add("./SyncFloderServer/test2.txt",0);
 // filefd=open("./SyncFloderServer/test.pdf",O_RDONLY);
 // Serverfl->Add("./SyncFloderServer/test.pdf",0);
  //filefd=open("./SyncFloderServer/test6.txt",O_RDONLY);
  //Serverfl->Add("./SyncFloderServer/test6.txt",0);
  //filefd=open("./SyncFloderServer/test7.txt",O_RDONLY);
  //Serverfl->Add("./SyncFloderServer/test7.txt",0);
  
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
        if(strstr(Serverfl->FilePath[i].c_str(),".txt")==NULL)
        {
          BinaryFileEncode(Serverfl->FilePath[i].c_str(),sockConn);
#ifdef _DEBUG_
          clock_t start=clock();
#endif
          UploadFile("encodedfile",sockConn);
#ifdef _DEBUG_
          clock_t end=clock();
          cout<<"transport time count is"<<((double)(end-start))/CLOCKS_PER_SEC<<endl;
#endif
        }
        else
        {

          CompressFile(Serverfl->FilePath[i].c_str());
#ifdef _DEBUG_
          clock_t start=clock();
#endif
          UploadFile("compressedfile.gz",sockConn);
#ifdef _DEBUG_
          clock_t end=clock();
          cout<<"transport time count is"<<((double)(end-start))/CLOCKS_PER_SEC<<endl;
#endif

        }

       // sleep(10) ;
      }


    }
    else if(sig==3)//client add
    {
      char fileinforecv[BUFFSIZE]={'\0'};
      
      recv(sockConn,fileinforecv,BUFFSIZE,0);
#ifdef _DEBUG_
      printf("the filepath is %s\n",fileinforecv);
#endif

      cJSON *fileinfo=cJSON_Parse(fileinforecv);
      char *filepath=cJSON_GetObjectItem(fileinfo,"filepath")->valuestring;
      int filesize=cJSON_GetObjectItem(fileinfo,"filesize")->valueint;
      int index=Serverfl->Add(filepath,filesize);

      send(sockConn,"you can send file,i am ready",JSONSIZE,0);
      DownloadFile(filepath,sockConn);
      
      //unlock
      lock[index]=0;
#ifdef _DEBUG_

      printf("unlock!%d\n",index);
#endif


    }
    else if(sig == 4)//client delete
    {
      char filepath[JSONSIZE]={'\0'};
      recv(sockConn,filepath,JSONSIZE,0);
#ifdef _DEBUG_
      printf("the filepath is %s\n",filepath);


      printf("before delete %d\n",Serverfl->size);
#endif

      Serverfl->Delete(filepath);
#ifdef _DEBUG_
      printf("after delete %d\n",Serverfl->size);
#endif

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

  //init lock
  lock.resize(10);


  //try to connect client
  struct sockaddr_in server_addr;
  int sockfd;
  sockfd=BindSocket("192.168.84.132",DEFAULTPORT,server_addr);
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
