#include <iostream>
#include <stdio.h>
#include <ZLink.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include "demo.h"

static void* run_thread1(void *args)
{
    pthread_t tid = pthread_self();
    pthread_detach(tid);

    int avId = *(int *)args;
    delete (int *)args;
    int len = 102400;
    unsigned int laste_seq = 0;

    struct timeval t1 = {0,0}, t2 = {0,0};
    uint8_t *framData = (uint8_t *)malloc(len);;
	int total_size = 0;

	gettimeofday(&t1, NULL);

    while(1)
    {
        ZLink_FrameInfo  framInfo;
        int ret = ZLink_AV_Recv_FrameData(avId, (char *)framData, len, (char *)&framInfo, sizeof(framInfo), 0);
        if(ret < 0)
        {
            printf("error: %s, %d\n", __func__, __LINE__);
            return NULL;
        }
        unsigned int tmp_seq = *(unsigned int *)framData;
        printf("ZLink_AV_Recv_FrameData[%lu][%04d],seq=%u\n", tid, ret, tmp_seq);

        if(tmp_seq != framInfo.seq)
        {
            printf("Video ################### tmp_seq(%u) != framInfo.seq(%u) ####################\n", tmp_seq, framInfo.seq);
		}

        if(tmp_seq < laste_seq)
        {
            printf("Video ################### laste_seq(%u) > framInfo.seq(%u) ####################\n", laste_seq, tmp_seq);
		}

        laste_seq = tmp_seq;
		total_size += ret;

        gettimeofday(&t2, NULL);
        if(t2.tv_sec - t1.tv_sec > 3)
        {
			t1 = t2;
			printf("BitRate: %dB/s %dKB/s\n", total_size/3, total_size/1024/3);
			total_size = 0;
		}

		// printf("V seq:%d size:%d\n", framInfo.seq, ret);
    }

    return NULL;
}

static void* run_thread2(void *args)
{
    pthread_t tid = pthread_self();
    pthread_detach(tid);

    int audioId = *(int *)args;
    delete (int *)args;
    unsigned int laste_seq = 0;

    while(1)
    {
        ZLink_FrameInfo  framInfo;
        uint8_t framData[1024];
        int ret = ZLink_AV_Recv_AudioData(audioId, (char *)framData, sizeof(framData), (char *)&framInfo, sizeof(framInfo), 0);
        if(ret < 0)
        {
            printf("error: %s, %d\n", __func__, __LINE__);
            return NULL;
        }

        unsigned int tmp_seq = *(unsigned int *)framData;
        printf("ZLink_AV_Recv_AudioData[%lu][%04d],seq=%u\n", tid, ret, tmp_seq);

        if(tmp_seq != framInfo.seq)
        {
            printf("Audio ################### tmp_seq(%u) != framInfo.seq(%u) ####################\n", tmp_seq, framInfo.seq);
		}

        if(tmp_seq < laste_seq)
        {
            printf("Audio ################### laste_seq(%u) > framInfo.seq(%u) ####################\n", laste_seq, tmp_seq);
		}
        laste_seq = tmp_seq;

		/*printf("A seq:%d size:%d\n", framInfo.seq, ret);*/
	}
    return NULL;
}

int test_server_main()
{
    int ret = ZLink_Init(ZLINK_HOST, 9000, (char *)"enp6s0");
    if(ret < 0)
        return -1;
    ret = ZLink_Host_Init("root", "123456");
    if(ret < 0)
        return -1;

    while(1)
    {
        int ssid = ZLink_Host_Listen(5000);
        if(ssid <= 0)
        {
            sleep(1);
            continue;
        }

        int avId = ZLink_AV_Host_Start(ssid, 0, 0, ZLINK_CHANNEL_IOCTRL);
        printf("avId: %d\n", avId);

        sleep(1);

        pthread_t tid1, tid2;
        pthread_create(&tid1, NULL, run_thread1, new int(avId));
        pthread_create(&tid2, NULL, run_thread2, new int(avId));

        ZLink_SessionInfo pInfo;
        memset(&pInfo, 0, sizeof(ZLink_SessionInfo));

        ret =  ZLink_Session_GetInfo(ssid, &pInfo);
        printf("%s, %s\n", pInfo.name, pInfo.IP);

    }
    return 0;
}
