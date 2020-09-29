#include "zlink_inner.h"

//! Zlink库版本号
const char *_zlink_version = "1.0.01";

static pthread_mutex_t _zlink_mutex = PTHREAD_MUTEX_INITIALIZER;
#define ZLINK_LOCK()    MUTEX_LOCK(_zlink_mutex)
#define ZLINK_UNLOCK()  MUTEX_UNLOCK(_zlink_mutex)

LinkManager *_g_link = NULL;

//! ######################### zlink lib api ##################################
int ZLink_Init(ZLINK_ROLE nRole, int nPort, char *NetCardName)
{
    ZLINK_LOCK();
    if(_g_link != NULL)
    {
        delete _g_link;
        _g_link = NULL;
    }

    char strHostIpAddr[16][100] = {""};
    _g_link = new LinkManager;

    _g_link->m_nRole = nRole;
    _g_link->m_strNetCardName = std::string(NetCardName);

    getHostIpAddr(strHostIpAddr);
    _g_link->m_strLocalIP = std::string(strHostIpAddr[0]);
    _g_link->m_nLocalPort = nPort;

    int ret = UDT::startup();
    ZLINK_UNLOCK();
    return ret;
}

int ZLink_DeInit()
{
    ZLINK_LOCK();
    if(_g_link != NULL)
    {
        delete _g_link;
        _g_link = NULL;
    }

    UDT::cleanup();
    ZLINK_UNLOCK();
    return 0;
}

const char *ZLink_GetVersion()
{
    return _zlink_version;
}

//! ######################### host api ##################################
int ZLink_Host_Init(const char *pUid, const char *pPwd)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_HOST;

    if(_g_link->m_isInit == true)
    {
        return 0;
    }

    ZLINK_LOCK();
    _g_link->m_strUid = std::string(pUid);
    _g_link->m_strPwd = std::string(pPwd);

    int ret = createServerSocket(_g_link);
    if (ret < 0)
    {
        cout << "createServerSocket error" << endl;
        ZLINK_UNLOCK();
        return -1;
    }

    setServerListen(_g_link);

    pthread_create(&_g_link->m_trdSearch, NULL, createLanSearchService, NULL);

    _g_link->m_mapSessions.clear();
    _g_link->m_isInit = true;

    ZLINK_UNLOCK();

    return 0;
}

int ZLink_Host_DeInit()
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_HOST;
    CHECK_ZLINK_ROLE_INIT;

    ZLINK_LOCK();

    pthread_cancel(_g_link->m_trdSearch);
    pthread_join(_g_link->m_trdSearch, NULL);

    closeServerSocket(_g_link);

    //关闭所有session
    map<int, LinkSession>::reverse_iterator iter;
    for(iter = _g_link->m_mapSessions.rbegin(); iter != _g_link->m_mapSessions.rend(); iter++)
    {
        LinkSession session = iter->second;
        ZLink_Session_Close(session.sessionID);
    }
    _g_link->m_mapSessions.clear();
    _g_link->m_isInit = false;
    ZLINK_UNLOCK();
    return 0;
}

