#include <stdio.h>
#include <string.h>
/*
typedef union _scheme{
	struct _server{
		char *host;
		int port;
		char *user;
		char *passwd;
		char *binlog;
	}Url;
	struct _file{
		char *path;
	}
}Scheme;
*/

/*search match for chars */
static int smatch(char c,char *s){
	while(*s!='\0'){
		if(c==*s)return 1;
		s++;
	}
	return 0;
}
char *user,*passwd,*host,*file;
char *port;
void * parseUrl(const char *surl){
	char *next;
	int done = 0;
	int s_user,s_passwd,s_host,s_port,s_file;
	s_user=s_passwd=s_host=s_port=s_file = 1;

	char *url = strdup(surl);

	if(url[0]=='m'){/*mysql url*/
		next = ":";
		while(*++url!='\0'){
			if(smatch(*url,next)){
				if(s_user){
					s_user = 0;
					user = url+3;
					done++;
					next=":@";
					continue;
				}
				if(s_passwd && *url == ':'){
					s_passwd=0;
					*url='\0';
					passwd = url+1;	
					done++;
					continue;
					next = "@";
				}
				if( s_host && *url == '@'){
					s_passwd = 0;
					s_host = 0;
					*url='\0';
					host = url+1;	
					done++;
					next = ":/";
					continue;
				}
				if(s_port && *url == ':'){
					s_port = 0;
					*url='\0';
					port = url+1;	
					done++;
					next = "/";
					continue;
				}
				if(s_file && *url == '/'){
					s_port = 0;
					s_file = 0;
					*url='\0';
					file = url+1;	
					done++;
					continue;
				}
				if(*url == '/'){
					*url='\0';
					done++;
					continue;
				}
			}

		}
	}else if(url[0]=='f'){ /*local file url*/

	}
}
int main(int argc, const char *argv[]) {
	const char *url = "mysql://user:123456@localhost:3306/mysql-bin.000001";
	parseUrl(url);
	printf("host:%s\n",host);
	printf("port:%s\n",port);
	printf("user:%s\n",user);
	printf("passwd:%s\n",passwd);
	printf("file:%s\n",file);
	return 0;
}
