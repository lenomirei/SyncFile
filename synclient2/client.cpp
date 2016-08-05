#include "head.h"
#include "code.h"
#include "FileTransport.h"
#include "Compress.h"
#include "user.h"



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
    mkdir("S",0777);
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


    recv(sockConn,NULL,JSONSIZE,0);

   
    if(strstr(filepath,".txt")==NULL)
    {
      BinaryFileEncode(filepath,sockConn);
      UploadFile("encodedfile",sockConn);
    }
    else
    {
      CompressFile(filepath);
      UploadFile("compressedfile.gz",sockConn);
    }
  }

  void Connect()
  {

    struct sockaddr_in server_addr;
    int sockConn=ConnectToServer(DEFAULTIP,DEFAULTPORT,server_addr);
    if(sockConn<0)
    {
      printf("connect error exit after 3 seconds\n");
      sleep(3);
      exit(-1);
    }
    SetsockConn(sockConn);
  }
  void SyncAdd(const char *filepath,long long filesize)
  {
    Connect();
    //char test[JSONSIZE]={'\0'};
    Localfl.Add(filepath,filesize);


    FileUpdate(filepath,filesize);
    SendSignal(5,sockConn);
  }
  void SyncDelete(const char *filepath)
  {
    Connect();
    SendSignal(4,sockConn);

    Localfl.Delete(filepath);
    send(sockConn,filepath,JSONSIZE,0);
    SendSignal(5,sockConn);
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
  int sockConn=ConnectToServer(DEFAULTIP,DEFAULTPORT,server_addr);
  cli->SetsockConn(sockConn);
  cli->FileSync();
  ShutDownConnect(sockConn);
  alarm(UPDATERATE);
}


void mainstream()
{
  char username[21]={'\0'};
  char userpassword[21]={'\0'};
  char *userinfo;
  cout<<"please input username"<<endl;
  cin>>username;
  cout<<"please input userpassword"<<endl;
  cin>>userpassword;
  while(UserCheck(username,userpassword)!=0)
  {
    system("clear");
    printf("username or password is wrong ,please check in\n");
    cout<<"please input username"<<endl;
    cin>>username;
    cout<<"please input userpassword"<<endl;
    cin>>userpassword;
  }
  system("clear");
  printf("login success!\n");
  //cli->SyncAdd("./SyncFloderServer/test1.txt",0);
  //cli->SyncAdd("./SyncFloderServer/test2.txt",0);
  //cli->SyncAdd("./SyncFloderServer/test6.txt",0);
  //cli->SyncAdd("./SyncFloderServer/test7.txt",0);
  //cli->SyncAdd("./SyncFloderServer/test.pdf",0);
  //cli->SyncDelete("./SyncFloderServer/test1.txt");

  char command[100]={'\0'};
  char selection='1';
  while(selection!=4)
  {
    switch(selection)
    {
      case '1':
        sigset_t s,o;
        sigemptyset(&s);
        sigemptyset(&o);
        sigaddset(&s,SIGALRM);
        sigprocmask(SIG_SETMASK,&s,&o);
        printf("please input command\n");
        cin>>command;

        char *temp;
        temp=strtok(command,",");
        if(strcmp(temp,"add")==0)
        {
          temp=strtok(NULL,",");
          cli->SyncAdd(temp,0);

        }
        else if(strcmp(temp,"delete")==0)
        {
          temp=strtok(NULL,",");
          cli->SyncDelete(temp);
        }
        else if(strcmp(temp,"sync")==0)
        {
          selection='2';
        }
        else if(strcmp(temp,"exit")==0)
        {
          selection='3';
        }

        sigprocmask(SIG_SETMASK,&o,NULL);
        break; 
      case '2':
        struct pollfd fds[1];
        fds[0].fd=0;
        fds[0].events=POLLIN | POLLERR;
        fds[0].revents=0;
        signal(SIGALRM,filesync);
        alarm(UPDATERATE);
        while(1)
        {
          int ret;
          ret=poll(fds,1,-1);
          if(fds[0].revents & POLLIN)
          {
            read(0,&selection,1);
            break;
          }
        }
        break;
      case '3':
        printf("exit!\n");
        return;
        break;
      default:
        break;
     }
  }
}




int main()
{
  mainstream();
  return 0;
}

