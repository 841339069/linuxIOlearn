#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<assert.h>

#define IPADDR "127.0.0.1"
#define PORT 8787
#define MAXLINE 1024
#define LISTENQ 5
#define SIZE 10

typedef struct server_context_st
{
	int cli_cnt; /*客户端个数*/
	int clifds[SIZE]; /*客户端的个数*/
	fd_set allfds;  /*句柄集合*/
	int maxfd;       /*句柄最大直*/
}server_context_st;
static server_context_st *s_srv_ctx=NULL;

static int create_server_proc(const char *ip,int port){
	int fd;
	struct sockaddr_in servaddr;
	fd=socket(AF_INET,SOCK_STREAM,0);
	if(fd==-1){
		printf("create fail!");
		return -1;
	}
	int ch=1;
	if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&ch,sizeof(ch))==-1)
		return -1;
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	inet_pton(AF_INET,ip,&servaddr.sin_addr);
	servaddr.sin_port=htons(port);
	if(bind(fd,(struct sockaddr*)&servaddr,sizeof(servaddr))==-1){
		perror("bind error:");
		return -1;
	}
	listen(fd,LISTENQ);
	return fd;
}

static int accpet_client_proc(int srvfd){
	struct sockaddr_in cliaddr;
	socklen_t cliaddrlen;
	cliaddrlen=sizeof(cliaddr);
	int clifd=-1;
	printf("accpet clint proc is called.\n");
ACCPET:
        clifd=accept(srvfd,(struct sockaddr*)&cliaddr,&cliaddrlen);
	if(clifd==-1){
		if(errno==EINTR)
			goto ACCPET;
		else{
			printf("accpet fail.\n");
			return -1;
		}
	}
	int i=0;
	for(i=0;i<SIZE;i++){
		if(s_srv_ctx->clifds[i]<0){
			s_srv_ctx->clifds[i]=clifd;
			s_srv_ctx->cli_cnt++;
			break;
		}
	}
	if(i==SIZE)
		return -1;
}

static int handle_client_msg(int fd,char *buf){
	assert(buf);
	printf("recv buf is %s\n",buf);
	write(fd,buf,strlen(buf)+1);
	return 0;
}

static void recv_client_msg(fd_set *readfds){
	int i=0,n=0;
	int clifd;
	char buf[MAXLINE]={0};
	for(i=0;i<=s_srv_ctx->cli_cnt;i++){
		clifd=s_srv_ctx->clifds[i];
		if(clifd<0)
			continue;
		if(FD_ISSET(clifd,readfds)){
			n=read(clifd,buf,MAXLINE);
			if(n<0){
				FD_CLR(clifd,&s_srv_ctx->allfds);
				close(clifd);
				s_srv_ctx->clifds[i]=-1;
				continue;
			}
			handle_client_msg(clifd,buf);
		}
	}
}

static void handle_client_proc(int srvfd){
	int clifd=-1;
	int netval=0;
	fd_set *readfds=&s_srv_ctx->allfds;
	struct timeval tv;
	int i=0;
	while(1){
		FD_ZERO(readfds);
		FD_SET(srvfd,readfds);
		s_srv_ctx->maxfd=srvfd;
		tv.tv_sec=30;
		tv.tv_usec=0;
		for(i=0;i<s_srv_ctx->cli_cnt;i++){
			clifd=s_srv_ctx->clifds[i];
			if(clifd!=-1){
				FD_SET(clifd,readfds);
			}
			s_srv_ctx->maxfd=(clifd>s_srv_ctx->maxfd?clifd:s_srv_ctx->maxfd);
		}
		netval=select(s_srv_ctx->maxfd+1,readfds,NULL,NULL,&tv);
		if(netval==-1){
			printf("select fail\n");
			return ;
		}
		if(netval==0){
			printf("select timeout\n");
			continue;
		}
		if(FD_ISSET(srvfd,readfds)){
			accpet_client_proc(srvfd);
		}
		else{
			recv_client_msg(readfds);
		}
	}
}

static void server_uninit(){
	if(s_srv_ctx){
		free(s_srv_ctx);
		s_srv_ctx=NULL;
	}
}

static int server_init(){
	s_srv_ctx=(server_context_st*)malloc(sizeof(server_context_st));
	if(s_srv_ctx==NULL)
		return -1;
	memset(s_srv_ctx,0,sizeof(server_context_st));
	int i=0;
	for(i=0;i<SIZE;i++){
		s_srv_ctx->clifds[i]=-1;
	}
	return 0;
}

int main(int argc,char* argv[]){
	int srvfd;
	if(server_init()<0)
		return -1;
	srvfd=create_server_proc(IPADDR,PORT);
	if(srvfd<0)
		goto err;
	handle_client_proc(srvfd);
	server_uninit();
	return 0;
err:
        server_uninit();
	return -1;
}
