struct datapack
{
  int size;
  char data[BUFFSIZE];
  datapack()
  {
    memset(data,'\0',BUFFSIZE);
  }
};

int UploadFile(const char * filepath,int sockConn,long long offset=0)
{
  printf("tranporting file%s\n",filepath);

  int filefd=open(filepath,O_RDONLY);
  if(filefd<0)
  {
    printf("file open error!\n");
    return -1;
  }
  lseek(filefd,offset,SEEK_SET);
  while(1)
  {
    struct datapack *Buf=new datapack();
    int len = read(filefd,Buf->data,BUFFSIZE);
    Buf->size=len;
    int test=send(sockConn,Buf,JSONSIZE,0);
    if(len < 1)
    {
      break;
    }
    delete Buf;
  }
  return 0;
}
int DownloadFile(const char *filepath,int clientfd,long long offset=0)
{
  printf("downloading file %s......\n",filepath);
  char recvBuf[JSONSIZE]={'\0'};
  int filefd;
  filefd = open(filepath,O_RDWR|O_CREAT,0777);
  if(filefd<0)
  {
char *temp=new char[strlen(filepath)+1];
strcpy(temp,filepath);
    char *dir=dirname(temp);
printf("filepath is %s!!!!!!!!!!\n",filepath);
#ifdef _DEBUG_
    printf("dir is %s\n",dir);
#endif
    if(opendir(dir)==NULL)
    {
      mkdir(dir,0644);
      filefd=open(filepath,O_RDWR | O_CREAT | O_TRUNC,0777);
        
    }
 else 
 {
 printf("error!\n");
 }
delete[] temp;
  }
filefd = open(filepath,O_RDWR|O_CREAT,0777);
    lseek(filefd,offset,SEEK_SET);
    while(1)
    {
      int packcount=recv(clientfd,recvBuf,JSONSIZE,0);


      while(packcount>0 && packcount<JSONSIZE)
      {
        packcount+=recv(clientfd,recvBuf+packcount,JSONSIZE-packcount,0);
      }
      struct datapack *Buf=(struct datapack *)recvBuf;

      int len=Buf->size;

      if(len<=0)
      {
        break;
      }
      write(filefd ,Buf->data ,len);
      memset(recvBuf,'\0',JSONSIZE);
    }
  
}
