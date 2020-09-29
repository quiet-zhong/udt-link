#include "zlink_inner.h"

int createServerSocket(LinkManager *link)
{
    if ((link->m_nTcpServer = createTCPSocket(link->m_nLocalPort)) < 0)
    {
        cout << "create TCP Socket error" << endl;
        return -1;
    }

    if ((link->m_nUdtServer = createUDTSocket(link->m_nLocalPort)) < 0)
    {
        cout << "create UDT Socket error" << endl;
        tcp_close(link->m_nTcpServer);
        return -2;
    }
    return 0;
}

int closeServerSocket(LinkManager *link)
{
    tcp_close(link->m_nTcpServer);
    udt_close(link->m_nUdtServer);
    return 0;
}

int setServerListen(LinkManager *link)
{
    int sessionChanCount = 3+ZLINK_SESSION_EXTERN_CHANS; //ioctrl + audio + video + extern
    listen(link->m_nTcpServer, link->m_nSessionMaxCount);
    return UDT::listen(link->m_nUdtServer, link->m_nSessionMaxCount * sessionChanCount);
}

int AvFrameDataRecv(int nAVCid, char *pFrameData, int nFrameDataMaxSize, char *pFrameInfo, int nFrameInfoMaxSize, int nTimeoutMs, ZLINK_CHANNEL_TYPE chn)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nAVCid);

    int nSid = nAVCid;

    int nFrameInfoSize = 0;
    int nFrameDataSize = 0;
    if(0 != avHeaderRecv(nSid, chn,nTimeoutMs, &nFrameInfoSize, &nFrameDataSize))
        return -220011;

    if(nFrameInfoSize > nFrameInfoMaxSize || nFrameDataSize > nFrameDataMaxSize)
        return -220022; //参数buf不足

    int ret = ZLink_Session_Read(nSid, pFrameInfo, nFrameInfoSize, nTimeoutMs, chn);
    if(ret != nFrameInfoSize)
        return -220033;

    ret = ZLink_Session_Read(nSid, pFrameData, nFrameDataSize, nTimeoutMs, chn);
    if(ret != nFrameDataSize)
        return -220044;

    return nFrameDataSize;
}

int AvFrameDataSend(int nAVCid, char *pFrameData, int nFrameDataSize, char *pFrameInfo, int nFrameInfoSize, int nTimeoutMs, ZLINK_CHANNEL_TYPE chn)
{
    CHECK_ZLINK_LIB_INIT;
    CHECK_ZLINK_ROLE_INIT;
    CHECK_ZLINK_SESSION(nAVCid);

    int nSid = nAVCid;

    int ret = avHeaderSend(nSid, chn, nTimeoutMs, nFrameInfoSize, nFrameDataSize);
    if(ret != 0)
    {
        return -110011;
    }

    ret = ZLink_Session_Write(nSid, pFrameInfo, nFrameInfoSize, nTimeoutMs, chn);
    if(ret != nFrameInfoSize)
        return -110022;

    ret = ZLink_Session_Write(nSid, pFrameData, nFrameDataSize, nTimeoutMs, chn);
    if(ret != nFrameDataSize)
        return -110033;

    return nFrameDataSize;
}

int avHeaderSend(int nSid, ZLINK_CHANNEL_TYPE chn,int nTimeoutMs,int nFrameInfoSize, int nFrameDataSize)
{
    char pPackHead[32] = "HLJK:00000000:00000000:00000000";//头标志:总长度:帧信息长度:帧数据长度
    int nPackHeadLen = sizeof(pPackHead);

    snprintf(pPackHead, nPackHeadLen, "HLKJ:%08d:%08d:%08d",
             nPackHeadLen + nFrameInfoSize + nFrameDataSize, nFrameInfoSize, nFrameDataSize);

    int ret = ZLink_Session_Write(nSid, pPackHead, nPackHeadLen, nTimeoutMs, chn);
    if(ret != nPackHeadLen)
        return -1;
    return 0;
}

int avHeaderRecv(int nSid, ZLINK_CHANNEL_TYPE chn,int nTimeoutMs,int *nFrameInfoSize, int *nFrameDataSize)
{
    char pPackHead[32] = "HLKJ:00000000:00000000:00000000"; //头标志:总长度:帧信息长度:帧数据长度
    int nPackHeadLen = sizeof(pPackHead);

    //接受协议头
    int ret = ZLink_Session_Read(nSid, pPackHead, nPackHeadLen, nTimeoutMs, chn);
    if(ret != nPackHeadLen)
    {
        return -330011;
    }

    char *p = strstr(pPackHead, "HLKJ");
    if(!p)
    {
        return -330022;
    }

    char strVal[16][64] = {""};
    splitString(pPackHead, ":", strVal);

    *nFrameInfoSize = atoi(strVal[2]);
    *nFrameDataSize = atoi(strVal[3]);
    return 0;
}
