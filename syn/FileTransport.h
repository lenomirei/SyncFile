

int UploadFile(const char * filepath,int sockConn)
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
int DownloadFile(char *filepath,int clientfd)
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
