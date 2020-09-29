#ifndef __Z_LINK_H__
#define __Z_LINK_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/prctl.h>

/* Thread name */
#define SET_THREAD_NAME(name) (prctl(PR_SET_NAME, (unsigned long)name))

#define ZLINK_MAX_IOCTRL_BUF_SIZE 1024

/**
 * @brief: 事件
 */
typedef enum _event_type
{
    CHAN_DISCONN = 0x200,               /*<事件，channel被断开>*/
} ZLINK_EVENT;

/**
 * @brief: 错误码
 */
typedef enum _error_code {
    ZLINK_ERR_NO_ERROR          = 0,        /*<无错误>*/
    ZLINK_ERR_FAIL_NORMAL       = -101,     /*<普通错误>*/
    ZLINK_ERR_NOINIT            = -102,     /*<Zlink未初始化>*/
    ZLINK_ERR_NOHOST            = -103,     /*<zlink不是主机>*/
    ZLINK_ERR_NOCLIENT          = -104,     /*<zlink不是客户端>*/
    ZLINK_ERR_ROLE_NOINIT       = -105,     /*<zlink主机或客户端未初始化>*/
    ZLINK_ERR_NOSESSION         = -106,     /*<不存在SESSION>*/

    ZLINK_ERR_RECV_TIMEOUT      = -199,     /*<recv 超时>*/
    ZLINK_ERR_FAIL_INVALID_API  = -200,     /*<api不可用>*/
    ZLINK_ERR_INBUF_SHORT       = -201,     /*<输入buf小于实际接收buf>*/


    ZLINK_ERR_FAIL_ENOSERVER    = -1001,    /*<connection time out>*/
    ZLINK_ERR_FAIL_ECONNREJ     = -1002,    /*<connection rejected>*/
    ZLINK_ERR_FAIL_ESOCKFAIL    = -1003,    /*<unable to create/configure UDP socket>*/
    ZLINK_ERR_FAIL_ESECFAIL     = -1004,    /*<abort for security reasons>*/

    ZLINK_ERR_FAIL_ECONNLOST    = -2001,    /*<Connection was broken>*/
    ZLINK_ERR_FAIL_ENOCONN      = -2002,    /*<Connection does not exist>*/

    ZLINK_ERR_FAIL_ETHREAD      = -3001,    /*<unable to create new threads>*/
    ZLINK_ERR_FAIL_ENOBUF       = -3002,    /*<unable to allocate buffers>*/

    ZLINK_ERR_FAIL_EBOUNDSOCK   = -5001,    /*<Cannot do this operation on a BOUND socket>*/
    ZLINK_ERR_FAIL_ECONNSOCK    = -5002,    /*<Cannot do this operation on a CONNECTED socket>*/
    ZLINK_ERR_FAIL_EINVPARAM    = -5003,    /*<Bad parameters>*/
    ZLINK_ERR_FAIL_EINVSOCK     = -5004,    /*<Invalid socket ID>*/
    ZLINK_ERR_FAIL_EUNBOUNDSOCK = -5005,    /*<Cannot do this operation on an UNBOUND socket>*/
    ZLINK_ERR_FAIL_ENOLISTEN    = -5006,    /*<Socket is not in listening state>*/
    ZLINK_ERR_FAIL_ERDVNOSERV   = -5007,    /*<Listen/accept is not supported in rendezous connection setup>*/
    ZLINK_ERR_FAIL_ERDVUNBOUND  = -5008,    /*<Cannot call connect on UNBOUND socket in rendezvous connection setup>*/
    ZLINK_ERR_FAIL_ESTREAMILL   = -5009,    /*<operation is not supported in SOCK_STREAM mode>*/
    ZLINK_ERR_FAIL_EDGRAMILL    = -5010,    /*<This operation is not supported in SOCK_DGRAM mode>*/
    ZLINK_ERR_FAIL_EDUPLISTEN   = -5011,    /*<Another socket is already listening on the same port>*/
    ZLINK_ERR_FAIL_ELARGEMSG    = -5012,    /*<Message is too large to send (it must be less than the UDT send buffer size)>*/
    ZLINK_ERR_FAIL_EINVPOLLID   = -5013,    /*<Invalid epoll ID>*/

    ZLINK_ERR_FAIL_EASYNCSND    = -6001,    /*<no buffer available for sending>*/
    ZLINK_ERR_FAIL_EASYNCRCV    = -6002,    /*<no data available for reading>*/

    ZLINK_HOST_CLI_CANNOT_FIND  = -7001,    /*<there is no client in host database>*/
    ZLINK_HOST_SSID_ERROR       = -7002,    /*<ssid error>*/
    ZLINK_HOST_CHAN_ERROR       = -7003,    /*<chan greater than 3 or chan less than 0>*/
    ZLINK_HOST_CLI_EXIT         = -7004,    /*<client exit>*/

    ZLINK_CLIENT_NULL_ERROR     = -7100,    /*<no client init error>*/
    ZLINK_CLIENT_EXIT           = -7101,    /*<client exit>*/
    ZLINK_CLIENT_CHAN_ERROR     = -7102,    /*<chan greater than 3 or chan less than 0>*/
    ZLINK_CLIENT_CONNECT_ERROR  = -7103,    /*<cannot connect host>*/

} ZLINK_ERROR_CODE;

