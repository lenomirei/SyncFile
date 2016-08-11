#include "head.h"
#include "code.h"
#include "FileTransport.h"
#include "Compress.h"
#include "user.h"
#include "md5.h"


long long GetFileSize(const char *filename)
{
  struct stat buf;
  if(stat(filename,&buf)<0)
  {
    return 0;
  }
  return (long long)buf.st_size;
}

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








struct FileList
{
  FileList()
    :size(0)
  {

  }
  void Add(const char *filepath,const char* filemd5)
  {
    FilePath[size]=filepath;
    Filemd5[size]=filemd5;
    size++;
  }
  void Delete(const char *filepath)
  {
    vector<string>::iterator it;
    vector<string>::iterator itmd5;
    it = FilePath.begin();
    itmd5=Filemd5.begin();
    for(;it!=FilePath.end();)
   {
      if(*it==filepath)
      {
        it=FilePath.erase(it);
        itmd5=Filemd5.erase(itmd5);
        size--;
        break;
      }
      else if(strstr((*it).c_str(),filepath)!=NULL)
      {
        it=FilePath.erase(it);
        itmd5=Filemd5.erase(itmd5);
        size--;
      }
      else
      {
        it++;
        itmd5++;
      }
    }
  }
  bool operator==(FileList &fl)
  {
    if(size!=fl.size)
    {
      return false;
    }
    for(int i=0;i<size;++i)
    {
      if(FilePath[i]!= fl.FilePath[i] || Filemd5[i]!=fl.Filemd5[i] )
      {
        return false;
      }
    }
    return true;
  }
  vector<string> FilePath;
  vector<string> Filemd5;
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
    Serverfl.Filemd5.resize(10);
  }
  void InitLocalfl()
  {
    Localfl.FilePath.resize(10);
    Localfl.Filemd5.resize(10);
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


    cJSON *filemd5=cJSON_Parse(data);
    for(int i=0;i<sizeofarray;++i)
    {
      char *temp=cJSON_GetArrayItem(filemd5,i)->valuestring;
      Serverfl.Filemd5[i]=temp;

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


        
            DownloadFile(Serverfl.FilePath[i].c_str(),sockConn);
        
       


          printf("download success!next one\n");

          Localfl.FilePath[i]=Serverfl.FilePath[i];
          Localfl.Filemd5[i]=Serverfl.Filemd5[i]; 
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
  void FileUpdate(const char *filepath,const char *filemd5)//专门认为是从客户端上传到服务器端
  {
    //add or delete or change will do this
    struct stat buf;
    lstat(filepath,&buf);
    if(S_ISDIR(buf.st_mode))
    {
      struct dirent *ent=NULL;
      DIR *pDir;
      pDir=opendir(filepath);
      while(NULL!=(ent=readdir(pDir)))
      {
        if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)
        {
          continue;
        }
        string temp=filepath;
        temp+=ent->d_name;
char *leno=GetMD5(temp.c_str());
        FileUpdate(temp.c_str(),leno);
delete leno;
      }
    }
    else
    {
      SendSignal(3,sockConn);
      Localfl.Add(filepath,filemd5);
      cJSON *fileinfo=cJSON_CreateObject();
      cJSON_AddItemToObject(fileinfo,"filepath",cJSON_CreateString(filepath));
      cJSON_AddItemToObject(fileinfo,"filemd5",cJSON_CreateString(filemd5));
      char *fileinfosend=cJSON_Print(fileinfo);
      send(sockConn,fileinfosend,1024,0);
  
      recv(sockConn,NULL,1024,0);
#ifdef _DEBUG_
      clock_t start=clock();
#endif
        int ret=UploadFile(filepath,sockConn);//!!!!!!!!!
#ifdef _DEBUG_
        clock_t end=clock();
        cout<<"transport time count is"<<((double)(end-start))/CLOCKS_PER_SEC<<endl;
#endif
    }
  }

  void SyncAdd(const char *filepath,const char* filemd5)
  {
    Connect();
    //char test[JSONSIZE]={'\0'};
    Localfl.Add(filepath,filemd5);


    FileUpdate(filepath,filemd5);
    SendSignal(5,sockConn);
  }
  void SyncDelete(const char *filepath)
  {
    Connect();
    SendSignal(4,sockConn);

    Localfl.Delete(filepath);
    send(sockConn,filepath,512,0);//!!!!!!!!!!!!!!!!!!
    SendSignal(5,sockConn);
  }
  void SyncContinue()
  {
    if(GetFileSize("./log")>0)
    {
      FILE *fp=NULL;
      long long offset=0;
      char filename[20]={'\0'};
      fp=fopen("log","rb");
      if(NULL==fp)
      {
        return ;
      }
      fscanf(fp,"%s %lld",filename,&offset);
      Connect();
      SendSignal(6,sockConn);
      cJSON *fileinfo=cJSON_CreateObject();
      cJSON_AddItemToObject(fileinfo,"filename",cJSON_CreateString(filename));
      cJSON_AddItemToObject(fileinfo,"offset",cJSON_CreateNumber(offset));
      char *fileinfosend=cJSON_Print(fileinfo);
      send(sockConn,fileinfosend,1024,0);
      DownloadFile(filename,sockConn,offset);
      SendSignal(5,sockConn);
    }
  }
  int Connect()
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


struct termios initialrsettings, newrsettings;
tcgetattr(fileno(stdin), &initialrsettings);
newrsettings = initialrsettings;
newrsettings.c_lflag &= ~ECHO;



  cout<<"please input userpassword"<<endl;

if(tcsetattr(fileno(stdin), TCSAFLUSH, &newrsettings) != 0)
    {

        fprintf(stderr,"Could not set attributes\n");//异常处理
    }

 else {
        cin>>userpassword;

        tcsetattr(fileno(stdin), TCSANOW, &initialrsettings);
    }

char *md5passwd=GetPasswdMD5(userpassword);
char *codepasswd=GetPasswdMD5(md5passwd);
  
  while(UserCheck(username,codepasswd)!=0)
  {
    system("clear");
    printf("username or password is wrong ,please check in\n");
    cout<<"please input username"<<endl;
    cin>>username;
    cout<<"please input userpassword"<<endl;
    cin>>userpassword;
  }
delete md5passwd;
delete codepasswd;
  system("clear");
  printf("login success!\n");
  //cli->SyncAdd("./SyncFloderServer/test1.txt",0);
  //cli->SyncAdd("./SyncFloderServer/test2.txt",0);
  //cli->SyncAdd("./SyncFloderServer/test6.txt",0);
  //cli->SyncAdd("./SyncFloderServer/test7.txt",0);
  //cli->SyncAdd("./SyncFloderServer/test.pdf",0);
  //cli->SyncDelete("./SyncFloderServer/test1.txt");
  

  
  //cli->SyncContinue();

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
          while(temp!=NULL)
          {
	    char *dele=GetMD5(temp);
            cli->SyncAdd(temp,dele);
            temp=strtok(NULL,",");
	    delete dele;
          }

        }
        else if(strcmp(temp,"delete")==0)
        {
          temp=strtok(NULL,",");
	  string dsc="rm -rf ";
	  dsc+=temp;
          cli->SyncDelete(temp);
          system(dsc.c_str());
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

