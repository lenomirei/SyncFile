#include <stdio.h>
#include <iostream>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <vector>
#include <zlib.h>
#include "cJSON.h"
extern "C"
{
#include <b64/cencode.h>
#include <b64/cdecode.h>
}



using namespace std;
#define DEFAULTPORT 8001
#define FILELISTSIZE 1024
#define BUFFSIZE 512
#define JSONSIZE 2048
#define SIGSIZE 17
#define SIZE 100
#define UPDATERATE 3


void UnCompressFile(const char *filepath)
{
  gzFile temp;
  int filefd;
  char readbuf[BUFFSIZE]={'\0'};
  temp=gzopen("compressedfile.gz","rb9");
  filefd=open(filepath,O_WRONLY | O_CREAT);
  while(1)
  {
    int len=gzread(temp,readbuf,BUFFSIZE);
    if(len<1)
    {
      break;
    }
    write(filefd,readbuf,len);
    memset(readbuf,'\0',BUFFSIZE);
  }
  gzclose(temp);
}


//char fileend[BUFFSIZE]={'\0'};

void encode(FILE* inputFile, FILE* outputFile)
{
	/* set up a destination buffer large enough to hold the encoded data */
	int size = SIZE;
	char* input = (char*)malloc(size);
	char* encoded = (char*)malloc(2*size); /* ~4/3 x input */
	/* we need an encoder and decoder state */
	base64_encodestate es;
	/* store the number of bytes encoded by a single call */
	int cnt = 0;
	
	/*---------- START ENCODING ----------*/
	/* initialise the encoder state */
	base64_init_encodestate(&es);
	/* gather data from the input and send it to the output */
	while (1)
	{
		cnt = fread(input, sizeof(char), size, inputFile);
		if (cnt == 0) break;
		cnt = base64_encode_block(input, cnt, encoded, &es);
		/* output the encoded bytes to the output file */
		fwrite(encoded, sizeof(char), cnt, outputFile);
	}
	/* since we have reached the end of the input file, we know that 
	   there is no more input data; finalise the encoding */
	cnt = base64_encode_blockend(encoded, &es);
	/* write the last bytes to the output file */
	fwrite(encoded, sizeof(char), cnt, outputFile);
	/*---------- STOP ENCODING  ----------*/
	
	free(encoded);
	free(input);
}

void decode(FILE* inputFile, FILE* outputFile)
{
	/* set up a destination buffer large enough to hold the decoded data */
	int size = SIZE;
	char* encoded = (char*)malloc(size);
	char* decoded = (char*)malloc(1*size); /* ~3/4 x encoded */
	/* we need an encoder and decoder state */
	base64_decodestate ds;
	/* store the number of bytes encoded by a single call */
	int cnt = 0;
	
	/*---------- START DECODING ----------*/
	/* initialise the encoder state */
	base64_init_decodestate(&ds);
	/* gather data from the input and send it to the output */
	while (1)
	{
		cnt = fread(encoded, sizeof(char), size, inputFile);
		if (cnt == 0) break;
		cnt = base64_decode_block(encoded, cnt, decoded, &ds);
		/* output the encoded bytes to the output file */
		fwrite(decoded, sizeof(char), cnt, outputFile);
	}
	/*---------- START DECODING  ----------*/
	
	free(encoded);
	free(decoded);
}


void BinaryFile(const char *filepath)
{
	FILE* encodedFile;
	FILE* decodedFile;
	encodedFile = fopen("encodedfile", "r");
	decodedFile = fopen(filepath, "w");
	
	decode(encodedFile, decodedFile);
	
	fclose(encodedFile);
	fclose(decodedFile);

}


void DownloadFile(const char *filepath,int clientfd)
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


int UpdateFile(const char *filepath,int sockConn)
{

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
    char *signals;
    cJSON *root=cJSON_CreateObject();
    cJSON_AddItemToObject(root,"signal",cJSON_CreateNumber(1));
    signals=cJSON_Print(root);
    send(sockConn,signals,SIGSIZE,0);


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
            BinaryFile(Serverfl.FilePath[i].c_str());
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
    char *signals;
    cJSON *root=cJSON_CreateObject();
    cJSON_AddItemToObject(root,"signal",cJSON_CreateNumber(3));
    signals=cJSON_Print(root);
    send(sockConn,signals,SIGSIZE,0);

    Localfl.Add(filepath,filesize);


    cJSON *fileinfo=cJSON_CreateObject();
    cJSON_AddItemToObject(fileinfo,"filepath",cJSON_CreateString(filepath));
    cJSON_AddItemToObject(fileinfo,"filesize",cJSON_CreateNumber(filesize));
    char *fileinfosend=cJSON_Print(fileinfo);
    send(sockConn,fileinfosend,BUFFSIZE,0);


    printf("I should zuse here\n");
    recv(sockConn,NULL,JSONSIZE,0);
    printf("I am right!\n");


    UpdateFile(filepath,sockConn);
  }

  void SyncAdd(const char *filepath,long long filesize)
  {
    //char test[JSONSIZE]={'\0'};
    Localfl.Add(filepath,filesize);


    FileUpdate(filepath,filesize);
    SendSignal(5,sockConn);
  }
  void SyncDelete(const char *filepath)
  {
    char *signals;
    cJSON *root=cJSON_CreateObject();
    cJSON_AddItemToObject(root,"signal",cJSON_CreateNumber(4));
    signals=cJSON_Print(root);
    send(sockConn,signals,SIGSIZE,0);

    Localfl.Delete(filepath);
    send(sockConn,filepath,JSONSIZE,0);
  }
  void SyncChange();

private:
  int sockConn;
  FileList Localfl;
  FileList Serverfl;
};



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

  
  struct sockaddr_in server_addr;
  int sockConn=ConnectToServer("192.168.84.132",DEFAULTPORT,server_addr);
  cli->SetsockConn(sockConn);
  cli->SyncAdd("./SyncFloderServer/test.pdf",0);
  //cli->SyncDelete("./SyncFloderServer/test1.txt");

  //signal(SIGALRM,filesync);
  //alarm(UPDATERATE);
  //char command[1024]={'\0'};
  //while(1)
  //{
    //sleep(UPDATERATE);
  //}
}




int main()
{
  mainstream();
  return 0;
}