/**
 * @brief: zlink角色，station--host, camera--client
 */
typedef enum {
	ZLINK_HOST = 0,                     /*<zlink host<--->station>*/
	ZLINK_CLIENT,                       /*<zlink client<--->camera>*/
    ZLINK_UNKNOWN
} ZLINK_ROLE;

typedef enum {
    ZLINK_CHANNEL_IOCTRL,
    ZLINK_CHANNEL_VIDEO,
    ZLINK_CHANNEL_AUDIO,
    ZLINK_CHANNEL_RELIABLE,
} ZLINK_CHANNEL_TYPE;


/**
 * @brief: 帧标记
 */
typedef enum {
	ZLINK_VIDEO_PB_FRAME = 0,           /*<PB帧>*/
	ZLINK_VIDEO_I_FRAME = 1,            /*<I帧>*/
} ZLINK_VIDEO_FLAGS;

/**
 * @brief: IOCTL 控制指令
 */
typedef enum {
	ZLINK_VIDEO_START = 0,              /*<video 启动>*/
	ZLINK_VIDEO_STOP,                   /*<video 停止>*/
	ZLINK_VIDEO_REQUEST_IDR,            /*<video 请求I帧>*/
	ZLINK_AUDIO_START,                  /*<audio 启动>*/
	ZLINK_AUDIO_STOP,                   /*<audio 停止>*/
	ZLINK_AUDIO_SPK_START,              /*<audio 对讲启动>*/
	ZLINK_AUDIO_SPK_STOP,               /*<audio 对讲停止>*/
	ZLINK_MP4_RECORD_START,             /*<audio> mp4录像启动*/
	ZLINK_MP4_RECORD_STOP,              /*<audio> mp4录像停止*/
	ZLINK_CLIENT_CLOSE,                 /*client 关闭*/
} ZLINK_AV_IOCTRL_CMD;

typedef struct _ZLink_LanSearchInfo {
	int num; // TODO:
    char ip[16];
} ZLink_LanSearchInfo;

/**
 * @brief: session 信息
 */
typedef struct _ZLink_SessionInfo {
	char name[32]; // TODO:             /*<session名称>*/
	char IP[16];                        /*<session IP>*/
	char reserve[128];                  /*<保留>*/
} ZLink_SessionInfo;

/**
 * @brief: 录像信息
 */
typedef struct _ZLink_RecordInfo {
	int width;                      /*<分辨率宽高>*/
	int height;                     /*<分辨率宽高>*/
	int fps;                        /*<帧率>*/
	char name[128];                 /*<录像名称>*/
} ZLink_RecordInfo;

/**
 * @brief: 帧信息
 */
typedef struct _ZLink_FrameInfo {
    unsigned short codec_id;        /*<编码格式>*/
	unsigned char flags;            /*<IPB帧标志>*/
	unsigned char nCamIndex;        /*<camera ID>*/

	unsigned char nOnlineNum;       /*<app连接个数>*/
	unsigned char aReserve[3];      /*保留*/

	unsigned int seq;               /*<帧序号>*/
	unsigned int mediaType;

	long long int timestamp;        /*<时间戳>*/
	unsigned char hlReserve[20];
} ZLink_FrameInfo;

