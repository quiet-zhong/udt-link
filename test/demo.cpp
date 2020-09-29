#include "demo.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>

#include <iostream>

using namespace std;

#include "udt.h"
#include "utils.h"

#define g_Remote_IP     "192.168.1.53"
#define g_Server_Port   9000




// Test basic data transfer.
#define g_TotalNum  10000

void* Test_1_Srv(void* param)
{
    (void) param;
    cout << "Testing simple data transfer.\n";

    UDTSOCKET serv = createUDTSocket(g_Server_Port);
    if (serv < 0)
        return NULL;

    UDT::listen(serv, 1024);
    sockaddr_storage clientaddr;
    int addrlen = sizeof(clientaddr);

    cout << "UDT::accept " << serv << endl;
    UDTSOCKET new_sock = UDT::accept(serv, (sockaddr*)&clientaddr, &addrlen);
    cout << "have a new udt client connected:" << new_sock << "->" << serv << endl;

    UDT::close(serv);

    if (new_sock == UDT::INVALID_SOCK)
    {
        return NULL;
    }

    int32_t buffer[g_TotalNum];
    fill_n(buffer, 0, g_TotalNum);

    int torecv = g_TotalNum * sizeof(int32_t);
    while (torecv > 0)
    {
        cout << "UDT::recv wait " << new_sock << endl;
        int rcvd = UDT::recv(new_sock, (char*)buffer + g_TotalNum * sizeof(int32_t) - torecv, torecv, 0);
        cout << "UDT::recv size=" << rcvd << endl;
        if (rcvd < 0)
        {
            cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
            return NULL;
        }
        torecv -= rcvd;
    }

    // check data
    for (int i = 0; i < g_TotalNum; ++ i)
    {
        if (buffer[i] != i)
        {
            cout << "DATA ERROR " << i << " " << buffer[i] << endl;
            break;
        }
    }

    int eid = UDT::epoll_create();
    UDT::epoll_add_usock(eid, new_sock);
    /*
   set<UDTSOCKET> readfds;
   if (UDT::epoll_wait(eid, &readfds, NULL, -1) > 0)
   {
      UDT::close(new_sock);
   }
   */

    UDTSOCKET readfds[1];
    int num = 1;
    if (UDT::epoll_wait2(eid, readfds, &num, NULL, NULL, -1) > 0)
    {
        UDT::close(new_sock);
    }

    return NULL;
}

void* Test_1_Cli(void* param)
{
    (void) param;
    UDTSOCKET client = createUDTSocket();
    if (client < 0)
        return NULL;

    udt_connect(client, g_Remote_IP, g_Server_Port);

    int32_t buffer[g_TotalNum];
    for (int i = 0; i < g_TotalNum; ++ i)
        buffer[i] = i;

    int tosend = g_TotalNum * sizeof(int32_t);
    while (tosend > 0)
    {
        int sent = UDT::send(client, (char*)buffer + g_TotalNum * sizeof(int32_t) - tosend, tosend, 0);
        cout << "UDT::send -> " << client << " size=" << sent << endl;
        if (sent < 0)
        {
            cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
            return NULL;
        }
        tosend -= sent;
    }

    UDT::close(client);
    return NULL;
}


// Test parallel UDT and TCP connections, over shared and dedicated ports.

#define g_UDTNum        200
#define g_IndUDTNum     100  // must < g_UDTNum.
#define g_TCPNum        10
int g_ActualUDTNum = 0;


