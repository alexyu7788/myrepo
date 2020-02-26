#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <msg_broker.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	int fd;
	struct sockaddr_un local_addr;
	struct sockaddr_un remote_addr;
	char path[128];
} ConnContext;

#define CONN_ARRAY_SIZE 16

static ConnContext* ConnArray[CONN_ARRAY_SIZE] = { NULL };
static int conn_array_used_count = 0;
static pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;

static void MsgBroker_Release(void)
{
	int i;
	char path[128];
	long pid = (long)getpid();
	for (i = 0; i < conn_array_used_count; ++i)
	{
		if (ConnArray[i])
		{
			if (ConnArray[i]->fd >= 0)
				close(ConnArray[i]->fd);

			snprintf(path, 128, "%s.%ld", ConnArray[i]->path, pid);
			unlink(path);
			free(ConnArray[i]);
		}
	}
}

/* ========================================================================== */

static inline int init_client_conn_context(ConnContext* context)
{
	if ((context->fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0)) < 0)
		return -1;

	memset(&context->local_addr, 0, sizeof(struct sockaddr_un));
	context->local_addr.sun_family = AF_UNIX;
	snprintf(context->local_addr.sun_path, sizeof(context->local_addr.sun_path),
			"%s.%ld", context->path, (long)getpid());

	unlink(context->local_addr.sun_path);

	if (bind(context->fd, (struct sockaddr *) &context->local_addr, sizeof(struct sockaddr_un)) < 0)
	{
		printf("bind %s errno: %d\n", context->path, errno);
		close(context->fd);
		context->fd = -1;
		return -1;
	}

	memset(&context->remote_addr, 0, sizeof(struct sockaddr_un));
	context->remote_addr.sun_family = AF_UNIX;
	strncpy(context->remote_addr.sun_path, context->path, sizeof(context->remote_addr.sun_path) - 1);

	return 0;
}

static inline int init_server_conn_context(ConnContext* context)
{
	if ((context->fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0)) < 0)
		return -1;

	memset(&context->local_addr, 0, sizeof(struct sockaddr_un));
	context->local_addr.sun_family = AF_UNIX;
	unlink(context->path);
	strncpy(context->local_addr.sun_path, context->path, sizeof(context->local_addr.sun_path) - 1);

	if (bind(context->fd, (struct sockaddr *) &context->local_addr, sizeof(struct sockaddr_un)) < 0)
	{
		printf("bind %s errno: %d\n", context->path, errno);
		close(context->fd);
		context->fd = -1;
		return -1;
	}

	struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	if (setsockopt(context->fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval)))
	{
		printf("setsockopt errno: %d\n", errno);
		close(context->fd);
		context->fd = -1;
		return -1;
	}

	return 0;
}

void MsgBroker_Run(const char* path, MsgCallBack func, void* user_data, int* terminate)
{
	socklen_t addr_len;
	ConnContext conn_context;
	memset(&conn_context, 0x0, sizeof(ConnContext));
	strncpy(conn_context.path, path, 127);
	if (init_server_conn_context(&conn_context) < 0)
		return;

	int v;
	char msg[512];
	int* ptr;
	MsgContext msg_context;
	while (!(*terminate))
	{
		addr_len = sizeof(struct sockaddr_un);
		if ((v = recvfrom(conn_context.fd, msg, 512, 0,
						(struct sockaddr *) &conn_context.remote_addr, &addr_len)) > 0)
		{
			ptr = (int*) msg;
			msg_context.has_response = *ptr;
			++ptr;

			msg_context.host_len = *ptr;
			++ptr;
			if (msg_context.host_len)
			{
				msg_context.host = (char*) ptr;
				ptr += 4;
				msg_context.data_size = v - 24;
			}
			else
			{
				msg_context.data_size = v - 8;
				msg_context.host = "";
			}

			msg_context.cmd_len = *ptr;
			++ptr;

			if (msg_context.cmd_len)
			{
				msg_context.cmd = (char*)ptr;
				ptr += 4;
				msg_context.data_size -= 20;
			}
			else
			{
				msg_context.data_size -= 4;
				msg_context.cmd = "";
			}

			if (msg_context.data_size)
			{
				msg_context.data = (unsigned char*)ptr;
			}

			func(&msg_context, user_data);

			//printf("send to msg receiver\n");
			if (msg_context.has_response != 0)
			{
				do
				{
					v = sendto(conn_context.fd, msg_context.data, msg_context.data_size,
							0, (struct sockaddr *) &conn_context.remote_addr, sizeof(struct sockaddr_un));
				} while ((v < 0) && ((errno == EAGAIN) || (errno == EINTR)));
			}
		}
	}

	close(conn_context.fd);
	unlink(path);
}