typedef void (*ZL_PF_ST_EVT)(int nSid, int nCode, void *data);

//! ######################### zlink lib api ##################################
/**
 * @brief ZLink_Init: zlink初始化函数
 *
 * @param[in] nRole: 角色，host or client
 * @param[in] nPort: 本地端口
 * @param[in] NetCardName:网卡名称
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_Init(ZLINK_ROLE nRole, int nPort, char *NetCardName);

/**
 * @brief ZLink_DeInit:zlink去初始化
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_DeInit();

/**
 * @brief ZLink_GetVersion: zlink获取版本信息
 *
 * @retval: NULL error
 * @retval: !=NULL success
 *
 * @attention
 */
const char *ZLink_GetVersion();


//! ######################### host api ##################################
/**
 * @brief ZLink_Host_Init: host初始化操作
 *
 * @param[in] pUid: 保留
 * @param[in] pPwd: 保留
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_Host_Init(const char *pUid, const char *pPwd);

/**
 * @brief ZLink_Host_DeInit: host去初始化
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_Host_DeInit();

/**
 * @brief ZLink_Host_Listen:zlink host监听clinet连接
 *
 * @param[in] nTimeoutMs: timeout暂不支持，该函数阻塞，直到连接返回
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: >=0 success
 *
 * @attention
 */
int ZLink_Host_Listen(int nTimeoutMs);

/**
 * @brief ZLink_Host_Listen_Exit
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_Host_Listen_Exit();

/**
 * @brief ZLink_Host_Set_Client_Concurrent_Size: host设置camera 最大连接数量
 *
 * @param[in] size: 设置数量
 *
 * @retval: = 0; success;
 * @retval: < 0; error;
 *
 * @attention
 */
int ZLink_Host_Set_Client_Concurrent_Size(int size);

/**
 * @brief ZLink_AV_Host_Start: host连接成功后，获取channelId
 *
 * @param[in] nSid: 对应session ID
 * @param[in] nTimeoutMs: 保留
 * @param[in] nHostType: 保留
 * @param[in] nChn: 申请ID标记号
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: >=0 success
 *
 * @attention
 */
int ZLink_AV_Host_Start(int nSid, int nTimeoutMs, int nHostType, ZLINK_CHANNEL_TYPE nChn);

/**
 * @brief ZLink_AV_Host_Exit
 *
 * @param[] nSid
 * @param[] nChn
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_AV_Host_Exit(int nSid, ZLINK_CHANNEL_TYPE nChn);

/**
 * @brief ZLink_AV_Host_Stop
 *
 * @param[] nAVCid
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_AV_Host_Stop(int nAVCid);


//! ####################### client api ####################################
/**
 * @brief ZLink_Client_Lan_Search:
 *
 * @param[in] pLanSearchInfo
 * @param[in] nArrayLen
 * @param[in] nTimeoutMs
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_Client_Lan_Search(ZLink_LanSearchInfo *pLanSearchInfo, int nArrayLen, int nTimeoutMs);

/**
 * @brief ZLink_Client_Init
 *
 * @param[in] pUid: 保留
 * @param[in] pPwd: 保留
 * @param[in] IP: host ip
 * @param[in] port: host port
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_Client_Init(const char *pUid, const char *pPwd, char *IP, int port);

/**
 * @brief ZLink_Client_DeInit: client去初始化
 *
 * @param[in] nCid: client id
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_Client_DeInit();

/**
 * @brief ZLink_Client_Connect: client连接
 *
 * @param[in] nCid:  保留
 * @param[in] cName:    客户端名称
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: >=0 成功
 *
 * @attention
 */
int ZLink_Client_Connect(int nCid, char *cName);

/**
 * @brief ZLink_Client_Connect_Exit
 *
 * @param[] nCid
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_Client_Connect_Exit(int nCid);

/**
 * @brief ZLink_AV_Client_Start:client连接成功后，获取channelId
 *
 * @param[in] nSid: 对应session ID
 * @param[in] nTimeoutMs: 保留
 * @param[in] pnHostType: 保留
 * @param[in] nChn: 申请ID标记号
 * @param[in] nResend: 保留
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: >=0 success
 *
 * @attention
 */
int ZLink_AV_Client_Start(int nSid, int nTimeoutMs, int *pnHostType, ZLINK_CHANNEL_TYPE nChn, int nResend);

