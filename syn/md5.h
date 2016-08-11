

char* GetMD5(const char *password)
{
	MD5_CTX ctx;
	int len = 0;
	unsigned char digest[16] = {0};
	
	
	MD5_Init (&ctx);

	MD5_Update (&ctx, password, strlen(password));

	MD5_Final (digest, &ctx);
	
	

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

	
