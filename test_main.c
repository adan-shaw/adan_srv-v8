#include "socket_server.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static void *_poll(void * ud){
	struct socket_server *ss = ud;
	struct socket_message result;
	int type;
	for(;;){
		type = socket_server_poll(ss, &result, NULL);
		// DO NOT use any ctrl command (socket_server_close , etc.) in this thread.
		switch(type){
		case SOCKET_EXIT:
			return NULL;
		case SOCKET_DATA:
			printf("message(%lu) [id=%d] size=%d\n",result.opaque,result.id, result.ud);
			free(result.data);
			break;
		case SOCKET_CLOSE:
			printf("close(%lu) [id=%d]\n",result.opaque,result.id);
			break;
		case SOCKET_OPEN:
			printf("open(%lu) [id=%d] %s\n",result.opaque,result.id,result.data);
			break;
		case SOCKET_ERROR:
			printf("error(%lu) [id=%d]\n",result.opaque,result.id);
			break;
		case SOCKET_ACCEPT:
			printf("accept(%lu) [id=%d %s] from [%d]\n",result.opaque, result.ud, result.data, result.id);
			break;
		}
	}
}

static void test(struct socket_server *ss){
	pthread_t pid;
	int c,l,b,i;
	pthread_create(&pid, NULL, _poll, ss);								//创建新线程for srv io_event (仅只处理io事件, 为做任何收发io 操作)

	c = socket_server_connect(ss,100,"127.0.0.1",80);			//connect 非阻塞test
	printf("connecting %d\n",c);

	l = socket_server_listen(ss,200,"127.0.0.1",8888,32);	//监听
	printf("listening %d\n",l);

	socket_server_start(ss,201,l);												//启动srv

	b = socket_server_bind(ss,300,1);											//bind 标准输入stdin 到ss(绑定前, accept from 1; 绑定后, accept from 2; 这个操作跟pipe 管道有关?)
	printf("binding stdin %d\n",b);

	for(i=0;i<100;i++){
		socket_server_connect(ss, 400+i, "127.0.0.1", 8888);
	}
	sleep(5);

	socket_server_exit(ss);																//退出srv

	pthread_join(pid, NULL); 
}

int main(){
	struct sigaction sa;
	struct socket_server * ss;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);														//忽略管道信号

	ss = socket_server_create();
	test(ss);
	socket_server_release(ss);

	return 0;
}