int MsgBroker_SendMsg(const char* path, MsgContext* msg_context)
{
	ConnContext* context = NULL;

	//payload format: !!!!Important!!!!! all data section should be aligned in 4 bytes (This is ARM platform).
	//Note: the max. length of host and cmd is 16 (not including '\0');
	// | 4-bytes (has_response)
	// | 4-bytes (host len) | Host (string, Max. 16 bytes)
	// | 4-bytes (cmd len) | command (string, Max. 16 bytes)
	// | data (max. 256 bytes)
	char msg[512];
	int len;

	if ((msg_context->host_len > 16) || (msg_context->cmd_len > 16) ||
			(msg_context->data_size > 256))
	{
		printf("[%s] Error: Length or size exceed the limitation. host_len = %u <= 16, cmd_len = %u <= 16, data_size = %u <= 256. Message won't be sent.\n",
			__func__, msg_context->host_len, msg_context->cmd_len, msg_context->data_size);
		return -1;
	}

	pthread_mutex_lock(&msg_mutex);
	for (int i = 0; i < conn_array_used_count; ++i)
	{
		if ((ConnArray[i] != NULL) && (strncmp(path, ConnArray[i]->path, 127) == 0))
		{
			context = ConnArray[i];
			break;
		}
	}

	if (!context)
	{
		if (conn_array_used_count == 0)
		{
			atexit(MsgBroker_Release);
		}

		if(conn_array_used_count >= CONN_ARRAY_SIZE)
		{
			printf("[%s]: ConnArray is full.\n", __func__);
			pthread_mutex_unlock(&msg_mutex);
			return -1;
		}

		context = ConnArray[conn_array_used_count] = (ConnContext*) malloc(sizeof(ConnContext));
		if (context == NULL) {
			printf("[%s]: malloc fail.\n", __func__);
			pthread_mutex_unlock(&msg_mutex);
			return -1;
		}

		conn_array_used_count++;
		strncpy(context->path, path, 127);
		context->fd = -1;
	}

	if (context->fd < 0)
	{
		if (init_client_conn_context(context) < 0)
		{
			pthread_mutex_unlock(&msg_mutex);
			return -1;
		}
	}
	if (context->fd < 0)
	{
		pthread_mutex_unlock(&msg_mutex);
		return -1;
	}

	int* ptr = (int*)msg;
	*ptr = msg_context->has_response;
	++ptr;

	*ptr = msg_context->host_len;
	++ptr;
	if (msg_context->host_len)
	{
		memcpy(ptr, msg_context->host, msg_context->host_len);
		ptr += 4;
	}

	*ptr = msg_context->cmd_len;
	++ptr;
	if (msg_context->cmd_len)
	{
		memcpy(ptr, msg_context->cmd, msg_context->cmd_len);
		ptr += 4;
	}

	len = ((char*)ptr) - msg;
	if (msg_context->data_size)
	{
		memcpy(ptr, msg_context->data, msg_context->data_size);
		len += msg_context->data_size;
	}

	for(;;)
	{
		if (sendto(context->fd, msg, len, 0, (struct sockaddr *) &context->remote_addr, sizeof(struct sockaddr_un)) == len)
			break;

		if (errno != EINTR)
		{
			close(context->fd);
			context->fd = -1;
			pthread_mutex_unlock(&msg_mutex);
			return -1;
		}
	}

	len = 0;
	if (msg_context->has_response != 0)
	{
		//printf("wait for msg response\n");
		for(;;)
		{
			len = recvfrom(context->fd, (void*) msg_context->data, 256, 0, NULL, 0);
			if (len >= 0)
			{
				msg_context->data_size = len;
				len = 0;
				break;
			}

			if (errno != EINTR)
			{
				len = -1;
				break;
			}
		}
	}

	pthread_mutex_unlock(&msg_mutex);
	return len;
}
