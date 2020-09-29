#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
//#include <algorithm>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"

#include "ZLink.h"

#include <iostream>

using namespace std;

#define _IP_VERSION_    (AF_INET)
#define _SOCKET_TYPE_   (SOCK_STREAM)

#define SOCKET          int
#define SOCKADDR        struct sockaddr
#define SOCKADDR_IN 	struct sockaddr_in
#define IFRSIZE         ((int)(size*sizeof(struct ifreq)))

#define STRCASECMP(x,y)         strcasecmp(x,y)
#define STRNCASECMP             strncasecmp
#define STRTOK_S(x,y,z)     	strtok_r(x,y,z)
#define strcpy_s(x,y,z)     	strncpy(x,z,y)

#define MAX_TIMEOUT_MS     (5000)


//! ################################################
UDTSOCKET createUDTSocket(int port, bool rendezvous)
{
    UDTSOCKET usock = -1;
    addrinfo hints;
    addrinfo* res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = _IP_VERSION_;
    hints.ai_socktype = _SOCKET_TYPE_;

    char service[16];
    sprintf(service, "%d", port);
    if (0 != getaddrinfo(NULL, service, &hints, &res))
    {
        cout << "createUDTSocket illegal port number or port is busy.\n" << endl;
        return -1;
    }

    usock = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    cout << "create UDT Socket [sock=" << usock << "][port=" << port << "] rendezvous=" << rendezvous << endl;
    if(usock <= 0)
        return usock;

    // since we will start a lot of connections, we set the buffer size to smaller value.
    int snd_buf = 16000;
    int rcv_buf = 16000;
    UDT::setsockopt(usock, 0, UDT_SNDBUF, &snd_buf, sizeof(int));
    UDT::setsockopt(usock, 0, UDT_RCVBUF, &rcv_buf, sizeof(int));
    snd_buf = 8192;
    rcv_buf = 8192;
    UDT::setsockopt(usock, 0, UDP_SNDBUF, &snd_buf, sizeof(int));
    UDT::setsockopt(usock, 0, UDP_RCVBUF, &rcv_buf, sizeof(int));
    int fc = 16;
    UDT::setsockopt(usock, 0, UDT_FC, &fc, sizeof(int));
    bool reuse = true;
    UDT::setsockopt(usock, 0, UDT_REUSEADDR, &reuse, sizeof(bool));
    UDT::setsockopt(usock, 0, UDT_RENDEZVOUS, &rendezvous, sizeof(bool));

    if(port >= 0)
    {
        if (UDT::ERROR == UDT::bind(usock, res->ai_addr, res->ai_addrlen))
        {
            cout << "UDT::bind: " << UDT::getlasterror().getErrorMessage() << endl;
            UDT::close(usock);
            return -2;
        }
    }

    freeaddrinfo(res);
    return usock;
}

int udt_close(UDTSOCKET& usock)
{
    if(usock > 0)
    {
        UDT::close(usock);
        usock = 0;
    }
    return 0;
}

static int udt_settimeout(UDTSOCKET sock, int timeout_ms)
{
    struct timeval timeout = {0,0};
    bool isSync = true;
    if(timeout_ms > 0)
    {
        timeout.tv_sec = timeout_ms/1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        isSync = false;
    }
    int ret;
    ret = UDT::setsockopt(sock, 0, UDT_SNDSYN, &isSync, sizeof(bool));
    ret = UDT::setsockopt(sock, 0, UDT_RCVSYN, &isSync, sizeof(bool));

    ret = UDT::setsockopt(sock, 0, UDT_RCVTIMEO, &timeout, sizeof(timeout));
    ret = UDT::setsockopt(sock, 0, UDT_SNDTIMEO, &timeout, sizeof(timeout));

    return ret;
}

