#include "socket_server.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
			printf("message(%lu) [id=%d] size=%d, data: %s\n",result.opaque,result.id, result.ud, result.data);
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

const char* str_data = "hello adan_srv-v8";
#define test_count (128)

static void test(void){
	pthread_t pid;
	int c,l,b,i;
	void * pdata;
	struct socket_server * ss;
	int id_pool[test_count];

	ss = socket_server_create();
	
	pthread_create(&pid, NULL, _poll, ss);											//创建新线程for srv io_event (仅只处理io事件, 为做任何收发io 操作)

	c = socket_server_connect(ss,100,"127.0.0.1",80);						//connect 非阻塞test
	printf("connecting %d\n",c);

	l = socket_server_listen(ss,200,"127.0.0.1",8888,32);				//监听
	printf("listening %d\n",l);

	socket_server_start(ss,201,l);															//启动srv

	b = socket_server_bind(ss,300,1);														//bind 标准输入stdin 到ss(绑定前, accept from 1; 绑定后, accept from 2; 这个操作跟pipe 管道有关?)
	printf("binding stdin %d\n",b);

	for(i=0;i<test_count;i++){
		id_pool[i] = socket_server_connect(ss, 400+i, "127.0.0.1", 8888);				//成功返回id, id 不等于sfd, 但可以在管理pool 中找到结构体
		printf("socket_server_connect(): return value = %d, second_parameter = %d\n", id_pool[i], 400+i);//for test only
	}
	sleep(5);

	for(i=0;i<test_count;i++){
		pdata = MALLOC(sizeof(str_data));													//发送的数据buf, 必须是malloc 分配的, 否则释放时会报错!! socket_server_poll() 解析data 完毕后会自动释放指针
		memcpy(pdata, str_data, sizeof(str_data));
		//socket_server_send(ss, id_pool[i], pdata, sizeof(str_data));					//高优先级发送data(需要使用id), 接收数据会在socket_server_poll() 的SOCKET_DATA 事件中
		socket_server_send_lowpriority(ss, id_pool[i], pdata, sizeof(str_data));//低优先级发送data
	}
	sleep(5);

	socket_server_exit(ss);																			//退出srv

	pthread_join(pid, NULL);

	socket_server_release(ss);
}

int main(){
	struct sigaction sa;
	
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);																	//忽略管道信号

	test();																											//这个原始test(), 只能测试API 的有效性, 并不能测试API 的使用方法, 因此, passed

	return 0;
}

