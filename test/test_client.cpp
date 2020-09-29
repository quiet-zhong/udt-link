#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <ZLink.h>

#include "demo.h"

static void* run_thread1(void *args)
{
    int avId = *(int *)args;
    unsigned int video_seq = 0;
	uint8_t framData[102400];
    struct timeval t1 = {0,0}, t2 = {0,0};
    int send_len = 0;
    unsigned int total_size = 0;
    gettimeofday(&t1, NULL);

    while(1)
    {
        ZLink_FrameInfo  framInfo;
		framInfo.seq = video_seq;
        memset(framData, rand()&0xff, sizeof(framData));
        memcpy(framData, &video_seq, sizeof(video_seq));

        if (video_seq % 60 == 0)
        {
            send_len = 50000;   //I
        }
        else
        {
            send_len =  5000;   //P
        }
        int ret = ZLink_AV_Send_FrameData(avId, (char *)framData, send_len, (char *)&framInfo, sizeof(framInfo), 0);
        printf("ZLink_AV_Send_FrameData[%04d], seq=%u\n", ret, video_seq);
        if(ret < 0)
        {
            printf("error: func=%s, line=%d\n", __func__, __LINE__);
            return NULL;
        }
        else
        {
            total_size +=  send_len;
        }

        usleep(2000);
		video_seq++;

        gettimeofday(&t2, NULL);
        if(t2.tv_sec - t1.tv_sec > 3) 
        {
			t1 = t2;
			printf("BitRate: %dB/s %dKB/s\n", total_size/3, total_size/1024/3);
			total_size = 0;
		}
    }

    return NULL;
}

static void* run_thread2(void *args)
{
    int audioId = *(int *)args;
    unsigned int audio_seq = 0;
	uint8_t framData[1024];

    while(1)
    {
        ZLink_FrameInfo  framInfo;
		framInfo.seq = audio_seq;
		memset(framData, rand()&0xff, sizeof(framData));
        memcpy(framData, &audio_seq, sizeof(audio_seq));

		int ret = ZLink_AV_Send_AudioData(audioId, (char *)framData, 800, (char *)&framInfo, sizeof(framInfo), 0);
        printf("ZLink_AV_Send_AudioData[%04d], seq=%u\n", ret, audio_seq);
        if(ret < 0)
        {
            printf("error: func=%s, line=%d\n", __func__, __LINE__);
            return NULL;
        }
        usleep(rand() % 100000);
		audio_seq++;
    }
    return NULL;
}

static void event_cb(int nSid, int nCode, void *data)
{
    /* 断开连接 */
    if(nCode != CHAN_DISCONN)
    {
        return;
    }
    ZLink_Client_DeInit();
    sleep(20);

    int ret = ZLink_Init(ZLINK_CLIENT, 0, (char *)"wlan0");
    if(ret < 0)
        return;

    ret = ZLink_Client_Init(NULL, NULL, (char *)"192.168.200.1", 0);
    if(ret < 0)
        return;

    int ssid =  ZLink_Client_Connect(ret, (char *)"client1");
    if(ssid < 0)
        return;
    int avId = ZLink_AV_Client_Start(ssid, 0, NULL, ZLINK_CHANNEL_IOCTRL, 0);
    printf("avId: %d\n", avId);

    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, run_thread1, (void *)&avId);
    pthread_create(&tid2, NULL, run_thread2, (void *)&avId);

    return;
}

int test_client_main()
{
    int ret = ZLink_Init(ZLINK_CLIENT, 0, (char *)"wlan0");
    if(ret < 0) return -1;

    ZLink_LanSearchInfo result[32];
    ret = ZLink_Client_Lan_Search(result, 32, 200);
    if(ret <= 0)
    {
//        cout << "ZLink_Client_Lan_Search error " << ret << endl;
        return -1;
    }
    ret = ZLink_Client_Init((char *)"root", (char *)"123456", result[0].ip, 9000);
    if(ret < 0) return -1;

    int ssid =  ZLink_Client_Connect(ret, (char *)"client1");
    if(ssid < 0) return -1;
    int avId = ZLink_AV_Client_Start(ssid, 0, NULL, ZLINK_CHANNEL_IOCTRL, 0);
    printf("avId: %d\n", avId);

    sleep(1);

    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, run_thread1, (void *)&avId);
    pthread_create(&tid2, NULL, run_thread2, (void *)&avId);

    ZLink_Set_Event_Callback(event_cb);

    while(1)
    {
        sleep(10);
    }

    return 0;
}