//返回session id
int ZLink_Host_Listen(int nTimeoutMs)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_HOST;
    CHECK_ZLINK_ROLE_INIT;

    ZLINK_LOCK();
    int ret;
    char data[1024] = "";
    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);

    LinkSession session;

    //! TCP accept
    session.sessionID = tcp_accept(_g_link->m_nTcpServer, (sockaddr*)&clientaddr, &addrlen, nTimeoutMs);
    if (session.sessionID <= 0)
    {
        // cout << "tcp_accept error: " << session.sessionID << endl;
        ZLINK_UNLOCK();
        return -1;
    }

    session.strClientIP = std::string(inet_ntoa(clientaddr.sin_addr));

    cout << "new client request ip:" << session.strClientIP << endl
         << "the ctrl channel connected:" << session.sessionID << "->" << _g_link->m_nTcpServer << endl;

    //! UDT Audio accept
    session.sockAudio = udt_accept(_g_link->m_nUdtServer, (sockaddr*)&clientaddr, (int *)&addrlen, nTimeoutMs);
    cout << "the audio channel connected:" << session.sockAudio << "->" << _g_link->m_nUdtServer << endl;
    if (session.sockAudio == UDT::INVALID_SOCK)
    {
        cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
        tcp_close(session.sessionID);
        ZLINK_UNLOCK();
        return -2;
    }

    //! UDT Video accept
    session.sockVideo = udt_accept(_g_link->m_nUdtServer, (sockaddr*)&clientaddr, (int *)&addrlen, nTimeoutMs);
    cout << "the video channel connected:" << session.sockVideo << "->" << _g_link->m_nUdtServer << endl;
    if (session.sockVideo == UDT::INVALID_SOCK)
    {
        cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
        tcp_close(session.sessionID);
        udt_close(session.sockAudio);
        ZLINK_UNLOCK();
        return -3;
    }

    int sid = session.sessionID;
    //! 接收客户端发送的用户名、密码和序列号, 如:"root::123456::HL120001"
    ret = tcp_recv(sid, data, sizeof(data), 2000);
    cout << "tcp_recv: " << ret << " : " << data << endl;
    if(ret <= 0)
    {
        tcp_close(session.sessionID);
        udt_close(session.sockAudio);
        udt_close(session.sockVideo);
        ZLINK_UNLOCK();
        return -1;
    }

    //! 验证名户名和密码，并发送验证结果OK或ERROR
    char strVal[16][64] = {""};
    splitString(data, "::", strVal);
    cout << "UID::PASSWD::SN==>" << strVal[0] << ":" << strVal[1] << ":" << strVal[2] << endl;

    session.strClientSN = std::string(strVal[2]);

    if( (_g_link->m_strUid != std::string(strVal[0]))
            || (_g_link->m_strPwd != std::string(strVal[1])) )
    {
        cout << "UID or PASSWORD error, disconnect " << endl;
        memset(data, 0, sizeof(data));
        strcpy(data, "ERROR");
        ret = tcp_send(sid, data, strlen(data), 2000);
        cout << "tcp::send[" << sid << "][" << ret << "]:" << data << endl;

        tcp_close(session.sessionID);
        udt_close(session.sockAudio);
        udt_close(session.sockVideo);
        ZLINK_UNLOCK();
        return -4;
    }

    memset(data, 0, sizeof(data));
    strcpy(data, "OK");
    ret = tcp_send(sid, data, strlen(data), 2000);
    cout << "tcp::send[" << sid << "][" << ret << "]:" << data << endl;

    _g_link->m_mapSessions.insert(pair<int, LinkSession>(sid, session));

    ZLINK_UNLOCK();
    return sid;
}

int ZLink_Host_Listen_Exit();

int ZLink_Host_Set_Client_Concurrent_Size(int size)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_HOST;
    if(size > 0)
    {
        _g_link->m_nSessionMaxCount = size;
    }
    return 0;
}

// 返回 channel id
int ZLink_AV_Host_Start(int nSid, int nTimeoutMs, int nHostType, ZLINK_CHANNEL_TYPE nChn)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_HOST;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    return nSid;
}

int ZLink_AV_Host_Exit(int nSid, ZLINK_CHANNEL_TYPE nChn)
{
    return ZLINK_ERR_FAIL_INVALID_API;
}

int ZLink_AV_Host_Stop(int nAVCid)
{
    return ZLINK_ERR_FAIL_INVALID_API;
}


//! ####################### client api ####################################
int ZLink_Client_Lan_Search(ZLink_LanSearchInfo *pLanSearchInfo, int nArrayLen, int nTimeoutMs)
{
    ZLINK_LOCK();
    char ip[32][100] = {""};
    int count = socketFindResources(ip, nTimeoutMs);
    for(int i=0; i<count && i < nArrayLen; i++)
    {
        strcpy(pLanSearchInfo[i].ip, ip[i]);
    }
    ZLINK_UNLOCK();
    return count;
}

