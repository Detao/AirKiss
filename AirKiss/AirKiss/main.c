#include"airkiss.h"
int main()
{
    int nRetCode;
    printf("AirKiss Connecting.......\n");
    nRetCode=AirKissToNet("gebilaowang","yy939495");
    if(nRetCode==0)
    {
        printf("AirKiss Connect Success\n");
    }
    else if(nRetCode==1)
    {
        printf("AisKiss Time Out\n");
    }
    else {
        printf("AirKiss Error\n");
    }
    return 0;
}
