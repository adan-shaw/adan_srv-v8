#include "io_event/socket_poll.h"

// custom malloc/free
//#define SOCKET_SERVER_FILE_MEMAPI   adan.h
//#define SOCKET_SERVER_MALLOC        adan_malloc
//#define SOCKET_SERVER_FREE          adan_free

#ifdef SOCKET_SERVER_MALLOC
#	define MALLOC 	SOCKET_SERVER_MALLOC
#else
#	define MALLOC 	malloc
#endif

#ifdef SOCKET_SERVER_FREE
#	define FREE 	SOCKET_SERVER_FREE
#else
#	define FREE 	free
#endif

#define SOCKET_DATA (0)
#define SOCKET_CLOSE (1)
#define SOCKET_OPEN (2)
#define SOCKET_ACCEPT (3)
#define SOCKET_ERROR (4)
#define SOCKET_EXIT (5)
#define SOCKET_UDP (6)

#ifdef SOCKET_SERVER_FILE_MEMAPI
#define  STRINIFY_(S)    #S
#define  STRINIFY(S)     STRINIFY_(S)
#include STRINIFY(SOCKET_SERVER_FILE_MEMAPI)
#undef   STRINIFY
#undef   STRINIFY_
#endif

#define MAX_INFO (128)
// MAX_SOCKET will be 2^MAX_SOCKET_P
#define MAX_SOCKET_P (16)
#define MAX_EVENT (64)
#define MIN_READ_BUFFER (64)
#define SOCKET_TYPE_INVALID (0)
#define SOCKET_TYPE_RESERVE (1)
#define SOCKET_TYPE_PLISTEN (2)
#define SOCKET_TYPE_LISTEN (3)
#define SOCKET_TYPE_CONNECTING (4)
#define SOCKET_TYPE_CONNECTED (5)
#define SOCKET_TYPE_HALFCLOSE (6)
#define SOCKET_TYPE_PACCEPT (7)
#define SOCKET_TYPE_BIND (8)

#define MAX_SOCKET (1<<(MAX_SOCKET_P))

#define PRIORITY_HIGH (0)
#define PRIORITY_LOW (1)

#define HASH_ID(id) (((unsigned)id) % (MAX_SOCKET))

#define PROTOCOL_TCP (0)
#define PROTOCOL_UDP (1)
#define PROTOCOL_UDPv6 (2)

#define UDP_ADDRESS_SIZE (19)		// ipv6 128bit + port 16bit + 1 byte type

#define MAX_UDP_PACKAGE (65535)

struct write_buffer
{
	struct write_buffer *next;
	void *buffer;
	char *ptr;
	int sz;
	bool userobject;
	uint8_t udp_address[UDP_ADDRESS_SIZE];
};

#define SIZEOF_TCPBUFFER (offsetof(struct write_buffer, udp_address[0]))
#define SIZEOF_UDPBUFFER (sizeof(struct write_buffer))

struct wb_list
{
	struct write_buffer *head;
	struct write_buffer *tail;
};

struct socket
{
	uintptr_t opaque;
	struct wb_list high;
	struct wb_list low;
	int64_t wb_size;
	int fd;
	int id;
	uint16_t protocol;
	uint16_t type;
	union
	{
		int size;
		uint8_t udp_address[UDP_ADDRESS_SIZE];
	} p;
};

struct socket_object_interface
{
	void *(*buffer) (void *);
	int (*size) (void *);
	void (*free) (void *);
};

struct socket_server
{
	int recvctrl_fd;
	int sendctrl_fd;
	int checkctrl;
	poll_fd event_fd;
	int alloc_id;
	int event_n;
	int event_index;
	struct socket_object_interface soi;
	struct event ev[MAX_EVENT];
	struct socket slot[MAX_SOCKET];
	char buffer[MAX_INFO];
	uint8_t udpbuffer[MAX_UDP_PACKAGE];
	fd_set rfds;
};

struct socket_message
{
	int id;
	uintptr_t opaque;
	int ud;												// for accept, ud is listen id ; for data, ud is size of data
	char *data;
};

struct request_open
{
	int id;
	int port;
	uintptr_t opaque;
	char host[1];
};

struct request_send
{
	int id;
	int sz;
	char *buffer;
};

struct request_send_udp
{
	struct request_send send;
	uint8_t address[UDP_ADDRESS_SIZE];
};

struct request_setudp
{
	int id;
	uint8_t address[UDP_ADDRESS_SIZE];
};

struct request_close
{
	int id;
	uintptr_t opaque;
};

struct request_listen
{
	int id;
	int fd;
	uintptr_t opaque;
	char host[1];
};

struct request_bind
{
	int id;
	int fd;
	uintptr_t opaque;
};

struct request_start
{
	int id;
	uintptr_t opaque;
};

struct request_setopt
{
	int id;
	int what;
	int value;
};

struct request_udp
{
	int id;
	int fd;
	int family;
	uintptr_t opaque;
};

/*
	The first byte is TYPE
	S Start socket
	B Bind socket
	L Listen socket
	K Close socket
	O Connect to (Open)
	X Exit
	D Send package (high)
	P Send package (low)
	A Send UDP package
	T Set opt
	U Create UDP socket
	C set udp address
 */

struct request_package
{
	uint8_t header[8];						// 6 bytes dummy
	union
	{
		char buffer[256];
		struct request_open open;
		struct request_send send;
		struct request_send_udp send_udp;
		struct request_close close;
		struct request_listen listen;
		struct request_bind bind;
		struct request_start start;
		struct request_setopt setopt;
		struct request_udp udp;
		struct request_setudp set_udp;
	} u;
	uint8_t dummy[256];
};

union sockaddr_all
{
	struct sockaddr s;
	struct sockaddr_in v4;
	struct sockaddr_in6 v6;
};

struct send_object
{
	void *buffer;
	int sz;
	void (*free_func) (void *);
};

/*
	// ctrl command only exist in local fd, so don't worry about endian.
	switch(type){
	case 'S':
		return start_socket(ss,(struct request_start *)buffer, result);
	case 'B':
		return bind_socket(ss,(struct request_bind *)buffer, result);
	case 'L':
		return listen_socket(ss,(struct request_listen *)buffer, result);
	case 'K':
		return close_socket(ss,(struct request_close *)buffer, result);
	case 'O':
		return open_socket(ss, (struct request_open *)buffer, result);
	case 'X':
		result->opaque = 0;
		result->id = 0;
		result->ud = 0;
		result->data = NULL;
		return SOCKET_EXIT;
	case 'D':
		return send_socket(ss, (struct request_send *)buffer, result, PRIORITY_HIGH, NULL);
	case 'P':
		return send_socket(ss, (struct request_send *)buffer, result, PRIORITY_LOW, NULL);
	case 'A': {
		rsu = (struct request_send_udp *)buffer;
		return send_socket(ss, &rsu->send, result, PRIORITY_HIGH, rsu->address);
	}
	case 'C':
		return set_udp_address(ss, (struct request_setudp *)buffer, result);
	case 'T':
		setopt_socket(ss, (struct request_setopt *)buffer);
		return -1;
	case 'U':
		add_udp_socket(ss, (struct request_udp *)buffer);
		return -1;
	default:
		fprintf(stderr, "socket-server: Unknown ctrl %c.\n",type);
		return -1;
	};
*/
