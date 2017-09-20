#ifndef AIRKISS_H
#define AIRKISS_H
#include<stdio.h>
#include<assert.h>
#include<stdlib.h>
#include<winsock2.h>
#include<windows.h>
#pragma comment(lib, "Ws2_32.lib")

#define      AIRKISS_TIME_OUT   1
#define      AIRKISS_SUCCESS    0
#define      AIRKISS_ERROR     -1
//WSA
typedef struct AirKissValue
{
    int   Sock;
    int   Random;
    char  *Data;
    int   nRetCode;
    char *ssid;
    char *password;
}AirKissValue;

#define BROADCASE_ADD 255.255.255.255
#define RANDOMCHAR  (rand() % 127)
int AirKissToNet(char *ssid,char*password);

#endif // AIRKISS_H