int udt_accept(UDTSOCKET sock, sockaddr *addr, int *addrlen, int timeout_ms)
{
    int ret = UDT::accept(sock, addr, addrlen);
    return ret;

#if 0
    udt_settimeout(sock, 0);

    int status = 0;
    int time = 0;
    if(timeout_ms <= 0)
    {
        status = UDT::accept(sock, addr, addrlen);
        return status;
    }

    do
    {
        errno = 0;
        status = UDT::accept(sock, addr, addrlen);
        if(status > 0)
        {
            return 0;
        }
        else if(status < 0 && errno == EINPROGRESS)
        {
            usleep(10 * 1000);
            time += 10;
            continue;
        }
        return -1;
    }while (time < timeout_ms);

    return -2; //超时
#endif
}

int udt_connect(UDTSOCKET& usock, const char *serverip, int port)
{
    addrinfo hints, *peer;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family =  _IP_VERSION_;
    hints.ai_socktype = _SOCKET_TYPE_;

    char buffer[16];
    sprintf(buffer, "%d", port);
    if (0 != getaddrinfo(serverip, buffer, &hints, &peer))
    {
        cout << "udt_connect illegal port number or port is busy." << endl;
        return -1;
    }

    UDT::connect(usock, peer->ai_addr, peer->ai_addrlen);
    cout << "UDT::connect -> " << serverip << ":" << port << endl;

    freeaddrinfo(peer);
    return 0;
}

int udt_send(UDTSOCKET &sock, const char *buf, size_t len, int timeout_ms)
{
    return UDT::send(sock, buf, len, timeout_ms);
}

int udt_recv(UDTSOCKET &sock, char *buf, size_t len, int timeout_ms)
{
    return UDT::recv(sock, buf, len, timeout_ms);
}

int udt_send_all(UDTSOCKET &sock, const char *buf, size_t len, int timeout_ms)
{
    int time = 0;
    size_t ssize = 0;
    int ss;
    do
    {
        ss = UDT::send(sock, buf + ssize, len - ssize, 0);
        if (UDT::ERROR == ss)
        {
            std::cout << "UDT::send error:" << UDT::getlasterror().getErrorMessage() << std::endl;
            break;
        }
        ssize += ss;

        if(timeout_ms > 0)
        {
            time += 10;
            usleep(10 * 1000);
            if(time >= timeout_ms)
            {
                return ZLINK_ERR_RECV_TIMEOUT;
            }
        }
    }while (ssize < len);

    return ssize;
}

int udt_recv_all(UDTSOCKET &sock, char *buf, size_t len, int timeout_ms)
{
    int time = 0;
    size_t ssize = 0;
    int ss;
    do
    {
        ss = UDT::recv(sock, buf + ssize, len - ssize, 0);
        if (UDT::ERROR == ss)
        {
            std::cout << "UDT::send error:" << UDT::getlasterror().getErrorMessage() << std::endl;
            break;
        }
        ssize += ss;

        if(timeout_ms > 0)
        {
            time += 10;
            usleep(10 * 1000);
            if(time >= timeout_ms)
            {
                return ZLINK_ERR_RECV_TIMEOUT;
            }
        }
    }while (ssize < len);

    return ssize;
}

//! ################################################
SYSSOCKET createTCPSocket(int port, bool rendezvous)
{
    SYSSOCKET ssock = -1;
    addrinfo hints;
    addrinfo* res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = _IP_VERSION_;
    hints.ai_socktype = _SOCKET_TYPE_;

    char service[16];
    sprintf(service, "%d", port);
    if (0 != getaddrinfo(NULL, service, &hints, &res))
    {
        cout << "createTCPSocket illegal port number or port is busy." << endl;
        return -1;
    }

    ssock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    cout << "create tcp Socket [sock=" << ssock << "][port=" << port << "]" << endl;
    if(ssock <= 0)
        return ssock;

    int reuse = 1;
    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

    if(port > 0)
    {
        if (bind(ssock, res->ai_addr, res->ai_addrlen) != 0)
        {
            cout << "tcp socket bind error" << endl;
            close(ssock);
            return -2;
        }
    }
    freeaddrinfo(res);
    return ssock;
}

int tcp_close(SYSSOCKET& ssock)
{
    if(ssock > 0)
    {
        close(ssock);
        ssock = 0;
    }
    return 0;
}

