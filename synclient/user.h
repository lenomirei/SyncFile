
int UserCheck(const char *username,const char *userpassword)
{
const char * host = "192.168.1.1";
const char * user = "root";
const char * pwd = "shiwanfute";
const char * dbname = "UserInfo";
char sql[100]={'\0'};
sprintf(sql,"select password from user where name='%s'",username);
#ifdef _DEBUG_
printf("the sql is %s\n",sql);
#endif 
unsigned int port = 3306;
int status;

	MYSQL *mysql;
	mysql = mysql_init(0);
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (mysql_real_connect(mysql, host, user, pwd, dbname, port, NULL, 0) == NULL) {
		cout << "connect failure!" << endl;
		return -1;
	}
	mysql_set_character_set(mysql, "gbk");
	status = mysql_query(mysql, sql);
	if (status != 0) {
		cout << "query failure!" << endl;
    return -1;
	}
	result = mysql_store_result(mysql);


  int rowcount=mysql_num_rows(result);
  if(rowcount<1)
  {
    printf("oho , you are not the customer!get out !\n");
    return -2;
  }
  int fieldcount=mysql_num_fields(result);


  row=mysql_fetch_row(result);
	while (NULL!=row) {
    if(strcmp(row[0],userpassword)!=0)
    {
      printf("oho,your password is not correct!\n");
      return 1;
    }
    row=mysql_fetch_row(result);
	}
	mysql_free_result(result);
	mysql_close(mysql);
  return 0;
}
