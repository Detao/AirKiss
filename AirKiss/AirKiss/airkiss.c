#include "airkiss.h"
#include<stdlib.h>
#include <ws2tcpip.h>


int GetRandomNum()
{
    srand((unsigned)(time(NULL)));
    return rand()%127;
}
int  SockInit(int nState // Requested operation
              // nState could be:
              // 0 = Shutdown sockets
              // 1 = Turn on sockets
              )
{
    // Windows Specific - Sockets initialization
    unsigned short      wVersionRequested;
    WSADATA             wsaData;
    int              nErr = 0;

    // We want to use Winsock v 1.2 (can be higher)
    wVersionRequested = MAKEWORD( 1, 2 );

    if(nState > 0) // Initialize Winsock
    {
        nErr = WSAStartup( wVersionRequested, &wsaData );
    }
    else
    {
        // Windows sockets cleanup
        WSACleanup();
    }
    return nErr;
}


int sendPacketAndSleep(int sock, int len)
{
    char buffer[1500];
    memset(buffer,'a',1500);
    struct sockaddr_in Inet_addr;
    Inet_addr.sin_family = AF_INET;
    Inet_addr.sin_port = htons(10001);
    Inet_addr.sin_addr.s_addr = inet_addr("255.255.255.255");//

    sendto(sock, buffer, len, 0, (struct sockaddr*)&Inet_addr, sizeof(Inet_addr));
    Sleep(6);


}
//crc8
unsigned char calcrc_1byte(unsigned char abyte)
{
    unsigned char i,crc_1byte;
    crc_1byte=0;
    for(i = 0; i < 8; i++)
    {
        if(((crc_1byte^abyte)&0x01))
        {

            crc_1byte^=0x18;
            crc_1byte>>=1;
            crc_1byte|=0x80;
        }
        else
        {
            crc_1byte>>=1;
        }
        abyte>>=1;
    }
    return crc_1byte;
}


unsigned char calcrc_bytes(unsigned char *p,unsigned int num_of_bytes)
{
    unsigned char crc=0;
    while(num_of_bytes--)
    {
        crc=calcrc_1byte(crc^*p++);
    }
    return crc;
}

void sendLeadingPart(int sock){
    for (int i = 0; i < 50; ++i) {
        for (int j = 1; j <=4; ++j)
        {
            sendPacketAndSleep(sock,j);
        }
    }
}
void sendMagicCode(int sock,char *password,char *ssid) {
    assert(password&&ssid);
    int length = strlen(password) + strlen(ssid)+ 1;
    int magicCode[4];
    magicCode[0] = 0x00 | (length >> 4 & 0xF);
    if (magicCode[0] == 0)
        magicCode[0] = 0x08;
    magicCode[1] = 0x10 | (length & 0xF);
    int crc8 =calcrc_bytes((unsigned char *)ssid,(unsigned int)strlen(ssid));
    magicCode[2] = 0x20 | (crc8 >> 4 & 0xF);
    magicCode[3] = 0x30 | (crc8 & 0xF);
    for (int j = 0; j < 4; ++j){
        sendPacketAndSleep(sock,magicCode[j]);
    }
}
void sendPrefixCode(int sock,char* password)
{
    assert(password);
    int length = strlen(password);
    int prefixCode[4] ;
    prefixCode[0] = 0x40 | (length >> 4 & 0xF);
    prefixCode[1] = 0x50 | (length & 0xF);
    int crc8 = calcrc_1byte((unsigned char)length);
    prefixCode[2] = 0x60 | (crc8 >> 4 & 0xF);
    prefixCode[3] = 0x70 | (crc8 & 0xF);
    for (int j = 0; j < 4; ++j)
    {
        sendPacketAndSleep(sock,prefixCode[j]);
    }
}

void sendSequence(int sock,int index, char* data)
{
    assert(data);
    int length=strlen(data);
    char content[5];
    content[0] = (char)(index & 0xFF);
    strcpy(&(content[1]),data);
    int crc8 = calcrc_bytes((unsigned char *)content,length+1);
    sendPacketAndSleep(sock,0x80 | crc8);
    sendPacketAndSleep(sock,0x80 | index);
    for (int i = 0; i <length; ++i){
        sendPacketAndSleep(sock,data[i] | 0x100);

    }
}

