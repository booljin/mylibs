#include <winsock2.h>
#include <stdio.h>
#pragma comment(lib,"ws2_32.lib")
int main()
{
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD( 2, 2 );
    int err = WSAStartup( wVersionRequested, &wsaData );
    
    SOCKET s;
    SOCKADDR_IN sa;
    
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s == INVALID_SOCKET)
    {
        printf("socket create err \n");
        closesocket(s);
        WSACleanup();
        return 0;
    }
    int nAddrLen = sizeof(SOCKADDR_IN);  
    memset(&sa, 0, nAddrLen);  
    sa.sin_family = AF_INET;  
    sa.sin_port   = htons(10096);  
    sa.sin_addr.s_addr = inet_addr("192.168.127.128");
    
    int nRet = connect(s, (LPSOCKADDR)&sa, nAddrLen);  
    if (SOCKET_ERROR == nRet)  
    {  
        printf("socket connect() failse:%d/n", WSAGetLastError());  
        closesocket(s);  
        WSACleanup();  
        return 0;  
    }
    char szBuffer[3] = {'a', 'b', '\0'};
    nRet = send(s, szBuffer, strlen(szBuffer), 0);  
    if (SOCKET_ERROR == nRet)  
    {  
        printf("Socket send() failse:%d/n", WSAGetLastError());   
    }  
    closesocket(s);
    WSACleanup();
    return 0;
}