int ZLink_Client_Init(const char *pUid, const char *pPwd, char *IP, int port)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_CLIENT;

    ZLINK_LOCK();
    if(_g_link->m_isInit == true)
        ZLink_Client_DeInit();

    _g_link->m_strUid = std::string(pUid);
    _g_link->m_strPwd = std::string(pPwd);

    _g_link->m_strServerIP = string(IP);
    _g_link->m_nServerPort = port;

    LinkSession session;

    session.sessionID = CLIENT_SESSION_ID;
    session.strClientIP = _g_link->m_strLocalIP;  //获取本地IP赋值进去
    session.strClientSN = "";  //ZLink_Client_Connect 填充

    if ((session.sessionID = createTCPSocket()) < 0)
    {
        ZLINK_UNLOCK();
        return -2;
    }

    if ((session.sockAudio = createUDTSocket()) < 0)
    {
        tcp_close(session.sessionID);
        ZLINK_UNLOCK();
        return -3;
    }

    if ((session.sockVideo = createUDTSocket()) < 0)
    {
        tcp_close(session.sessionID);
        udt_close(session.sockAudio);
        ZLINK_UNLOCK();
        return -4;
    }

    _g_link->m_mapSessions.insert(pair<int, LinkSession>(CLIENT_SESSION_ID, session));

    _g_link->m_isInit = true;
    ZLINK_UNLOCK();
    return 0;
}

int ZLink_Client_DeInit()
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_CLIENT;
    CHECK_ZLINK_ROLE_INIT;

    ZLINK_LOCK();
    if(0 != _g_link->m_mapSessions.count(CLIENT_SESSION_ID))
    {
        tcp_close(_g_link->m_mapSessions.at(CLIENT_SESSION_ID).sessionID);
        udt_close(_g_link->m_mapSessions.at(CLIENT_SESSION_ID).sockAudio);
        udt_close(_g_link->m_mapSessions.at(CLIENT_SESSION_ID).sockVideo);
    }
    _g_link->m_mapSessions.clear();
    _g_link->m_isInit = false;
    ZLINK_UNLOCK();
    return 0;
}

//返回session id
int ZLink_Client_Connect(int nCid, char *cName)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_CLIENT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(CLIENT_SESSION_ID);

    int ret = -1;
    ZLINK_LOCK();
    LinkSession session = _g_link->m_mapSessions.at(CLIENT_SESSION_ID);
    session.strClientSN = std::string(cName);

    int timeout_ms = -1;
    ret = tcp_connect(session.sessionID, _g_link->m_strServerIP.c_str(), _g_link->m_nServerPort, timeout_ms);
    if(ret < 0)
    {
        cout << "tcp_connect: " << UDT::getlasterror().getErrorMessage() << endl;
        ZLINK_UNLOCK();
        return -1;
    }

    if ((ret = udt_connect(session.sockAudio, _g_link->m_strServerIP.c_str(), _g_link->m_nServerPort)) < 0)
    {
        cout << "udt_connect: " << UDT::getlasterror().getErrorMessage() << endl;
        ZLINK_UNLOCK();
        return -2;
    }
    if ((ret = udt_connect(session.sockVideo, _g_link->m_strServerIP.c_str(), _g_link->m_nServerPort)) < 0)
    {
        cout << "udt_connect: " << UDT::getlasterror().getErrorMessage() << endl;
        ZLINK_UNLOCK();
        return -3;
    }

    //! 发送用户名和密码
    char data[1024] = "";
    memset(data, 0, sizeof(data));
    snprintf(data, sizeof(data), "%s::%s::%s",
             _g_link->m_strUid.c_str(),_g_link->m_strPwd.c_str(),session.strClientSN.c_str());
    ret = tcp_send(session.sessionID, data, strlen(data), 2000);
    cout << "tcp::send[" << session.sessionID << "][" << ret
         << "]:" << data << endl;

    memset(data, 0, sizeof(data));
    ret = tcp_recv(session.sessionID, data, sizeof(data), 2000);
    cout << "tcp::recv[" << ret << "]:" << data << endl;

    if(ret > 0 || std::string(data) == std::string("OK"))
    {
        ZLINK_UNLOCK();
        return 0;
    }
    ZLINK_UNLOCK();
    return -4;
}