void RecvResult(int Handle)
{
    AirKissValue *HandleSession=(AirKissValue *)Handle;
    char random[4];
    struct timeval Timeval = {0, 50000};
    struct sockaddr_in from;
    fd_set FDRead, FDError;

    int nSocketEvents;
    long StartTime=GetTickCount();

    from.sin_family = AF_INET;
    from.sin_addr.s_addr = htonl(INADDR_ANY);
    from.sin_port = htons(10000);
    int len=sizeof(struct sockaddr_in);

    while(1)
    {
        // Check for timeout
        long EndTime=GetTickCount();
        if(EndTime-StartTime>60000)
        {
            HandleSession->nRetCode = AIRKISS_TIME_OUT;
            break;
        }

        // Reset socket events
        FD_ZERO(&FDRead);
        FD_ZERO(&FDError);
        FD_SET(HandleSession->Sock,&FDRead);
        FD_SET(HandleSession->Sock,&FDError);

        // See if we got any events on the socket
        nSocketEvents = select(HandleSession->Sock + 1, &FDRead,
                               0,
                               &FDError,
                               &Timeval);

        if(nSocketEvents < 0) // Error or no new socket events
        {
            HandleSession->nRetCode = AIRKISS_ERROR;
            break;
        }
        if(nSocketEvents == 0)
        {
            continue;
        }

        if(FD_ISSET(HandleSession->Sock ,&FDRead)) // Are there any read events on the socket ?
        {
            FD_CLR((int)HandleSession->Sock,&FDRead);
            int ret=recvfrom(HandleSession->Sock, random, sizeof(random), 0, (struct sockaddr*)&from,(socklen_t *)&len);
            if(ret<=0)
            {
                HandleSession->nRetCode = AIRKISS_ERROR;
                return;
            }
            else {
                if(random[0]==HandleSession->Random)
                {
                    HandleSession->nRetCode = AIRKISS_SUCCESS;
                    break;
                }
            }
        }
        // We had a socket related error
        if(FD_ISSET(HandleSession->Sock ,&FDError))
        {
            FD_CLR((int)HandleSession->Sock,&FDError);
            // To-Do: Handle this case
            HandleSession->nRetCode = AIRKISS_ERROR;
            break;

        }
    }

}
void SendData(int Handle)
{
    AirKissValue *HandleSession=(AirKissValue *)Handle;
    int data_len=strlen(HandleSession->Data);
    int times=6,i;
    while(times--)
    {
        if(HandleSession->nRetCode==AIRKISS_SUCCESS||HandleSession->nRetCode==AIRKISS_TIME_OUT)
        {
            break;
        }


        sendLeadingPart(HandleSession->Sock);

        for(i=0;i<20;i++) {
            if(HandleSession->nRetCode==AIRKISS_SUCCESS||HandleSession->nRetCode==AIRKISS_TIME_OUT)
            {
                break;
            }

            sendMagicCode(HandleSession->Sock,HandleSession->password,HandleSession->ssid);
        }
        for(i=0;i<20;i++)
        {
            if(HandleSession->nRetCode==AIRKISS_SUCCESS||HandleSession->nRetCode==AIRKISS_TIME_OUT)
            {
                break;
            }

            sendPrefixCode(HandleSession->Sock,HandleSession->password);
        }
        for(i=0;i<20;i++)
        {
            if(HandleSession->nRetCode==AIRKISS_SUCCESS||HandleSession->nRetCode==AIRKISS_TIME_OUT)
            {
                break;
            }

            int index;
            char content[5];
            memset(content,0,5);
            for (index = 0; index < data_len / 4; index++) {
                strncpy(content,&(HandleSession->Data[index*4]),4);
                content[4]='\0';
                sendSequence(HandleSession->Sock,index, content);
            }
            memset(content,0,4);
            if (data_len % 4 != 0) {
                content [data_len % 4];
                strncpy(content,&(HandleSession->Data[index*4]),(data_len % 4));
                sendSequence(HandleSession->Sock,index, content);
            }
        }
    }
}
int AirKissToNet(char *ssid,char*password)
{
    AirKissValue Handle;
    int nRetCode=AIRKISS_ERROR;
    memset(&Handle,0,sizeof(Handle));
    //data

    Handle.Data=malloc(1500);
    char randinchar=GetRandomNum();
    memset(Handle.Data,0,1500);
    strcat(Handle.Data,password);
    int len=strlen(Handle.Data);
    Handle.Data[len]=randinchar;
    Handle.Data[++len]='\0';

    strcat(Handle.Data,ssid);

    Handle.Random=randinchar;
    Handle.nRetCode=-1;
    Handle.ssid=ssid;
    Handle.password=password;
    Handle.nRetCode=AIRKISS_ERROR;

    //sock
    SockInit(1);
    Handle.Sock = socket(AF_INET, SOCK_DGRAM,0);
    if( Handle.Sock < 0 ){
        perror("socket");
        nRetCode = AIRKISS_ERROR;
        free(Handle.Data);
        return nRetCode;
    }
    int nb = 0;
    int opt = 1;
    nb=setsockopt(Handle.Sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));
    if(nb == -1)
    {
        nRetCode = AIRKISS_ERROR;
        free(Handle.Data);
        return nRetCode;
    }
    //bind
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(10000);
    local.sin_addr.s_addr = inet_addr("192.168.5.106");//htonl(INADDR_ANY);//

    if(bind(Handle.Sock, (struct sockaddr*)&local,\
            sizeof(local)) < 0){
        perror("bind");
        nRetCode = AIRKISS_ERROR;
        free(Handle.Data);
        return nRetCode;
    }


    //htHread
    HANDLE hThread1;
    HANDLE hThread2;
    hThread2 = CreateThread(NULL,0,SendData,&Handle,0,NULL);
    hThread1 = CreateThread(NULL,0,RecvResult,&Handle,0,NULL);

    WaitForSingleObject(hThread1,INFINITE);
    WaitForSingleObject(hThread2,INFINITE);

    CloseHandle(hThread1);
    CloseHandle(hThread2);
    closesocket(Handle.Sock);
    SockInit(0);
    nRetCode=Handle.nRetCode;
    free(Handle.Data);
    return nRetCode;
}
