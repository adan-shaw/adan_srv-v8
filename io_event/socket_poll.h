#ifndef socket_poll_h
#define socket_poll_h

#include <stdbool.h>

typedef int poll_fd;

struct event {
	void * s;
	bool read;
	bool write;
};

//c 语言不支持inline, 使用static 也是一个不错的选择, 可以减少函数变量压栈(你要编写一个跨平台程序, 不要用c++ 那套)
//static bool ev_invalid(poll_fd fd);
#define ev_invalid(sfd) ((sfd) == -1 ? 1 : 0)
static poll_fd ev_create();
//static void ev_release(poll_fd fd);
#define ev_release(sfd) (close((sfd)))
static int ev_add(poll_fd fd, int sock, void *ud);
static void ev_del(poll_fd fd, int sock);
static void ev_write(poll_fd, int sock, void *ud, bool enable);
static int ev_wait(poll_fd, struct event *e, int max);
static void ev_nonblocking(int sock);

#ifdef __linux__
#include "socket_epoll.h"
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
#include "socket_kqueue.h"
#endif

#endif