static int tcp_settimeout(SYSSOCKET sock, int timeout_ms)
{
    struct timeval timeout = {0,0};
    if(timeout_ms > 0 )
    {
        timeout.tv_sec = timeout_ms/1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
    }
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    return 0;
}

//函数：设置IO为非阻塞模式
static int tcp_set_nonblock(SYSSOCKET sock)
{
    int flags;
    flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0)
    {
        perror("fcntl(F_GETFL) failed");
        return -1;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("fcntl(F_SETFL) failed");
        return -2;
    }
    return 0;
}

//函数：设置IO为阻塞模式
static int tcp_set_block(SYSSOCKET sock)
{
    int flags;
    flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0)
    {
        perror("fcntl(F_GETFL) failed");
        return -1;
    }
    if (fcntl(sock, F_SETFL, flags&(~O_NONBLOCK) ) < 0)
    {
        perror("fcntl(F_SETFL) failed");
        return -2;
    }
    return 0;
}

int tcp_accept(SYSSOCKET sock, struct sockaddr *addr, socklen_t *addrlen, int timeout_ms)
{
    tcp_settimeout(sock, timeout_ms); //设置超时
    int ret = accept(sock, addr, addrlen);
    tcp_settimeout(sock, 0); //禁止超时
    return ret;
}

//返回0表示连接成功,-1表示连接失败,-2表示超时
int tcp_connect(SYSSOCKET& ssock, const char *serverip, int port, int timeout_ms)
{
    struct sockaddr_in addr;
    addr.sin_family = _IP_VERSION_;
    addr.sin_addr.s_addr = inet_addr(serverip);
    addr.sin_port = htons(port);

    int status = 0;
    int time = 0;
    if(timeout_ms <= 0)
    {
         status = connect(ssock, (struct sockaddr*)&addr, sizeof(addr));
         return status;
    }

    tcp_set_nonblock(ssock);
    do
    {
        errno = 0;
        status = connect(ssock, (struct sockaddr*)&addr, sizeof(addr));
        if(status == 0)
        {
            goto END;//成功
        }
        else if(status < 0 && errno == EINPROGRESS)
        {
            usleep(10 * 1000);
            time += 10;
            continue;
        }
        status = -1;
        goto END; //连接失败
    }while (time < timeout_ms);

    status = -2; //超时
END:
    cout << "tcp connect -> " << serverip << ":" << port << endl;
    tcp_set_block(ssock);
    return status;
}

int tcp_send(SYSSOCKET& sock, const void *buf, size_t len, int timeout_ms)
{
    if(timeout_ms <= 0)
    {
        tcp_settimeout(sock, 0); //禁止超时
        return send(sock, buf, len, 0);
    }
    int time = 0;
    do{
        errno = 0;
        int ret = send(sock, buf, len, MSG_DONTWAIT);
        if(ret >= 0)
        {
            return ret;
        }
        else if (ret < 0 && errno == EAGAIN)
        {
            usleep(1000 * 10);
            time += 10;
            continue;
        }
        return -1; //表示读取失败
    }while (time < timeout_ms);
    return -2; //超时
}

int tcp_recv(SYSSOCKET& sock, void *buf, size_t len, int timeout_ms)
{
    if(timeout_ms <= 0)
    {
        tcp_settimeout(sock, 0); //禁止超时
        return recv(sock, buf, len, 0);
    }

    int time = 0;
    do{
        errno = 0;
        int ret = recv(sock, buf, len, MSG_DONTWAIT);
        if(ret > 0)
        {
            return ret;
        }
        else if (ret < 0 && errno == EAGAIN)
        {
            usleep(1000 * 10);
            time += 10;
            continue;
        }
        return -1; //表示读取失败
    }while (time < timeout_ms);
    return -2; //超时
}







//! ###########################################################################
int splitString(const char *str, const char *split, char output[][64])
{
    char *p = NULL;
    char *pNext = NULL;
    int count = 0;
    char strBuff[4096] = "";
    if(strlen(str) > sizeof(strBuff))
        return -1;
    strcpy(strBuff, str);
    p = STRTOK_S(strBuff, split, &pNext);
    while (p)
    {
        strcpy( output[count++], p);
        p = STRTOK_S(NULL, split, &pNext);
    }
    return count;
}

