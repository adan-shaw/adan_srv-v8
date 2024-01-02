#ifndef poll_socket_kqueue_h
#define poll_socket_kqueue_h

#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//static bool ev_invalid(int kfd){ return kfd == -1; }

static int ev_create ()
{
	return kqueue ();
}

//static void ev_release(int kfd){ close(kfd); }

static void ev_del (int kfd, int sock)
{
	struct kevent ke;
	EV_SET (&ke, sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	kevent (kfd, &ke, 1, NULL, 0, NULL);
	EV_SET (&ke, sock, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	kevent (kfd, &ke, 1, NULL, 0, NULL);
}

static int ev_add (int kfd, int sock, void *ud)
{
	struct kevent ke;
	EV_SET (&ke, sock, EVFILT_READ, EV_ADD, 0, 0, ud);
	if (kevent (kfd, &ke, 1, NULL, 0, NULL) == -1)
	{
		return 1;
	}
	EV_SET (&ke, sock, EVFILT_WRITE, EV_ADD, 0, 0, ud);
	if (kevent (kfd, &ke, 1, NULL, 0, NULL) == -1)
	{
		EV_SET (&ke, sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		kevent (kfd, &ke, 1, NULL, 0, NULL);
		return 1;
	}
	EV_SET (&ke, sock, EVFILT_WRITE, EV_DISABLE, 0, 0, ud);
	if (kevent (kfd, &ke, 1, NULL, 0, NULL) == -1)
	{
		ev_del (kfd, sock);
		return 1;
	}
	return 0;
}

static void ev_write (int kfd, int sock, void *ud, bool enable)
{
	struct kevent ke;
	EV_SET (&ke, sock, EVFILT_WRITE, enable ? EV_ENABLE : EV_DISABLE, 0, 0, ud);
	if (kevent (kfd, &ke, 1, NULL, 0, NULL) == -1)
	{
		// todo: check error
		perror ("kevent()");
	}
}

static int ev_wait (int kfd, struct event *e, int max)
{
	int i, n;
	unsigned filter;
	struct kevent ev[max];
	n = kevent (kfd, NULL, 0, ev, max, NULL);

	for (i = 0; i < n; i++)
	{
		e[i].s = ev[i].udata;
		filter = ev[i].filter;
		e[i].write = (filter == EVFILT_WRITE);
		e[i].read = (filter == EVFILT_READ);
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
