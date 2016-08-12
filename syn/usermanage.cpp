#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <mysql/mysql.h>
#include "head.h"
#include "md5.h"
using namespace std;


void menu()
{
  printf("welcome to use usermanage system!\n");
  printf("please choose one option\n");
  printf("1.add user\n");
  printf("2.delete user\n");
  printf("3.find user\n");
  printf("4.exit system\n");
}


int main()
{
  const char * host = "127.0.0.1";
  const char * user = "root";
  const char * pwd = "shiwanfute";
  const char * dbname = "UserInfo";
  char sql[100]={'\0'};
  unsigned int port = 3306;
  int status;

	MYSQL *mysql;
	mysql = mysql_init(0);
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (mysql_real_connect(mysql, host, user, pwd, dbname, port, NULL, 0) == NULL) {
		cout<< "connect failure!" << endl;
		return -1;
	}




  char username[21]={'\0'};
  char userpassword[21]={'\0'};
  int selection;
  while(selection!=4)
  {
    system("clear");
    menu();
    cin>>selection;
    system("clear");
    switch(selection)
    {
      case 1:
        cout<<"please input username"<<endl;
        cin>>username;
        cout<<"please input userpassword"<<endl;
        cin>>userpassword;
	
	char *md5passwd,*codepasswd;
	md5passwd=GetMD5(userpassword);
printf("1\n");
	codepasswd=GetMD5(md5passwd);
printf("2\n");

        sprintf(sql,"insert into user(name,password) values('%s','%s');",username,codepasswd);
printf("%s\n",sql);
        mysql_set_character_set(mysql, "gbk");
        status = mysql_query(mysql, sql);
        if (status != 0) {
          cout << "query failure!" << endl;
          return -1;
        }
//	delete md5passwd;
//	delete codepasswd;
        break;
      case 2:
        cout<<"please input username"<<endl;
        cin>>username;
        sprintf(sql,"delete from user where name='%s';",username);
        mysql_set_character_set(mysql, "gbk");
        status = mysql_query(mysql, sql);
        if (status != 0) {
          cout << "query failure!" << endl;
          return -1;
        }
        break;
      case 3:
        cout<<"please input username"<<endl;
        cin>>username;
        sprintf(sql,"select * from user where name='%s';",username);
        mysql_set_character_set(mysql, "gbk");
        status = mysql_query(mysql, sql);
	      result = mysql_store_result(mysql);
        if (status != 0) {
          cout << "query failure!" << endl;
          return -1;
        }
        if(0==mysql_num_rows(result))
        {
          printf("nothing is found\n");
          break;
        }
        row=mysql_fetch_row(result);
      	while (NULL!=row) 
        {
          cout<<row[0]<<endl;
          row=mysql_fetch_row(result);
      	}
        printf("press any key to continue\n");
        break;
      case 4:
        break;
      default:
        break;
    }
    fgetc(stdin);
    fgetc(stdin);
  }





  int rowcount=mysql_num_rows(result);
  int fieldcount=mysql_num_fields(result);


	mysql_free_result(result);
	mysql_close(mysql);
  return 0;
}
