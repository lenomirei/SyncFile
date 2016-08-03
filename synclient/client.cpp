#include "head.h"
#include "code.h"
#include "FileTransport.h"
#include "Compress.h"




int ConnectToServer(const char *address,int port,struct sockaddr_in& server_addr)
{
  printf("Connecting to Server......\n");
  server_addr.sin_addr.s_addr=inet_addr(address);
  server_addr.sin_port=htons(port);
  server_addr.sin_family=AF_INET;
  int sockfd=socket(PF_INET,SOCK_STREAM,0);
  if(sockfd<0)
  {
    printf("socket error!\n");
    return -1;
 
  }
  if(connect(sockfd,(struct sockaddr*)(&server_addr),sizeof(struct sockaddr)) == -1)
  {
    printf("connect error!\n");
    return -1;
  }
  printf("connect success!\n");
  return sockfd;
}
void ShutDownConnect(int sockConn)
{
  close(sockConn);
}



void SendSignal(int sig,int sockConn)
{

      char *signals;
      cJSON *root=cJSON_CreateObject();
      cJSON_AddItemToObject(root,"signal",cJSON_CreateNumber(sig));
      signals=cJSON_Print(root);
      send(sockConn,signals,SIGSIZE,0);
}





struct FileList
{
  FileList()
    :size(0)
  {

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


class Sync
{
public:
  Sync()
  {
    InitLocalfl();
    InitServerfl();
  }
  void SetsockConn(int sockConn)
  {
    this->sockConn=sockConn;
  }
  void InitServerfl()
  {
    Serverfl.FilePath.resize(10);
    Serverfl.FileSize.resize(10);
  }
  void InitLocalfl()
  {
    mkdir("SyncFloderServer",0777);
    Localfl.FilePath.resize(10);
    Localfl.FileSize.resize(10);

  }
  void RequestServerfl(int sockConn)
  {
    //char *signals;
    //cJSON *root=cJSON_CreateObject();
    //cJSON_AddItemToObject(root,"signal",cJSON_CreateNumber(1));
    //signals=cJSON_Print(root);
    //send(sockConn,signals,SIGSIZE,0);
    SendSignal(1,sockConn);

    char data[FILELISTSIZE]={'\0'};

    int len=recv(sockConn,data,FILELISTSIZE,0);


    send(sockConn,data,FILELISTSIZE,0);//acknowledge send
    
    
    cJSON *filepath=cJSON_Parse(data);

    int sizeofarray=cJSON_GetArraySize(filepath);


    Serverfl.size=sizeofarray;

    for(int i=0;i<sizeofarray;++i)
    {
      char *temp=cJSON_GetArrayItem(filepath,i)->valuestring;
      Serverfl.FilePath[i]=*(new string(temp));
    }


    memset(data,'\0',FILELISTSIZE);
    len=recv(sockConn,data,FILELISTSIZE,0);


    cJSON *filesize=cJSON_Parse(data);
    for(int i=0;i<sizeofarray;++i)
    {
      int temp=cJSON_GetArrayItem(filesize,i)->valueint;
      Serverfl.FileSize[i]=temp;

    }
  }
  void FileSync()//专门认为是从服务器端下载到客户端
  {
    printf("syncing filelist......\n");
    RequestServerfl(sockConn);
    printf("filelist recvive complete!\n");
    //printf("serverfl size is %d\n",Serverfl.size);
    //printf("localfl size is %d\n",Localfl.size);


    if(!(Localfl == Serverfl))
    {
      //need to sync
      
      SendSignal(2,sockConn);
      printf("need to sync\n");
      printf("___sync strart___\n");


      for(int i=0;i<Serverfl.size;++i)
      {


          if(strstr(Serverfl.FilePath[i].c_str(),".txt")==NULL)
          {
            DownloadFile("encodedfile",sockConn);
            BinaryFileDecode(Serverfl.FilePath[i].c_str());
          }
          else
          {
            DownloadFile("compressedfile.gz",sockConn);
            UnCompressFile(Serverfl.FilePath[i].c_str());
          }


          printf("download success!next one\n");

          Localfl.FilePath[i]=Serverfl.FilePath[i];
          Localfl.FileSize[i]=Serverfl.FileSize[i]; 
      }
      Localfl.size=Serverfl.size;


      printf("___sync end___\n");
    }
    //no need to sync
    else
    {
      //do nothing
      printf("I don't need to sync ,I choose to do nothing\n");
    }
    SendSignal(5,sockConn);
  }
  void FileUpdate(const char *filepath,int filesize)//专门认为是从客户端上传到服务器端
  {
    //add or delete or change will do this
    
    SendSignal(3,sockConn);
    Localfl.Add(filepath,filesize);
  

    cJSON *fileinfo=cJSON_CreateObject();
    cJSON_AddItemToObject(fileinfo,"filepath",cJSON_CreateString(filepath));
    cJSON_AddItemToObject(fileinfo,"filesize",cJSON_CreateNumber(filesize));
    char *fileinfosend=cJSON_Print(fileinfo);
    send(sockConn,fileinfosend,BUFFSIZE,0);


    printf("I should zuse here\n");
    recv(sockConn,NULL,JSONSIZE,0);
    printf("I am right!\n");

   

    UploadFile(filepath,sockConn);
  }

  void SyncAdd(const char *filepath,long long filesize)
  {
    struct sockaddr_in server_addr;
    int sockConn=ConnectToServer("192.168.84.132",DEFAULTPORT,server_addr);
    SetsockConn(sockConn);
    //char test[JSONSIZE]={'\0'};
    Localfl.Add(filepath,filesize);


    FileUpdate(filepath,filesize);
    SendSignal(5,sockConn);
  }
  void SyncDelete(const char *filepath)
  {
    //char *signals;
    //cJSON *root=cJSON_CreateObject();
    //cJSON_AddItemToObject(root,"signal",cJSON_CreateNumber(4));
    //signals=cJSON_Print(root);
    //send(sockConn,signals,SIGSIZE,0);
    SendSignal(4,sockConn);

    Localfl.Delete(filepath);
    send(sockConn,filepath,JSONSIZE,0);
  }
  void SyncChange();

private:
  int sockConn;
  FileList Localfl;
  FileList Serverfl;
};





Sync *cli=new Sync();

void filesync(int num)
{
  struct sockaddr_in server_addr;
  int sockConn=ConnectToServer("192.168.84.132",DEFAULTPORT,server_addr);
  cli->SetsockConn(sockConn);
  cli->FileSync();
  ShutDownConnect(sockConn);
  alarm(UPDATERATE);
}


void mainstream()
{

  
  //struct sockaddr_in server_addr;
  //int sockConn=ConnectToServer("192.168.84.132",DEFAULTPORT,server_addr);
  //cli->SetsockConn(sockConn);
  cli->SyncAdd("./SyncFloderServer/test1.txt",0);
  cli->SyncAdd("./SyncFloderServer/test2.txt",0);
  cli->SyncAdd("./SyncFloderServer/test6.txt",0);
  cli->SyncAdd("./SyncFloderServer/test7.txt",0);
  
  //cli->SyncDelete("./SyncFloderServer/test1.txt");

  signal(SIGALRM,filesync);
  alarm(UPDATERATE);
  //char command[1024]={'\0'};
  while(1)
  {
    //sleep(UPDATERATE);
  }
}




int main()
{
  mainstream();
  return 0;
}