//! 获取主机的IP地址
int getHostIpAddr(char strHostIp[][100])
{
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[2048];

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1)
    {
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
    {
        return -2;
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len/sizeof(struct ifreq));

    int count = 0;
    for (; it != end; ++it)
    {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0)
        {
            return -3;
        }
        else
        {
            if (!(ifr.ifr_flags & IFF_LOOPBACK))
            {   // don't count loopback
                if (ioctl(sock, SIOCGIFADDR, &ifr) == 0)
                {
                    //printf("IP address is: %s\n", inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr));
                }
                else
                { continue; }
                sprintf(strHostIp[count], "%s",
                        inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr));
                count++;
            }
        }
    }

    return count;
}

//! 广播查找设备,返回查找的数量
int socketFindResources(char ip[][100], size_t timeout_ms)
{
    int port = 9090;
    int ret;
    int count = 0;
    int hostIpCount = 0;
    int i;
    SOCKET sock[256];
    SOCKADDR_IN localAddr;
    SOCKADDR_IN remoteAddr;
    SOCKADDR_IN servaddr;
    char recvBuf[50];
    char strHostIpAddr[256][100];

    socklen_t len = sizeof(SOCKADDR);
    int bOpt = 1;

    hostIpCount = getHostIpAddr(strHostIpAddr);
    if (hostIpCount <= 0)
    {
        return -1;
    }

    for (i = 0; i < hostIpCount; i++)
    {
        sock[i] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (-1 == sock[i])
        {
            return -1;
        }
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(0);
        localAddr.sin_addr.s_addr  = inet_addr(strHostIpAddr[i]);

        ret = bind(sock[i], (SOCKADDR*)&localAddr, sizeof(SOCKADDR));
        if (-1 == ret)
        {
            return -1;
        }

        struct timeval timeout;
        timeout.tv_sec = timeout_ms/1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        ret = setsockopt(sock[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
        if (ret != 0)
        {
            return -1;
        }

        ret = setsockopt(sock[i], SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, (char*)&bOpt, sizeof(bOpt));
        if(ret != 0)
        {
            return -1;
        }
    }
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port);
    remoteAddr.sin_addr.s_addr = inet_addr("255.255.255.255");

    for (i = 0; i < hostIpCount; i++)
    {
        sendto(sock[i], "*?", 2, 0, (SOCKADDR*)&remoteAddr, sizeof(SOCKADDR));
        while (1)
        {
            memset(recvBuf, 0, 50);
            ret = recvfrom(sock[i], recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&servaddr, &len);
            if (ret > 0)
            {
                strcpy_s(ip[count], 100, inet_ntoa(servaddr.sin_addr));
                count++;
            }
            else
            {
                break;
            }
        }
    }
    for (i = 0; i < hostIpCount; i++)
    {
        close(sock[i]);
    }
    return count;
}

void *createLanSearchService(void *arg)
{
    (void)arg;

    int port = 9090;
    const char *resp = "Hualai Technologies";
    struct sockaddr_in  local,remote;
    char message[256] = {0};
    socklen_t len = sizeof(struct sockaddr_in);
    int socket_fd = 0;
    int option = 1;
    memset(&local, 0, sizeof(struct sockaddr_in));
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        return NULL;
    }
    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0)
    {
        return NULL;
    }
    if(bind(socket_fd, (struct sockaddr *)&local, sizeof(struct sockaddr)) == -1)
    {
        return NULL;
    }

    while(1)
    {
        if(recvfrom(socket_fd, message, sizeof(message), 0, (struct sockaddr *)&remote, &len) <= 0)
        {
            continue;
        }
        if(strncmp(message,"*?",2) == 0)//接受到的消息为 “*?”
        {
            sendto(socket_fd, resp, strlen(resp), 0, (struct sockaddr*)&remote, sizeof(remote));
            continue;
        }
    }
    close(socket_fd);
    return (void *)resp;
}