int ZLink_Client_Connect_Exit(int nCid)
{
    return ZLINK_ERR_FAIL_INVALID_API;
}

// 返回 channel id
int ZLink_AV_Client_Start(int nSid, int nTimeoutMs, int *pnHostType, ZLINK_CHANNEL_TYPE nChn, int nResend)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_SESSION(nSid);

    return nSid;
}

int ZLink_AV_Client_Exit(int nSid, ZLINK_CHANNEL_TYPE nChn)
{
    return ZLINK_ERR_FAIL_INVALID_API;
}

int ZLink_AV_Client_Stop(int nAVCid)
{
    return ZLINK_ERR_FAIL_INVALID_API;
}


//! ######################### session api ##################################
int ZLink_Set_Event_Callback(ZL_PF_ST_EVT cb)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    _g_link->m_eventCallBackFunc = cb;
    return 0;
}

int ZLink_Session_Close(int nSid)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    ZLINK_LOCK();
    LinkSession session = _g_link->m_mapSessions.at(nSid);
    tcp_close(session.sessionID);
    udt_close(session.sockAudio);
    udt_close(session.sockVideo);

    _g_link->m_mapSessions.erase(nSid);
    ZLINK_UNLOCK();
    return 0;
}

int ZLink_Session_Read(int nSid, char *pBuf, int nMaxBufSize, int nTimeoutMs, ZLINK_CHANNEL_TYPE nChn)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    int ret = -1;
    LinkSession session = _g_link->m_mapSessions.at(nSid);
    switch (nChn)
    {
    case ZLINK_CHANNEL_IOCTRL:
        ret = tcp_recv(session.sessionID, pBuf, nMaxBufSize, nTimeoutMs);
        break;
    case ZLINK_CHANNEL_AUDIO:
        ret = udt_recv_all(session.sockAudio, pBuf, nMaxBufSize, nTimeoutMs);
        break;
    case ZLINK_CHANNEL_VIDEO:
        ret = udt_recv_all(session.sockVideo, pBuf, nMaxBufSize, nTimeoutMs);
        break;
    case ZLINK_CHANNEL_RELIABLE:

        break;
    default:
        break;
    }

    return ret;
}

int ZLink_Session_Write(int nSid, char *pBuf, int nBufSize, int nTimeoutMs, ZLINK_CHANNEL_TYPE nChn)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    int ret = -1;
    LinkSession session = _g_link->m_mapSessions.at(nSid);
    switch (nChn)
    {
    case ZLINK_CHANNEL_IOCTRL:
        ret = tcp_send(session.sessionID, pBuf, nBufSize, nTimeoutMs);
        break;
    case ZLINK_CHANNEL_AUDIO:
        ret = udt_send_all(session.sockAudio, pBuf, nBufSize, nTimeoutMs);
        break;
    case ZLINK_CHANNEL_VIDEO:
        ret = udt_send_all(session.sockVideo, pBuf, nBufSize, nTimeoutMs);
        break;
    case ZLINK_CHANNEL_RELIABLE:

        break;
    default:
        break;
    }

    return ret;
}

int ZLink_Session_GetInfo(int nSid, ZLink_SessionInfo *pInfo)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    ZLINK_LOCK();
    LinkSession session = _g_link->m_mapSessions.at(nSid);

    memset(pInfo, 0, sizeof(ZLink_SessionInfo));
    strncpy(pInfo->IP, session.strClientIP.c_str(), sizeof(pInfo->IP));
    strncpy(pInfo->name, session.strClientSN.c_str(), sizeof(pInfo->name));
    ZLINK_UNLOCK();
    return 0;
}

int ZLink_Session_GetState(int nSid)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);
    return 0;
}