void* Test_2_Srv(void* param)
{
    (void) param;
    cout << "Test parallel UDT and TCP connections.\n";

    //ignore SIGPIPE
    sigset_t ps;
    sigemptyset(&ps);
    sigaddset(&ps, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &ps, NULL);

    // create concurrent UDT sockets
    UDTSOCKET serv = createUDTSocket(g_Server_Port);
    if (serv < 0)
        return NULL;

    UDT::listen(serv, 1024);

    vector<UDTSOCKET> new_socks;
    new_socks.resize(g_UDTNum);

    int eid = UDT::epoll_create();

    for (int i = 0; i < g_UDTNum; ++ i)
    {
        sockaddr_storage clientaddr;
        int addrlen = sizeof(clientaddr);
        cout << "UDT::accept " << serv << endl;
        new_socks[i] = UDT::accept(serv, (sockaddr*)&clientaddr, &addrlen);
        cout << "have a new udt client connected:" << new_socks[i] << "->" << serv << endl;
        if (new_socks[i] == UDT::INVALID_SOCK)
        {
            cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
            return NULL;
        }
        UDT::epoll_add_usock(eid, new_socks[i]);
    }


    // create TCP sockets
    SYSSOCKET tcp_serv = createTCPSocket(g_Server_Port);
    if (tcp_serv < 0)
        return NULL;

    listen(tcp_serv, 1024);

    vector<SYSSOCKET> tcp_socks;
    tcp_socks.resize(g_TCPNum);

    for (int i = 0; i < g_TCPNum; ++ i)
    {
        sockaddr_storage clientaddr;
        socklen_t addrlen = sizeof(clientaddr);
        cout << "tcp accept " << tcp_serv << endl;
        tcp_socks[i] = accept(tcp_serv, (sockaddr*)&clientaddr, &addrlen);
        cout << "have a new tcp client connected:" << tcp_socks[i] << "->" << tcp_serv << endl;
        UDT::epoll_add_ssock(eid, tcp_socks[i]);
    }


    // using epoll to retrieve both UDT and TCP sockets
    set<UDTSOCKET> readfds;
    set<SYSSOCKET> tcpread;
    int count = g_UDTNum + g_TCPNum;
    while (count > 0)
    {
        UDT::epoll_wait(eid, &readfds, NULL, -1, &tcpread);
        for (set<UDTSOCKET>::iterator i = readfds.begin(); i != readfds.end(); ++ i)
        {
            int32_t data;
            cout << "UDT::recv wait " << *i << endl;
            int rcvd = UDT::recv(*i, (char*)&data, 4, 0);
            cout << "UDT::recv size=" << rcvd << endl;
            -- count;
        }

        for (set<SYSSOCKET>::iterator i = tcpread.begin(); i != tcpread.end(); ++ i)
        {
            int32_t data;
            cout << "tcp::recv wait" << *i << endl;
            int rcvd = recv(*i, (char*)&data, 4, 0);
            cout << "tcp::recv size=" << rcvd << endl;
            -- count;
        }
    }

    for (vector<UDTSOCKET>::iterator i = new_socks.begin(); i != new_socks.end(); ++ i)
    {
        UDT::close(*i);
    }

    for (vector<SYSSOCKET>::iterator i = tcp_socks.begin(); i != tcp_socks.end(); ++ i)
    {
        close(*i);
    }

    UDT::close(serv);
    close(tcp_serv);

    return NULL;
}

void* Test_2_Cli(void* param)
{
        (void) param;
    //ignore SIGPIPE
    sigset_t ps;
    sigemptyset(&ps);
    sigaddset(&ps, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &ps, NULL);

    // create UDT clients
    vector<UDTSOCKET> cli_socks;
    cli_socks.resize(g_UDTNum);

    // create UDT on individual ports
    for (int i = 0; i < g_IndUDTNum; ++ i)
    {
        if ((cli_socks[i] = createUDTSocket()) < 0)
        {
            cout << "socket: " << UDT::getlasterror().getErrorMessage() << endl;
            return NULL;
        }
    }

    // create UDT on shared port
    if ((cli_socks[g_IndUDTNum] = createUDTSocket()) < 0)
    {
        cout << "socket: " << UDT::getlasterror().getErrorMessage() << endl;
        return NULL;
    }

    sockaddr* addr = NULL;
    int size = 0;
    addr = (sockaddr*)new sockaddr_in;
    size = sizeof(sockaddr_in);
    UDT::getsockname(cli_socks[g_IndUDTNum], addr, &size);
    char sharedport[NI_MAXSERV];
    getnameinfo(addr, size, NULL, 0, sharedport, sizeof(sharedport), NI_NUMERICSERV);

    // Note that the first shared port has been created, so we start from g_IndUDTNum + 1.
    for (int i = g_IndUDTNum + 1; i < g_UDTNum; ++ i)
    {
        if ((cli_socks[i] = createUDTSocket(atoi(sharedport))) < 0)
        {
            cout << "socket: " << UDT::getlasterror().getErrorMessage() << endl;
            return NULL;
        }
    }
    for (vector<UDTSOCKET>::iterator i = cli_socks.begin(); i != cli_socks.end(); ++ i)
    {
        if (udt_connect(*i, g_Remote_IP, g_Server_Port) < 0)
        {
            cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
            return NULL;
        }
    }

    // create TCP clients
    vector<SYSSOCKET> tcp_socks;
    tcp_socks.resize(g_TCPNum);
    for (int i = 0; i < g_TCPNum; ++ i)
    {
        if ((tcp_socks[i] = createTCPSocket()) < 0)
        {
            return NULL;
        }

        int timeout_ms = 0;
        tcp_connect(tcp_socks[i], g_Remote_IP, g_Server_Port, timeout_ms);
    }

    // send data from both UDT and TCP clients
    int32_t data = 0;
    for (vector<UDTSOCKET>::iterator i = cli_socks.begin(); i != cli_socks.end(); ++ i)
    {
        int sent = UDT::send(*i, (char*)&data, 4, 0);
        cout << "UDT::send -> " << *i << " size=" << sent << endl;
        ++ data;
    }
    for (vector<SYSSOCKET>::iterator i = tcp_socks.begin(); i != tcp_socks.end(); ++ i)
    {
        int sent = send(*i, (char*)&data, 4, 0);
        cout << "tcp::send -> " << *i << " size=" << sent << endl;
        ++ data;
    }

    // close all client sockets
    for (vector<UDTSOCKET>::iterator i = cli_socks.begin(); i != cli_socks.end(); ++ i)
    {
        UDT::close(*i);
    }
    for (vector<SYSSOCKET>::iterator i = tcp_socks.begin(); i != tcp_socks.end(); ++ i)
    {
        close(*i);
    }

    return NULL;
}


