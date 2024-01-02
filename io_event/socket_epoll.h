#ifndef poll_socket_epoll_h
#define poll_socket_epoll_h

#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define EPOLL_FD_MAX (4096)

//由于static 函数不需要入栈, 这种操作也不需要用宏定义代替, 只是会移动一下函数指针
//static bool ev_invalid(int efd){ return efd == -1; }

static int ev_create ()
{
	return epoll_create (EPOLL_FD_MAX);
}

//static void ev_release(int efd){ close(efd); }

static int ev_add (int efd, int sock, void *ud)
{
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.ptr = ud;
	if (epoll_ctl (efd, EPOLL_CTL_ADD, sock, &ev) == -1)
	{
		return 1;
	}

	return 0;
}

static void ev_del (int efd, int sock)
{
	epoll_ctl (efd, EPOLL_CTL_DEL, sock, NULL);
}

static void ev_write (int efd, int sock, void *ud, bool enable)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | (enable ? EPOLLOUT : 0);
	ev.data.ptr = ud;
	epoll_ctl (efd, EPOLL_CTL_MOD, sock, &ev);
}

static int ev_wait (int efd, struct event *e, int max)
{
	int i, n;
	unsigned flag;
	struct epoll_event ev[max];
	n = epoll_wait (efd, ev, max, -1);
	for (i = 0; i < n; i++)
	{
		e[i].s = ev[i].data.ptr;
		flag = ev[i].events;
		e[i].write = (flag & EPOLLOUT) != 0;
		e[i].read = (flag & EPOLLIN) != 0;
	}

	return n;
}

static void ev_nonblocking (int fd)
{
	int flag = fcntl (fd, F_GETFL, 0);
	if (-1 == flag)
	{
		return;
	}

	fcntl (fd, F_SETFL, flag | O_NONBLOCK);
}

#endif