int ZLink_AV_Check_Normal_SendBuf(int nSid)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    int len;
    int var_size = sizeof(int);
    LinkSession session = _g_link->m_mapSessions.at(nSid);
    UDT::getsockopt(session.sockAudio, 0, UDT_SNDDATA, &len, &var_size);
    UDT::getsockopt(session.sockVideo, 0, UDT_SNDDATA, &len, &var_size);
    return len;
}

#if 01
//! ############################################################################

int ZLink_Session_Get_Free_Channel(int nSid);

int ZLink_AV_Check_Reliable_SendBuf(int nSid)
{
    return ZLink_AV_Check_RecvVideoBuf(nSid);
}

int ZLink_Set_RxCache_Size_Reliable(int nSid, int nSize)
{
    return ZLink_Set_RxCache_Size_Normal(nSid, nSize);
}

int ZLink_Set_TxCache_Size_Reliable(int nSid, int nSize)
{
    return ZLink_Set_TxCache_Size_Normal(nSid, nSize);
}

int ZLink_Set_RxCache_Size_Normal(int nSid, int nSize)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    LinkSession session = _g_link->m_mapSessions.at(nSid);
    UDT::setsockopt(session.sockAudio, 0, UDT_RCVBUF, &nSize, sizeof(int));
    UDT::setsockopt(session.sockVideo, 0, UDT_RCVBUF, &nSize, sizeof(int));
    return 0;
}

int ZLink_Set_TxCache_Size_Normal(int nSid, int nSize)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    LinkSession session = _g_link->m_mapSessions.at(nSid);
    UDT::setsockopt(session.sockAudio, 0, UDT_SNDBUF, &nSize, sizeof(int));
    UDT::setsockopt(session.sockVideo, 0, UDT_SNDBUF, &nSize, sizeof(int));
    return 0;
}

//! ############################################################################
#endif


//! ####################### channel api ####################################
int ZLink_AV_Recv_IOCtrl(int nAVCid, unsigned int *nIOType, char *pIOData, int nIODataMaxSize, int nTimeoutMs)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nAVCid);

    int ret = -1;
    int nSid = nAVCid;
    char strType[10] = "";
    char data[ZLINK_MAX_IOCTRL_BUF_SIZE] = "";
    ret = ZLink_Session_Read(nSid, data, ZLINK_MAX_IOCTRL_BUF_SIZE, nTimeoutMs, ZLINK_CHANNEL_IOCTRL);
    if(ret > 0)
    {
        memcpy(strType, data, 8);
        *nIOType = atoi(strType);
        memcpy(pIOData, data+8, nIODataMaxSize);
    }
    return ret;
}

int ZLink_AV_Send_IOCtrl(int nAVCid, unsigned int nIOType, char *pIOData, int nIODataSize, int nTimeoutMs)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nAVCid);

    int ret = -1;
    int nSid = nAVCid;
    char data[ZLINK_MAX_IOCTRL_BUF_SIZE] = "";
    sprintf(data, "%08d", nIOType);
    memcpy(data+8, pIOData, nIODataSize);
    ret = ZLink_Session_Write(nSid, data, nIODataSize + 8, nTimeoutMs, ZLINK_CHANNEL_IOCTRL);
    return ret;
}

int ZLink_AV_Recv_AudioData(int nAVCid, char *pAudioData, int nAudioDataMaxSize, char *pFrameInfo, int nFrameInfoMaxSize, int nTimeoutMs)
{
    return AvFrameDataRecv(nAVCid, pAudioData, nAudioDataMaxSize, pFrameInfo, nFrameInfoMaxSize, nTimeoutMs, ZLINK_CHANNEL_AUDIO);
}

int ZLink_AV_Send_AudioData(int nAVCid, char *pAudioData, int nAudioDataSize, char *pFrameInfo, int nFrameInfoSize, int nTimeoutMs)
{
    return AvFrameDataSend(nAVCid, pAudioData, nAudioDataSize, pFrameInfo, nFrameInfoSize, nTimeoutMs, ZLINK_CHANNEL_AUDIO);
}

