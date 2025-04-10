#ifndef _OSSSOCKET_HPP
#define _OSSSOCKET_HPP

#include "import.hpp"


#define SOCKET_GETLASTERROR errno
#define OSS_SOCKET_DFT_TIMEOUT 10000 // 10ms
#define OSS_MAX_HOSTNAME NI_MAXHOST
#define OSS_MAX_SERVICENAME NI_MAXSERV

namespace edb {

class ossSocket 
{
private:
    int fd_;
    socklen_t addressLen_;
    socklen_t peerAddressLen_;
    struct sockaddr_in sockAddress_;
    struct sockaddr_in peerAddress_;
    bool init_;
    int timeout_;

protected:
    unsigned int getPort( sockaddr_in *addr );
    int getAddress( sockaddr_in *addr, char *pAddress, unsigned int len );

public:
    int setSocketLi( int lOnOff, int linger );
    void setAddress(const char *pHostname, unsigned int port);

    ossSocket();
    ossSocket( unsigned int port, int timeout=0 );
    
    // create connecting socket
    ossSocket(const char *pHostname, unsigned int port, int timeout=0);
    ossSocket(int *sock, int timeout=0);

    ~ossSocket() 
    {
        close();
    }

    int initSocket();
    int bindListen();
    int send(const char *msg, int len, int timeout=OSS_SOCKET_DFT_TIMEOUT, int flag=0);
    int recv(char *msg, int len, int timeout=OSS_SOCKET_DFT_TIMEOUT, int flag=0);
    int recvNonBlock(char *msg, int& len, int timeout=OSS_SOCKET_DFT_TIMEOUT, int flag=0);

    int connect();
    bool isConnected();
    void close();
    int accept(int *sock, struct sockaddr *addr, socklen_t *addrlen, int timeout=OSS_SOCKET_DFT_TIMEOUT);
    int disableNagle();
    unsigned int getPeerPort();
    int getPeerAddress(char *pAddress, unsigned int len);
    unsigned int getLocalPort();
    int getLocalAddress(char *pAddress, unsigned int len);
    int setTimeout(int secs);

    static int getHostName(char *pName, int len);
    static int getPort(const char *pServiceName, unsigned short& port);
};

}



#endif