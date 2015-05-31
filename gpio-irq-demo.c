// This code forked by Aliz Hammond from https://github.com/kieranc/power, which was forked from yfory's at https://github.com/yfory/power.
//
// This will count GPIO interrupts, and listen for TCP connections on port 1001. When a connection is made, it will print the number of
// interrupts since it was last queried.
//
// Compile with gcc gpio-irq-demo.c -o gpio-irq -lpthread
//
#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <pthread.h>

#define GPIO_FN_MAXLEN	32
#define POLL_TIMEOUT	600000
#define RDBUF_LEN	5

// for interoperability with systems that may lose precision, multiply the interrupt count by
// this constant before sending.
#define INTMULTIPLIER 1
#define TCPPORT 1001

void *tcpThreadEntry( void *param );

int initListenSocket()
{
	// Create our listening socket. Specify a backlog of zero, since we only want a single connection at
	// a time.
	struct sockaddr_in listenAddr;
	listenAddr.sin_family = AF_INET;
	listenAddr.sin_addr.s_addr = INADDR_ANY;
	listenAddr.sin_port = htons(TCPPORT);
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		printf("socket() failed, errno=%d\n", errno);
		return -1;
	}

	if (bind(s, (struct sockaddr*)&listenAddr, sizeof(listenAddr)) == -1) {
		printf("bind() failed, errno=%d\n", errno);
		return -1;
	}

	if (listen(s, 0) == -1) {
		printf("listen() failed, errno=%d\n", errno);
		return -1;
	}

	return s;
}

struct threadParams {
	int listenSocket;
	int* interCount;
	pthread_mutex_t* interCountMtx;
};

int startThread(int socket, int* toSend, pthread_mutex_t* toSendMtx)
{
	pthread_t tcpThread;

	struct threadParams* params = (struct threadParams*)malloc(sizeof(struct threadParams));
	params->listenSocket = socket;
	params->interCount = toSend;
	params->interCountMtx = toSendMtx;

	int success = pthread_create( &tcpThread, NULL, tcpThreadEntry, (void*)params);
	if (success) {
		printf("pthread_create failed, returned %d\n", success);
		return -1;
	}

	return 0;
}

void *tcpThreadEntry( void *param )
{
	struct threadParams* typedParam = (struct threadParams*) param;
	int listenSocket = typedParam->listenSocket;
	int* interCount = typedParam->interCount;
	pthread_mutex_t* interCountMtx = typedParam->interCountMtx;
	free(param);

	struct sockaddr_in from;
	int fromSize = sizeof(struct sockaddr_in);

	while( 1 ) {
		// Await connections..
		int connSock = accept(listenSocket, (struct sockaddr*)&from, &fromSize);
		if (connSock == -1) {
			printf("accept failed, errno=%d\n", errno);
			exit(-1);
		}
		char buf[101];

		// Unfortunately we have to hold the mutex for the duration of our send() here. This is needed
		// since we only want to reset the interrupt count on a successful send().
		pthread_mutex_lock( interCountMtx );
		int bytesToSend = snprintf(buf, 101, "%d", (*interCount)*INTMULTIPLIER );
		int bytesSent = send(connSock, buf, bytesToSend, 0);
		if ( bytesSent != bytesToSend) {
			printf("send() failed, wrote only %d of %d bytes\n", bytesSent, bytesToSend);
			// This isn't fatal - just go back to waiting for a new connection.
			pthread_mutex_unlock( interCountMtx);
			close(connSock);
			continue;
		}
		*interCount = 0;
		pthread_mutex_unlock( interCountMtx);

		close(connSock);
	}
}

int main(int argc, char **argv) {
	char fn[GPIO_FN_MAXLEN];
	int fd,ret;
	struct pollfd pfd;
	char rdbuf[RDBUF_LEN];
	int intCount = 0;
	pthread_mutex_t intCountMtx = PTHREAD_MUTEX_INITIALIZER;

	memset(rdbuf, 0x00, RDBUF_LEN);
	memset(fn, 0x00, GPIO_FN_MAXLEN);

	if(argc!=2) {
		printf("Usage: %s <GPIO>\nGPIO must be exported to sysfs and have enabled edge detection\n", argv[0]);
		return 1;
	}

	int listenSock = initListenSocket();
	if (listenSock == -1) {
		printf("Cannot initialise socket\n");
		return 1;
	}

	if (startThread(listenSock, &intCount, &intCountMtx) == -1) {
		printf("Cannot start TCP thread\n");
		return 1;
	}

	snprintf(fn, GPIO_FN_MAXLEN-1, "/sys/class/gpio/gpio%s/value", argv[1]);
	fd=open(fn, O_RDONLY);
	if(fd<0) {
		perror(fn);
		return 2;
	}
	pfd.fd=fd;
	pfd.events=POLLPRI;

	ret=read(fd, rdbuf, RDBUF_LEN-1);
	if(ret<0) {
		perror("read()");
		return 4;
	}

	while(1) {
		memset(rdbuf, 0x00, RDBUF_LEN);
		lseek(fd, 0, SEEK_SET);
		ret=poll(&pfd, 1, POLL_TIMEOUT);
		if(ret<0) {
			perror("poll()");
			close(fd);
			return 3;
		}
		if(ret==0) {
			continue;
		}

		ret=read(fd, rdbuf, RDBUF_LEN-1);
		if(ret<0) {
			perror("read()");
			return 4;
		}

		pthread_mutex_lock( &intCountMtx);
		intCount++;
		pthread_mutex_unlock( &intCountMtx);

	}
	close(fd);
	return 0;
}