/**
 * @brief ZLink_AV_Client_Exit
 *
 * @param[] nSid
 * @param[] nChn
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_AV_Client_Exit(int nSid, ZLINK_CHANNEL_TYPE nChn);

/**
 * @brief ZLink_AV_Client_Stop
 *
 * @param[] nAVCid
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_AV_Client_Stop(int nAVCid);


//! ######################### session api ##################################
/**
 * @brief ZLink_Set_Event_Callback: 设置事件回调
 *
 * @param[in] cb: 回调
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_Set_Event_Callback(ZL_PF_ST_EVT cb);

/**
 * @brief ZLink_Session_Close: host 端session close
 *
 * @param[in] nSid: session ID
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_Session_Close(int nSid);

/**
 * @brief ZLink_Session_Read
 *
 * @param[] nSid
 * @param[] pBuf
 * @param[] nMaxBufSize
 * @param[] nTimeoutMs
 * @param[] nChn
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_Session_Read(int nSid, char *pBuf, int nMaxBufSize, int nTimeoutMs, ZLINK_CHANNEL_TYPE nChn);

/**
 * @brief ZLink_Session_Write
 *
 * @param[] nSid
 * @param[] pBuf
 * @param[] nBufSize
 * @param[] nTimeoutMs
 * @param[] nChn
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_Session_Write(int nSid, char *pBuf, int nBufSize, int nTimeoutMs, ZLINK_CHANNEL_TYPE nChn);

/**
 * @brief ZLink_Session_GetInfo: 根据ssid获取 session 信息
 *
 * @param[in] nSid: ssid
 * @param[out] pInfo: info信息
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention: 若是client端，必须在ZLink_Client_Init api之前，否则失败
 */
int ZLink_Session_GetInfo(int nSid, ZLink_SessionInfo *pInfo);

/**
 * @brief ZLink_Session_GetState
 *
 * @param[] nSid
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_Session_GetState(int nSid);

/**
 * @brief ZLink_AV_Check_Normal_SendBuf: 根据ssid检查发送帧缓存情况, normal 通道
 *
 * @param[in] nAVCid: 对应的session id, 若是client ssid为0
 *
 * @retval: > 0; success, 返回发送帧缓存的使用百分比
 *
 * @attention
 */
int ZLink_AV_Check_Normal_SendBuf(int nSid);

/**
 * @brief ZLink_Session_Get_Free_Channel
 *
 * @param[] nSid
 *
 * @retval
 * @retval
 *
 * @attention: 不可用
 */
int ZLink_Session_Get_Free_Channel(int nSid);

/**
 * @brief ZLink_AV_Check_Reliable_SendBuf: 根据ssid检查发送帧缓存情况
 *
 * @param[in] nAVCid: 对应的session id, 若是client ssid为0
 *
 * @retval: > 0; success, 返回发送帧缓存的使用百分比
 *
 * @attention: client ssid 为0;
 */
int ZLink_AV_Check_Reliable_SendBuf(int nSid);

/**
 * @brief ZLink_Set_RxCache_Size_Reliable: 设置reliable接收缓冲区大小
 *
 * @param[in] nSid: ssid
 * @param[in] nSize: 缓冲区大小
 *
 * @retval: < 0; error;
 * @retval: = 0; success
 *
 * @attention: 若是client端，必须在ZLink_Client_Connect api之前，否则无效, 若host端，当ssid < 0时，设置全局cache
 */
int ZLink_Set_RxCache_Size_Reliable(int nSid, int nSize);

/**
 * @brief ZLink_Set_TxCache_Size_Reliable: 设置reliable发送缓冲区大小
 *
 * @param[in] nSid: ssid
 * @param[in] nSize: 缓冲区大小
 *
 * @retval: < 0; error;
 * @retval: = 0; success
 *
 * @attention: 若是client端，必须在ZLink_Client_Connect api之前，否则无效, 若host端，当ssid < 0时，设置全局cache
 */
int ZLink_Set_TxCache_Size_Reliable(int nSid, int nSize);

/**
 * @brief ZLink_Set_RxCache_Size_Normal: 设置normal接收缓冲区大小
 *
 * @param[in] nSid: ssid
 * @param[in] nSize: 缓冲区大小
 *
 * @retval: < 0; error;
 * @retval: = 0; success
 *
 * @attention: 若是client端，必须在ZLink_Client_Connect api之前，否则无效, 若host端，当ssid < 0时，设置全局cache
 */
