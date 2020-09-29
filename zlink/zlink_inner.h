#ifndef ZLINK_INNER_H
#define ZLINK_INNER_H

#include "ZLink.h"
#include "udt.h"
#include "utils.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <map>

using namespace std;

#define  CLIENT_SESSION_ID  (0)

#define ZLINK_SESSION_EXTERN_CHANS  (4)

class LinkSession
{
public:
    LinkSession()
    {
        sessionID = 0;
        strClientIP = "";
        strClientSN = "";
        sockExternChans.resize(ZLINK_SESSION_EXTERN_CHANS);
    }

    SYSSOCKET           sessionID;              //会话ID
    std::string         strClientIP;            //会话设备IP
    std::string         strClientSN;            //会话设备序列号

    UDTSOCKET           sockAudio;              //会话音频通道
    UDTSOCKET           sockVideo;              //会话视频通道
    vector<UDTSOCKET>   sockExternChans;         //扩展通道
};

class LinkManager
{
public:
    LinkManager()
    {
        m_nRole             = ZLINK_UNKNOWN;
        m_strNetCardName    = "";
        m_strLocalIP        = "";
        m_nLocalPort        = 0;
        m_isInit            = false;
        m_eventCallBackFunc = NULL;
        m_strUid            = "";
        m_strPwd            = "";
        m_strServerIP       = "";
        m_nServerPort       = 0;
        m_trdSearch         = 0;
        m_nTcpServer        = 0;
        m_nUdtServer        = 0;
        m_nSessionMaxCount  = 10;
        m_mapSessions.clear();
    }

    //![1] 服务端客户端通用属性 在ZLink_Init初始化除了UID和passwd
    ZLINK_ROLE      m_nRole;                //Zlink要初始化的角色
    std::string     m_strNetCardName;       //本地网卡名称
    std::string     m_strLocalIP;           //本地IP
    int             m_nLocalPort;           //本地绑定端口
    bool            m_isInit;               //是否已经完成主机或客户端初始化
    ZL_PF_ST_EVT    m_eventCallBackFunc;    //会话事件回调函数

    std::string     m_strUid;               //会话验证用户名
    std::string     m_strPwd;               //会话验证密码

    //![2] 客户端属性 ZLink_Client_Init初始化
    std::string     m_strServerIP;          //客户端填充服务端的IP
    int             m_nServerPort;          //客户端填充服务端的Port
//    LinkSession     m_clientInfo;

    //![3] 服务端属性 ZLink_Host_Init初始化
    pthread_t       m_trdSearch;            //服务器局域网广播线程
    SYSSOCKET       m_nTcpServer;           //服务器TCP监听socket
    UDTSOCKET       m_nUdtServer;           //服务器UDT监听socket
    int             m_nSessionMaxCount;     //最大会话数量
    map<int, LinkSession> m_mapSessions;    //服务端会话信息表

};

extern LinkManager *_g_link;

//! 检测zlink是否已经初始化
#define CHECK_ZLINK_LIB_INIT \
    do{ \
    if(_g_link == NULL) \
    return ZLINK_ERR_NOINIT; \
    }while(0)

//! 检测zlink是否初始化为HOST
#define CHECK_ZLINK_ROLE_HOST \
    do{ \
    if(_g_link->m_nRole != ZLINK_HOST) \
    return ZLINK_ERR_NOHOST; \
    }while(0)

//! 检测zlink是否初始化为CLIENT
#define CHECK_ZLINK_ROLE_CLIENT \
    do{ \
    if(_g_link->m_nRole != ZLINK_CLIENT) \
    return ZLINK_ERR_NOCLIENT; \
    }while(0)

//! 检测zlink是否已经客户端或者主机初始化
#define CHECK_ZLINK_ROLE_INIT \
    do{ \
    if(_g_link->m_isInit != true) \
    return ZLINK_ERR_ROLE_NOINIT; \
    }while(0)

//! 检测session是否存在
#define CHECK_ZLINK_SESSION(nSid) \
    do{ \
    if(0 == _g_link->m_mapSessions.count(nSid)) \
    {  return ZLINK_ERR_NOSESSION; } \
    }while(0)

int createServerSocket(LinkManager *link);

int closeServerSocket(LinkManager *link);

int setServerListen(LinkManager *link);

int AvFrameDataRecv(int nAVCid, char *pFrameData, int nFrameDataMaxSize, char *pFrameInfo, int nFrameInfoMaxSize, int nTimeoutMs, ZLINK_CHANNEL_TYPE chn);

int AvFrameDataSend(int nAVCid, char *pFrameData, int nFrameDataMaxSize, char *pFrameInfo, int nFrameInfoMaxSize, int nTimeoutMs, ZLINK_CHANNEL_TYPE chn);

int avHeaderSend(int nSid, ZLINK_CHANNEL_TYPE chn,int nTimeoutMs,int nFrameInfoSize, int nFrameDataSize);

int avHeaderRecv(int nSid, ZLINK_CHANNEL_TYPE chn,int nTimeoutMs,int *nFrameInfoSize, int *nFrameDataSize);

#endif // ZLINK_INNER_H
