#ifndef adan_socket_server_h
#define adan_socket_server_h

#include "socket_serverEx.h"

//id 和fd 的区别, 为什么需要id?
/*
	目前需要关注的是, 每个socket 结构体有一个自己的id, 这个id 就是slot 中的索引;

	可以看到在socket 中除了fd 还有一个id, 
	那么能不能直接使用fd 作为socket 的唯一标识呢?
	只从fd 的分配范围上来看的话, 是没什么问题的, 
	但是因为fd 的分配策略, 内核可能会复用 fd, 这就导致了冲突发生的可能性, 
	skynet 分配的id 会不断累加, 在非极端情况下都不会有重复的风险, 因此作为socket 的唯一标识是更加合适的;
	skynet 在分配socket 的id 时, 也会碰到空洞位置的问题, 
	因为关闭的socket 连接会被再次设为可用状态, 这就导致了分配id 不能简单累加, 
	而是从alloc_id 开始, 遍历完整个 slot 数组, 计算哈希的时候直接针对 MAX_SOCKET 取模即可;
	这里有个小问题, alloc_id 是个原子变量, 可能会让这个分配id 的函数的效率雪上加霜, 最坏情况下在分配一次id 的过程中, alloc_id 要被 atomic_fetch_add 累加几万次;

	因此, id != fd

	srv:
		struct socket_server * ss = socket_server_create();					//创建'sfd info结构体'
		int id = socket_server_listen(ss,200,"127.0.0.1",8888,32);	//命令原始'sfd info结构体'进行端口监听
		socket_server_start(ss,201,id);															//监听'sfd info结构体' start, 并开始io 循环
		int type = socket_server_poll(ss, &result, NULL);						//接收监听'sfd info结构体'下所有的io 事件
		socket_server_send(ss, c, sendBuf, strlen(sendBuf));				//进行高优先级数据发送(srv 被动回覆)
		socket_server_close(ss,201,id);															//关闭指定id 的'sfd info结构体', 并踢出监听poll(踢客户端cli 下线)
		socket_server_exit(ss);																			//并通知socket_server_poll 停止io 循环
		socket_server_release(ss);																	//释放'sfd info结构体'
	cli:
		struct socket_server * ss = socket_server_create();					//创建'sfd info结构体'
		int id = socket_server_connect(ss,200,"127.0.0.1",8888);		//命令原始'sfd info结构体'创建非阻塞连接
		socket_server_send(ss, c, sendBuf, strlen(sendBuf));				//进行高优先级数据发送(cli 主动发送)
		socket_server_release(ss);																	//释放'sfd info结构体'
*/

//原始'sfd info结构体' 的创建和释放
struct socket_server * socket_server_create();																																//创建'sfd info结构体'
void socket_server_release(struct socket_server *);																														//释放'sfd info结构体'

//特殊的监听'sfd info结构体'的操作
int socket_server_poll(struct socket_server *, struct socket_message *result, int *more);											//接收监听'sfd info结构体'下所有的io 事件, 包括accept, 数据io 循环
void socket_server_exit(struct socket_server *);																															//监听'sfd info结构体' exit, 并通知socket_server_poll 停止io 循环
void socket_server_start(struct socket_server *, uintptr_t opaque, int id);																		//监听'sfd info结构体' start, 并开始io 循环
void socket_server_close(struct socket_server *, uintptr_t opaque, int id);																		//在监听'sfd info结构体'中, 关闭指定id 的'sfd info结构体', 并踢出监听poll

// return -1 when error
int64_t socket_server_send(struct socket_server *, int id, const void * buffer, int sz);											//命令已建立连接的'sfd info结构体', 进行高优先级数据发送
void socket_server_send_lowpriority(struct socket_server *, int id, const void * buffer, int sz);							//命令已建立连接的'sfd info结构体', 进行低优先级数据发送

// ctrl command below returns id(控制命令: 以下的返回id)
int socket_server_listen(struct socket_server *, uintptr_t opaque, const char * addr, int port, int backlog);	//命令原始'sfd info结构体'进行端口监听(并加入epoll列表), 返回id
int socket_server_connect(struct socket_server *, uintptr_t opaque, const char * addr, int port);							//命令原始'sfd info结构体'创建非阻塞连接, 并加入epoll列表, 返回id
//int socket_server_block_connect(struct socket_server *, uintptr_t opaque, const char * addr, int port);			//命令原始'sfd info结构体'创建阻塞连接, 并加入epoll列表, 返回id(已经被弃用)
int socket_server_bind(struct socket_server *, uintptr_t opaque, int fd);																			//绑定某个fd到epoll列表, 会自动添加'sfd info结构体', 返回id

// for tcp
void socket_server_nodelay(struct socket_server *, int id);																										//命令已建立连接的'sfd info结构体'的开启nodelay 算法

struct socket_udp_address;

// create an udp socket handle, attach opaque with it . udp socket don't need call socket_server_start to recv message
// if port != 0, bind the socket . if addr == NULL, bind ipv4 0.0.0.0 . If you want to use ipv6, addr can be "::" and port 0.
int socket_server_udp(struct socket_server *, uintptr_t opaque, const char * addr, int port);
// set default dest address, return 0 when success
int socket_server_udp_connect(struct socket_server *, int id, const char * addr, int port);
// If the socket_udp_address is NULL, use last call socket_server_udp_connect address instead
// You can also use socket_server_send
int64_t socket_server_udp_send(struct socket_server *, int id, const struct socket_udp_address *, const void *buffer, int sz);
// extract the address of the message, struct socket_message * should be SOCKET_UDP
const struct socket_udp_address * socket_server_udp_address(struct socket_server *, struct socket_message *, int *addrsz);

// if you send package sz == -1, use soi.
void socket_server_userobject(struct socket_server *, struct socket_object_interface *soi);



#endif