int ZLink_Set_RxCache_Size_Normal(int nSid, int nSize);

/**
 * @brief ZLink_Set_TxCache_Size_Normal: 设置normal发送缓冲区大小
 *
 * @param[in] nSid: ssid
 * @param[in] nSize: 缓冲区大小
 *
 * @retval: < 0; error;
 * @retval: = 0; success
 *
 * @attention: 若是client端，必须在ZLink_Client_Connect api之前，否则无效, 若host端，当ssid < 0时，设置全局cache
 */
int ZLink_Set_TxCache_Size_Normal(int nSid, int nSize);


//! ####################### channel api ####################################
/**
 * @brief ZLink_AV_Recv_IOCtrl: 接收控制通道指令，该函数一直阻塞，直到接收到控制数据或对应的client析构为止
 *
 * @param[in] nAVCid: 对应channelId
 * @param[in] nIOType: IO 类型
 * @param[out] pIOData: 输出数据
 * @param[in] nIODataMaxSize: 接收IO buf长度
 * @param[in] nTimeoutMs: 保留
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: > 0; success, 实际长度
 * @retval: ZLINK_ERR_INBUF_SHORT 接收buf长度比实际长度小，error
 *
 * @attention
 */
int ZLink_AV_Recv_IOCtrl(int nAVCid, unsigned int *nIOType, char *pIOData, int nIODataMaxSize, int nTimeoutMs);

/**
 * @brief ZLink_AV_Send_IOCtrl: 发送控制通道指令
 *
 * @param[in] nAVCid: 对应channelId
 * @param[in] nIOType: IO类型
 * @param[in] pIOData: IO数据
 * @param[in] nIODataSize: IO数据长度
 * @param[in] nTimeoutMs:保留
 *
 * @retval: <0; error
 * @retval: >0; success 实际长度
 *
 * @attention
 */
int ZLink_AV_Send_IOCtrl(int nAVCid, unsigned int nIOType, char *pIOData, int nIODataSize, int nTimeoutMs);

/**
 * @brief ZLink_AV_Recv_AudioData: 根据channelId接收对应audio帧数据。
 *
 * @param[in] nAVCid: 对应的channnelId
 * @param[out] pAudioData: 接收audio数据
 * @param[in] nAudioDataMaxSize: audio buf最大长度
 * @param[out] pFrameInfo: audio 帧信息
 * @param[in] nFrameInfoMaxSize: 帧信息最大长度
 * @param[in] nTimeoutMs: 保留
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: > 0; success, 实际长度
 * @retval: ZLINK_ERR_INBUF_SHORT 接收buf长度比实际长度小，error
 *
 * @attention
 */
int ZLink_AV_Recv_AudioData(int nAVCid, char *pAudioData, int nAudioDataMaxSize, char *pFrameInfo, int nFrameInfoMaxSize, int nTimeoutMs);

/**
 * @brief ZLink_AV_Send_AudioData:  根据channelId发送audio帧数据
 *
 * @param[in] nAVCid: 对应的channelId
 * @param[in] pAudioData: audio数据
 * @param[in] nAudioDataSize: audio数据长度
 * @param[in] pFrameInfo: audio 帧信息数据
 * @param[in] nFrameInfoSize: frameInfo 长度
 * @param[in] nTimeoutMs: 保留
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: > 0; success, 实际长度
 *
 * @attention
 */
int ZLink_AV_Send_AudioData(int nAVCid, char *pAudioData, int nAudioDataSize, char *pFrameInfo, int nFrameInfoSize, int nTimeoutMs);

/**
 * @brief ZLink_AV_Recv_FrameData: 根据channelId接收帧数据
 *
 * @param[in] nAVCid: 对应的channelId
 * @param[out] pFrameData: 接收frameData数据
 * @param[in] nFrameDataMaxSize: frameData最大长度
 * @param[out] pFrameInfo: 帧信息数据
 * @param[in] nFrameInfoMaxSize: 帧信息最大接收长度
 * @param[in] nTimeoutMs: 保留
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: > 0; success, 实际长度
 * @retval: ZLINK_ERR_INBUF_SHORT 接收buf长度比实际长度小，error
 *
 * @attention
 */