// Test concurrent rendezvous connections.

#define g_UDTNum3   50

void* Test_3_Srv(void* param)
{
    (void) param;
    cout << "Test rendezvous connections.\n";

    vector<UDTSOCKET> srv_socks;
    srv_socks.resize(g_UDTNum3);

    int port = 61000;

    for (int i = 0; i < g_UDTNum3; ++ i)
    {
        if ( (srv_socks[i] = createUDTSocket(port ++, true)) < 0)
        {
            cout << "error\n";
        }
    }

    int peer_port = 51000;

    for (vector<UDTSOCKET>::iterator i = srv_socks.begin(); i != srv_socks.end(); ++ i)
    {
        udt_connect(*i, g_Remote_IP, peer_port ++);
    }

    for (vector<UDTSOCKET>::iterator i = srv_socks.begin(); i != srv_socks.end(); ++ i)
    {
        int32_t data = 0;
        cout << "UDT::recv wait " << *i << endl;
        int rcvd = UDT::recv(*i, (char*)&data, 4, 0);
        cout << "UDT::recv size=" << rcvd << endl;
    }

    for (vector<UDTSOCKET>::iterator i = srv_socks.begin(); i != srv_socks.end(); ++ i)
    {
        UDT::close(*i);
    }

    return NULL;
}

void* Test_3_Cli(void* param)
{
        (void) param;
    vector<UDTSOCKET> cli_socks;
    cli_socks.resize(g_UDTNum3);

    int port = 51000;
    for (int i = 0; i < g_UDTNum3; ++ i)
    {
        if ((cli_socks[i] = createUDTSocket(port ++, true)) < 0)
        {
            cout << "error\n";
        }
    }

    int peer_port = 61000;
    for (vector<UDTSOCKET>::iterator i = cli_socks.begin(); i != cli_socks.end(); ++ i)
    {
        udt_connect(*i, g_Remote_IP, peer_port ++);
    }

    int32_t data = 0;
    for (vector<UDTSOCKET>::iterator i = cli_socks.begin(); i != cli_socks.end(); ++ i)
    {
        int sent = UDT::send(*i, (char*)&data, 4, 0);
        cout << "UDT::send -> " << *i << " size=" << sent << endl;
        ++ data;
    }

    for (vector<UDTSOCKET>::iterator i = cli_socks.begin(); i != cli_socks.end(); ++ i)
    {
        UDT::close(*i);
    }

    return NULL;
}


// Test concurrent UDT connections in multiple threads.

#define g_UDTNum4       1000
#define g_UDTThreads    40
#define g_UDTPerThread  25

