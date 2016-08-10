

char* GetMD5(const char *filepath)
{
	MD5_CTX ctx;
	int len = 0;
	unsigned char buffer[1024*1024] = {0};
	unsigned char digest[16] = {0};
	
	FILE *pFile = fopen (filepath, "rb"); 
	
	MD5_Init (&ctx);

	while ((len = fread (buffer, 1, 1024*1024, pFile)) > 0)
	{
		MD5_Update (&ctx, buffer, len);
	}

	MD5_Final (digest, &ctx);
	
	fclose(pFile);
	

	int i = 0;
	char *buf=new char[33];
	memset(buf,'\0',33);
        char tmp[3] = {0};
        for(i = 0; i < 16; i++ )
	{
		sprintf(tmp,"%02X", digest[i]); 
		strcat(buf, tmp); 
        }
	

    return buf;
}

	