int ZLink_AV_Recv_FrameData(int nAVCid, char *pFrameData, int nFrameDataMaxSize, char *pFrameInfo, int nFrameInfoMaxSize, int nTimeoutMs)
{
    return AvFrameDataRecv(nAVCid, pFrameData, nFrameDataMaxSize, pFrameInfo, nFrameInfoMaxSize, nTimeoutMs, ZLINK_CHANNEL_VIDEO);
}

int ZLink_AV_Send_FrameData(int nAVCid, char *pFrameData, int nFrameDataSize, char *pFrameInfo, int nFrameInfoSize, int nTimeoutMs)
{
    return AvFrameDataSend(nAVCid, pFrameData, nFrameDataSize, pFrameInfo, nFrameInfoSize, nTimeoutMs, ZLINK_CHANNEL_VIDEO);
}

int ZLink_AV_Check_RecvAudioBuf(int nAVCid)
{
    int nSid = nAVCid;
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    int len;
    int var_size = sizeof(int);
    LinkSession session = _g_link->m_mapSessions.at(nSid);
    UDT::getsockopt(session.sockAudio, 0, UDT_RCVDATA, &len, &var_size);
    return len;
}

int ZLink_AV_Check_RecvVideoBuf(int nAVCid)
{
    int nSid = nAVCid;
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    int len;
    int var_size = sizeof(int);
    LinkSession session = _g_link->m_mapSessions.at(nSid);
    UDT::getsockopt(session.sockVideo, 0, UDT_RCVDATA, &len, &var_size);
    return len;
}

#if 01
//! ############################################################################

int ZLink_AV_Clean_Local_AudioBuf(int nAVCid)
{
    int nSid = nAVCid;
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    LinkSession session = _g_link->m_mapSessions.at(nSid);
    struct timeval tmOut;
    tmOut.tv_sec = 0;
    tmOut.tv_usec = 0;

    UDT::UDSET fds;
    UD_ZERO( &fds );
    UD_SET( session.sockAudio, &fds );
    int nRet;
    char tmp[2];
    memset(tmp, 0, sizeof( tmp ) );
    while(1)
    {
        nRet = UDT::select( FD_SETSIZE, &fds, NULL, NULL, &tmOut );
        if(nRet== 0)
            break;
        UDT::recv(session.sockAudio, tmp, 1, 0);
    }
    return 0;
}

int ZLink_AV_Clean_Local_VideoBuf(int nAVCid)
{
    int nSid = nAVCid;
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nSid);

    LinkSession session = _g_link->m_mapSessions.at(nSid);
    struct timeval tmOut;
    tmOut.tv_sec = 0;
    tmOut.tv_usec = 0;

    UDT::UDSET fds;
    UD_ZERO( &fds );
    UD_SET( session.sockVideo, &fds );
    int nRet;
    char tmp[2];
    memset(tmp, 0, sizeof( tmp ) );
    while(1)
    {
        nRet = UDT::select( FD_SETSIZE, &fds, NULL, NULL, &tmOut );
        if(nRet== 0)
            break;
        UDT::recv(session.sockVideo, tmp, 1, 0);
    }
    return 0;
}

int ZLink_AV_Clean_Local_Reliable_RecvBuf(int nAVCid)
{
    return ZLink_AV_Clean_Local_VideoBuf(nAVCid);
}

int ZLink_AV_Reliable_Recv_FrameData(int nAVCid, char *pFrameData, int nFrameDataMaxSize, char *pFrameInfo, int nFrameInfoMaxSize, int nTimeoutMs)
{
    return ZLink_AV_Recv_FrameData(nAVCid, pFrameData, nFrameDataMaxSize, pFrameInfo, nFrameInfoMaxSize, nTimeoutMs);
}

int ZLink_AV_Reliable_Send_FrameData(int nAVCid, char *pFrameData, int nFrameDataSize, char *pFrameInfo, int nFrameInfoSize, int nTimeoutMs)
{
    return ZLink_AV_Send_FrameData(nAVCid, pFrameData, nFrameDataSize, pFrameInfo, nFrameInfoSize, nTimeoutMs);
}

//! ############################################################################
#endif