void* Test_4_Srv(void* param)
{
        (void) param;
    cout << "Test UDT in multiple threads.\n";

    UDTSOCKET serv = createUDTSocket(g_Server_Port);
    if (serv < 0)
        return NULL;

    UDT::listen(serv, 1024);

    vector<UDTSOCKET> new_socks;
    new_socks.resize(g_UDTNum4);

    for (int i = 0; i < g_UDTNum4; ++ i)
    {
        sockaddr_storage clientaddr;
        int addrlen = sizeof(clientaddr);
        cout << "UDT::accept" << endl;
        new_socks[i] = UDT::accept(serv, (sockaddr*)&clientaddr, &addrlen);
        cout << "have a new client connected:" << new_socks[i] << "->" << serv << endl;
        if (new_socks[i] == UDT::INVALID_SOCK)
        {
            cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
            return NULL;
        }
    }

    for (vector<UDTSOCKET>::iterator i = new_socks.begin(); i != new_socks.end(); ++ i)
    {
        UDT::close(*i);
    }

    UDT::close(serv);

    return NULL;

}

void* start_and_destroy_clients(void* param)
{
        (void) param;
    vector<UDTSOCKET> cli_socks;
    cli_socks.resize(g_UDTPerThread);

    if ((cli_socks[0] = createUDTSocket()) < 0)
    {
        cout << "socket: " << UDT::getlasterror().getErrorMessage() << endl;
        return NULL;
    }

    sockaddr* addr = NULL;
    int size = 0;

    addr = (sockaddr*)new sockaddr_in;
    size = sizeof(sockaddr_in);

    UDT::getsockname(cli_socks[0], addr, &size);
    char sharedport[NI_MAXSERV];
    getnameinfo(addr, size, NULL, 0, sharedport, sizeof(sharedport), NI_NUMERICSERV);

    for (int i = 1; i < g_UDTPerThread; ++ i)
    {
        if ((cli_socks[i] = createUDTSocket(atoi(sharedport))) < 0)
        {
            cout << "socket: " << UDT::getlasterror().getErrorMessage() << endl;
            return NULL;
        }
    }

    for (vector<UDTSOCKET>::iterator i = cli_socks.begin(); i != cli_socks.end(); ++ i)
    {
        if (udt_connect(*i, g_Remote_IP, g_Server_Port) < 0)
        {
            cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
            return NULL;
        }
    }

    for (vector<UDTSOCKET>::iterator i = cli_socks.begin(); i != cli_socks.end(); ++ i)
    {
        UDT::close(*i);
    }

    return NULL;
}


void* Test_4_Cli(void*)
{
    vector<pthread_t> cli_threads;
    cli_threads.resize(g_UDTThreads);

    for (vector<pthread_t>::iterator i = cli_threads.begin(); i != cli_threads.end(); ++ i)
    {
        pthread_create(&(*i), NULL, start_and_destroy_clients, NULL);
    }

    for (vector<pthread_t>::iterator i = cli_threads.begin(); i != cli_threads.end(); ++ i)
    {
        pthread_join(*i, NULL);
    }

    return NULL;
}


int demo_test(int index)
{
    if(index < 1 || index > 4)
    {
        std::cout << "test_case index error" << endl;
        return -1;
    }

    const int test_case = 4;

    void* (*Test_Srv[test_case])(void*);
    void* (*Test_Cli[test_case])(void*);

    Test_Srv[0] = Test_1_Srv;
    Test_Cli[0] = Test_1_Cli;
    Test_Srv[1] = Test_2_Srv;
    Test_Cli[1] = Test_2_Cli;
    Test_Srv[2] = Test_3_Srv;
    Test_Cli[2] = Test_3_Cli;
    Test_Srv[3] = Test_4_Srv;
    Test_Cli[3] = Test_4_Cli;

    cout << "Start Test # " << index << endl;

    int i = index-1;

    UDT::startup();

#ifdef UDT_SERVER
    pthread_t srv;
    pthread_create(&srv, NULL, Test_Srv[i], NULL);
    pthread_join(srv, NULL);
#else
    pthread_t cli;
    pthread_create(&cli, NULL, Test_Cli[i], NULL);
    pthread_join(cli, NULL);
#endif

    UDT::cleanup();
    cout << "Test # " << i + 1 << " completed." << endl;

    return 0;
}



