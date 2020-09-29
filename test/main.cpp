#include "demo.h"

#include "ZLink.h"

#include "utils.h"

#include <unistd.h>

#include <iostream>

using namespace std;

#include <stdio.h>
#include <string.h>

int demo_test()
{
    demo_test(1);
    demo_test(2);
    demo_test(3);
    demo_test(4);
    return 0;
}

int demo_test2()
{
    int ret = -1;
#ifdef UDT_SERVER
    ret = demo_server_with_epoll();
//    ret = demo_server();
#else
    ret = demo_client();
#endif
    return ret;
}

//测试客户端和服务端的建立会话
int zlink_connect_test()
{
    int ret = -1;
#ifdef UDT_SERVER
    ret = ZLink_Init(ZLINK_HOST, 9000, (char *)"eth0");
    if(ret != 0)
    {
        cout << "ZLink_Host_Init error " << ret << endl;
        return -1;
    }
    ret = ZLink_Host_Init((char *)"root", (char *)"123456");
    if(ret != 0)
    {
        cout << "ZLink_Host_Init error " << ret << endl;
        return -1;
    }

    while(1)
//    for(int i=0; i < 5; i++)
    {
        int nSid = -1;
        nSid = ZLink_Host_Listen(5000);
        if(nSid < 0)
        {
            cout << "ZLink_Host_Listen error " << nSid << endl;
            continue;
        }
        sleep(3);
        ZLink_Session_Close(nSid);
    }
    ZLink_Host_DeInit();
#else
    ret = ZLink_Init(ZLINK_CLIENT, 0, (char *)"eth0");
    if(ret != 0)
    {
        cout << "ZLink_Host_Init error " << ret << endl;
        return -1;
    }

    ZLink_LanSearchInfo result[32];
    ret = ZLink_Client_Lan_Search(result, 32, 2000);
    if(ret <= 0)
    {
        cout << "ZLink_Client_Lan_Search error " << ret << endl;
        return -1;
    }

    ret = ZLink_Client_Init("root", "123456", result[0].ip, 9000);
    if(ret != 0)
    {
        cout << "ZLink_Client_Init error " << ret << endl;
        return -1;
    }
    char strSN[16] = "";
    sprintf(strSN, "TJHL%08X", (unsigned int)time(NULL));
    ret = ZLink_Client_Connect(0, strSN);
    if(ret != 0)
    {
        cout << "ZLink_Client_Connect error " << ret << endl;
        return -1;
    }

    sleep(3);

    ZLink_Client_DeInit();
#endif

    ZLink_DeInit();
    return ret;
}

int app_test()
{
    int ret = -1;
#ifdef UDT_SERVER
    ret = test_server_main();
#else
    ret = test_client_main();
#endif
    return ret;
}

#include <vector>

int main()
{
    cout << "########################### begin ######################################" << endl;

    int ret = -1;
    cout << "Zlink Version:" << ZLink_GetVersion() << endl;;


//    zlink_connect_test();

    ret = app_test();



//    char temp[32] = "Golden Global View";

//    char *p;
//    char c = ' ';
//    p = strchr(temp, c);
//    if(p)
//        printf("%s", p);
//    else
//        printf("Not Found!");


//    char a[100] = "nie shi zhong shizhong";
//    char b[100] = "shi";
//    p = strstr(a, b);
//    if(p)
//    {
//        int index = p - a;
//        printf("%d", index);
//    }


//vector<int> vec;
//vec.resize(5);
//cout << vec.size() << vec.capacity();

    cout << "###########################  end  ######################################" << endl;
    return ret;
}
