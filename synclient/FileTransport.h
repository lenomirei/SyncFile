struct datapack
{
  int size;
  char data[BUFFSIZE];
  datapack()
  {
    memset(data,'\0',BUFFSIZE);
  }
};
const char *emname=NULL;
long long totallength=0;
int oho;


void down(int num)
{
  FILE *fp;
  fp=fopen("./log","wb+");
  fprintf(fp,"%s %lld",emname,totallength);
  SendSignal(5,oho);
  exit(3);
}


void up(int num)
{
struct datapack *p=new datapack();
p->size=0;
send(oho,p,JSONSIZE,0);
SendSignal(5,oho);

}


int UploadFile(const char * filepath,int sockConn)
{
oho=sockConn;
signal(SIGINT,up);
  printf("tranporting file%s\n",filepath);

  int filefd=open(filepath,O_RDONLY);
  if(filefd<0)
  {
    printf("file open error!\n");
    return -1;
  }

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
signal(SIGINT,SIG_IGN);
  return 0;
}
int DownloadFile(const char *filepath,int clientfd,long long offset=0)
{
oho=clientfd;
signal(SIGINT,down);
  printf("downloading file %s......\n",filepath);
  totallength=0;
  emname=filepath;
  char recvBuf[JSONSIZE]={'\0'};
  int filefd;
  filefd = open(filepath,O_RDWR|O_CREAT,0777);
  if(filefd<0)
  {
    char *dir=dirname((char *)filepath);
#ifdef _DEBUG_
    printf("dir is %s\n",dir);
#endif
    if(opendir(dir)==NULL)
    {
      mkdir(dir,0644);
      filefd=open(filepath,O_RDWR | O_CREAT | O_TRUNC,0777);
    }
  }
  else
  {
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

      totallength+=len;
      if(len<=0)
      {
        break;
      }
      write(filefd ,Buf->data ,len);
      memset(recvBuf,'\0',JSONSIZE);
    }
  }
}