int demo_server_with_epoll()
{
    cout << "server_test\n";

    int ret = -1;
    ret = UDT::startup();

    //ignore SIGPIPE
//    sigset_t ps;
//    sigemptyset(&ps);
//    sigaddset(&ps, SIGPIPE);
//    pthread_sigmask(SIG_BLOCK, &ps, NULL);

    int eid = UDT::epoll_create();
    if(eid == UDT::ERROR)
    {
        cout << "UDT::epoll_create error" << endl;
        return -1;
    }

    // create TCP sockets
    SYSSOCKET tcp_serv = createTCPSocket(g_Server_Port);
    if (tcp_serv < 0)
    {
        cout << "create TCP Socket error" << endl;
        return -2;
    }
    ret = listen(tcp_serv, 1024);
    ret = UDT::epoll_add_ssock(eid, tcp_serv);

    // create concurrent UDT sockets
    UDTSOCKET udt_serv = createUDTSocket(g_Server_Port);
    if (udt_serv < 0)
    {
        cout << "create UDT Socket error" << endl;
        return -3;
    }
    ret = UDT::listen(udt_serv, 1024);
    ret = UDT::epoll_add_usock(eid, udt_serv);

    // using epoll to retrieve both UDT and TCP sockets
    set<UDTSOCKET> udtrfds;
    set<SYSSOCKET> sysrfds;

    sockaddr_storage clientaddr;
    socklen_t addrlen = sizeof(clientaddr);

    char rdata[1024] = "";
    int  rlen = -1;

    while (1)
    {
        ret = UDT::epoll_wait(eid, &udtrfds, NULL, -1, &sysrfds, NULL);

//        cout << "have event " << udtrfds.size() << ":" << sysrfds.size() << endl;

        //! UDT event
        for (set<UDTSOCKET>::iterator i = udtrfds.begin(); i != udtrfds.end(); ++ i)
        {
            UDTSOCKET usock = *i;
            if(usock == udt_serv)
            {
                usock = UDT::accept(udt_serv, (sockaddr*)&clientaddr, (int *)&addrlen);
                cout << "have a new udt client connected:" << usock << "->" << udt_serv << endl;
                if (usock == UDT::INVALID_SOCK)
                {
                    cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
                    return -4;
                }
                ret = UDT::epoll_add_usock(eid, usock);
                continue;
            }

            memset(rdata, 0, sizeof(rdata));
            rlen = UDT::recv(usock, rdata, sizeof(rdata), 0);
            cout << "UDT::recv[" << rlen << "]:" << rdata << endl;
            if(rlen == -1)
            {
                UDT::close(usock);
                ret = UDT::epoll_remove_usock(eid, usock);
                cout << "udt disconnect: " << usock << endl;
            }
        }

        //! TCP event
        for (set<SYSSOCKET>::iterator i = sysrfds.begin(); i != sysrfds.end(); ++ i)
        {
            SYSSOCKET ssock = *i;
            if(ssock == tcp_serv)
            {
                ssock = accept(tcp_serv, (sockaddr*)&clientaddr, &addrlen);
                cout << "have a new tcp client connected:" << ssock << "->" << tcp_serv << endl;
                ret = UDT::epoll_add_ssock(eid, ssock);
                continue;
            }

            memset(rdata, 0, sizeof(rdata));
            rlen = recv(ssock, rdata, sizeof(rdata), 0);
            cout << "tcp::recv[" << rlen << "]:" << rdata << endl;
            if(rlen <= 0)
            {
                close(ssock);
                ret = UDT::epoll_remove_ssock(eid, ssock);
                cout << "tcp disconnect: " << ssock << endl;
            }
        }
    }

    ret = UDT::close(udt_serv);
    ret = close(tcp_serv);

    ret = UDT::cleanup();

    return ret;
}


