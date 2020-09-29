#ifndef UTILS_H
#define UTILS_H

#include "udt.h"
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define MUTEX_INIT(mutex)    do{mutex = PTHREAD_MUTEX_INITIALIZER;}while(0)
#define MUTEX_LOCK(mutex)     pthread_mutex_lock(&mutex)
#define MUTEX_UNLOCK(mutex)   pthread_mutex_unlock(&mutex)

//创建UDT套接字,port为0或负数表示不绑定端口
UDTSOCKET createUDTSocket(int port = 0, bool rendezvous = false);

int udt_close(UDTSOCKET& usock);

int udt_accept(UDTSOCKET sock, struct sockaddr *addr, int *addrlen, int timeout_ms);

int udt_connect(UDTSOCKET& usock, const char *serverip, int port);

int udt_send(UDTSOCKET& sockfd, const char *buf, size_t len, int timeout_ms);

int udt_recv(UDTSOCKET& sock, char *buf, size_t len, int timeout_ms);


//创建TCP套接字,port为0或负数表示不绑定端口
SYSSOCKET createTCPSocket(int port = 0, bool rendezvous = false);

int tcp_close(SYSSOCKET& ssock);

int tcp_accept(SYSSOCKET sock, struct sockaddr *addr, socklen_t *addrlen, int timeout_ms);

int tcp_connect(SYSSOCKET& ssock, const char *serverip, int port, int timeout_ms);

int tcp_send(SYSSOCKET& sockfd, const void *buf, size_t len, int timeout_ms);

int tcp_recv(SYSSOCKET& sock, void *buf, size_t len, int timeout_ms);

int udt_send_all(UDTSOCKET &sock, const char *buf, size_t len, int timeout_ms);

int udt_recv_all(UDTSOCKET &sock, char *buf, size_t len, int timeout_ms);



int splitString(const char *str, const char *split, char output[][64]);

int getHostIpAddr(char strHostIp[][100]);

int socketFindResources(char ip[][100], size_t timeout_ms);

void *createLanSearchService(void *arg);

#endif // UTILS_H