int ZLink_AV_Recv_FrameData(int nAVCid, char *pFrameData, int nFrameDataMaxSize, char *pFrameInfo, int nFrameInfoMaxSize, int nTimeoutMs);

/**
 * @brief ZLink_AV_Send_FrameData: 根据channelId发送帧数据
 *
 * @param[in] nAVCid: 对应的channelId
 * @param[in] pFrameData: 发送的frameData数据
 * @param[in] nFrameDataSize: 发送数据长度
 * @param[in] pFrameInfo: 帧信息数据
 * @param[in] nFrameInfoSize: 帧信息数据长度
 * @param[in] nTimeoutMs: 保留
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: > 0; success, 实际长度
 *
 * @attention
 */
int ZLink_AV_Send_FrameData(int nAVCid, char *pFrameData, int nFrameDataSize, char *pFrameInfo, int nFrameInfoSize, int nTimeoutMs);

/**
 * @brief ZLink_AV_Check_RecvAudioBuf: 根据channelId检测audio接收缓冲区数据个数
 *
 * @param[in] nAVCid: 对应的channelId
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error;
 * @retval: >= 0; 缓冲区中data的数量
 *
 * @attention
 */
int ZLink_AV_Check_RecvAudioBuf(int nAVCid);

/**
 * @brief ZLink_AV_Check_RecvVideoBuf: 根据channelId检测video缓冲区接收数据个数
 *
 * @param[in] nAVCid: 对应的channelId
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error;
 * @retval: >= 0; 缓冲区中data的数量
 *
 * @attention
 */
int ZLink_AV_Check_RecvVideoBuf(int nAVCid);

/**
 * @brief ZLink_AV_Clean_Local_AudioBuf: 根据channelId清除audio缓冲区数据
 *
 * @param[in] nAVCid: 对应的channelId
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_AV_Clean_Local_AudioBuf(int nAVCid);

/**
 * @brief ZLink_AV_Clean_Local_VideoBuf: 根据channelId清除video缓冲区数据
 *
 * @param[in] nAVCid
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_AV_Clean_Local_VideoBuf(int nAVCid);

/**
 * @brief ZLink_AV_Clean_Local_Reliable_RecvBuf: 清除可靠通道接收缓存
 *
 * @param[in] nAVCid: channel id
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: ZLINK_ERR_NO_ERROR success
 *
 * @attention
 */
int ZLink_AV_Clean_Local_Reliable_RecvBuf(int nAVCid);

/**
 * @brief ZLink_AV_Reliable_Recv_FrameData: 可靠接收通道数据
 *
 * @param[in] nAVCid: 对应的channelId
 * @param[out] pFrameData: 接收frameData数据
 * @param[in] nFrameDataMaxSize: frameData最大长度
 * @param[out] pFrameInfo: 帧信息数据
 * @param[in] nFrameInfoMaxSize: 帧信息最大接收长度
 * @param[in] nTimeoutMs: 保留
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: > 0; success, 实际长度
 * @retval: ZLINK_ERR_INBUF_SHORT 接收buf长度比实际长度小，error

 * @attention
 */
int ZLink_AV_Reliable_Recv_FrameData(int nAVCid, char *pFrameData, int nFrameDataMaxSize, char *pFrameInfo, int nFrameInfoMaxSize, int nTimeoutMs);

/**
 * @brief ZLink_AV_Reliable_Send_FrameData: 可靠发送通道数据
 *
 * @param[in] nAVCid: 对应的channelId
 * @param[in] pFrameData: 发送的frameData数据
 * @param[in] nFrameDataSize: 发送数据长度
 * @param[in] pFrameInfo: 帧信息数据
 * @param[in] nFrameInfoSize: 帧信息数据长度
 * @param[in] nTimeoutMs: 保留
 *
 * @retval: ZLINK_ERR_FAIL_NORMAL error
 * @retval: > 0; success, 实际长度
 *
 * @attention
 */
int ZLink_AV_Reliable_Send_FrameData(int nAVCid, char *pFrameData, int nFrameDataSize, char *pFrameInfo, int nFrameInfoSize, int nTimeoutMs);

#ifdef __cplusplus
}
#endif


#endif //__Z_LINK_H__