int demo_server()
{
    cout << "server_test\n";

    int ret = -1;
    ret = UDT::startup();

    //ignore SIGPIPE
//    sigset_t ps;
//    sigemptyset(&ps);
//    sigaddset(&ps, SIGPIPE);
//    pthread_sigmask(SIG_BLOCK, &ps, NULL);

    SYSSOCKET tcp_serv;
    UDTSOCKET udt_serv;
    SYSSOCKET ssock;
    UDTSOCKET usock;

    char rdata[1024] = "";
    int  rlen = -1;

    sockaddr_storage clientaddr;
    socklen_t addrlen = sizeof(clientaddr);

//! [1]    ZLink_Host_Init
    //! create TCP sockets
    if ((tcp_serv = createTCPSocket(g_Server_Port)) < 0)
    {
        cout << "create TCP Socket error" << endl;
        return -2;
    }

    //! create concurrent UDT sockets
    if ((udt_serv = createUDTSocket(g_Server_Port)) < 0)
    {
        cout << "create UDT Socket error" << endl;
        return -3;
    }

//! [2]    ZLink_Host_Listen
    ret = listen(tcp_serv, 1024);
    ret = UDT::listen(udt_serv, 1024);

    while(1)
    {
        //! TCP accept
        ssock = accept(tcp_serv, (sockaddr*)&clientaddr, &addrlen);
        cout << "have a new tcp client connected:" << ssock << "->" << tcp_serv << endl;


        //! UDT accept
        usock = UDT::accept(udt_serv, (sockaddr*)&clientaddr, (int *)&addrlen);
        cout << "have a new udt client connected:" << usock << "->" << udt_serv << endl;
        if (usock == UDT::INVALID_SOCK)
        {
            cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
            return -4;
        }

        //! TCP recv
        memset(rdata, 0, sizeof(rdata));
        rlen = recv(ssock, rdata, sizeof(rdata), 0);
        cout << "tcp::recv[" << rlen << "]:" << rdata << endl;
        if(rlen <= 0)
        {
            close(ssock);
            cout << "tcp disconnect: " << ssock << endl;
        }

        //! UDT recv
        memset(rdata, 0, sizeof(rdata));
        rlen = UDT::recv(usock, rdata, sizeof(rdata), 0);
        cout << "UDT::recv[" << rlen << "]:" << rdata << endl;
        if(rlen == -1)
        {
            UDT::close(usock);
            cout << "udt disconnect: " << usock << endl;
        }
    }

    ret = UDT::close(udt_serv);
    ret = close(tcp_serv);

    ret = UDT::cleanup();

    return ret;
}

#define TRANS_CHAN_IOCTRL   0
#define TRANS_CHAN_AUDIO    1
#define TRANS_CHAN_VIDEO    2

int demo_client()
{
    int ret = -1;
    cout << "client_test\n";
    ret = UDT::startup();

    //ignore SIGPIPE
//    sigset_t ps;
//    sigemptyset(&ps);
//    sigaddset(&ps, SIGPIPE);
//    pthread_sigmask(SIG_BLOCK, &ps, NULL);

    // create UDT clients
    UDTSOCKET cli_socks[TRANS_CHAN_VIDEO+1];
    UDTSOCKET share_socket;
    char sharedport[NI_MAXSERV];

    sockaddr* addr = NULL;
    int size = 0;
    addr = (sockaddr*)new sockaddr_in;
    size = sizeof(sockaddr_in);

    // create TCP clients
    SYSSOCKET tcp_sock;
    if ((tcp_sock = createTCPSocket()) < 0)
    {
        return -1;
    }
    int timeout_ms = 0;
    ret = tcp_connect(tcp_sock, g_Remote_IP, g_Server_Port, timeout_ms);

    // create UDT on shared port
    if ((share_socket = createUDTSocket()) < 0)
    {
        cout << "socket: " << UDT::getlasterror().getErrorMessage() << endl;
        return -2;
    }

    ret = UDT::getsockname(share_socket, addr, &size);
    ret = getnameinfo(addr, size, NULL, 0, sharedport, sizeof(sharedport), NI_NUMERICSERV);

    for (int i = TRANS_CHAN_AUDIO; i <= TRANS_CHAN_VIDEO; i++)
    {
        if ((cli_socks[i] = createUDTSocket(atoi(sharedport))) < 0)
        {
            cout << "socket: " << UDT::getlasterror().getErrorMessage() << endl;
            return -3;
        }
        if ((ret = udt_connect(cli_socks[i], g_Remote_IP, g_Server_Port)) < 0)
        {
            cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
            return -4;
        }
    }

    for(int times=0; times < 1000; times++)
    {
        // send data from both UDT and TCP clients
        char data[1024] = "";
        int  slen = -1;

        memset(data, 0, sizeof(data));
        sprintf(data, "TCP===>%08d", times);

        slen = send(tcp_sock, data, strlen(data), 0);
        cout << "tcp::send -> " << tcp_sock << " size=" << slen << endl;

        for (int i = TRANS_CHAN_AUDIO; i <= TRANS_CHAN_VIDEO; i++)
        {
            memset(data, 0, sizeof(data));
            sprintf(data, "UDT[%d]===>%08d", i, times);

            slen = UDT::send(cli_socks[i], data, strlen(data), 0);
            cout << "UDT::send -> " << cli_socks[i] << " size=" << slen << endl;
        }
    }

    // close all client sockets
    ret = UDT::close(cli_socks[TRANS_CHAN_AUDIO]);
    ret = UDT::close(cli_socks[TRANS_CHAN_VIDEO]);
    ret = UDT::close(cli_socks[0]);

    ret = close(tcp_sock);

    ret = UDT::cleanup();

    return 0;
}
