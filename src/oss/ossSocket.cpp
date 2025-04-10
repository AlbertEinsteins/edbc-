#include "ossSocket.hpp"
#include "common.hpp"
#include "pd.hpp"

#define MAX_RECV_RETRIES 5

namespace edb {
    
ossSocket::ossSocket(unsigned int port, int timeout)
{
    init_ = false;
    fd_ = 0;
    timeout_ = timeout;
    memset(&sockAddress_, 0, sizeof(sockaddr_in));
    memset(&peerAddress_, 0, sizeof(sockaddr_in));
    peerAddressLen_ = sizeof(peerAddressLen_);

    sockAddress_.sin_family = AF_INET;
    sockAddress_.sin_addr.s_addr = htonl(INADDR_ANY);
    sockAddress_.sin_port = htons(port);
    addressLen_ = sizeof(sockAddress_);
}


ossSocket::ossSocket()
{
    init_ = false;
    fd_ = 0;
    timeout_ = 0;
    memset(&sockAddress_, 0, sizeof(sockaddr_in));
    memset(&peerAddress_, 0, sizeof(sockaddr_in));
    addressLen_ = sizeof(sockAddress_);
    peerAddressLen_ = sizeof(sockAddress_);
}

ossSocket::ossSocket(const char *pHostname, unsigned int port, int timeout)
{
    struct hostent *he = NULL;
    init_ = false;
    fd_ = 0;
    timeout_ = timeout;
    memset(&sockAddress_, 0, sizeof(sockaddr_in));
    memset(&peerAddress_, 0, sizeof(sockaddr_in));
    peerAddressLen_ = sizeof(peerAddress_);

    sockAddress_.sin_family = AF_INET;
    he = gethostbyname(pHostname);
    if ( NULL != he ) {
        sockAddress_.sin_addr.s_addr = *((int *)he->h_addr_list[0]);
    } else {
        sockAddress_.sin_addr.s_addr =  inet_addr(pHostname);
    }
    sockAddress_.sin_port = htons(port);
    addressLen_ = sizeof(sockAddress_);
}

ossSocket::ossSocket(int *sock, int timeout)
{
    int rc = edb::EDB_OK;
    fd_ = *sock;
    init_ = true;
    timeout_ = timeout;
    addressLen_ = sizeof(socklen_t);
    peerAddressLen_ = sizeof(socklen_t);
    memset(&peerAddress_, 0, sizeof(sockaddr_in));
    if ((rc = getsockname(fd_, (struct sockaddr *)&sockAddress_, &addressLen_))) {
        PD_LOG(ERROR, "Failed to get the local hostname, errno = %d\n", SOCKET_GETLASTERROR);
        init_ = false;
    } else {
        rc = getpeername(fd_, (struct sockaddr *)&peerAddress_, &peerAddressLen_);
        if (rc) {
            printf("Failed to get the peer name, errno = %d\n", SOCKET_GETLASTERROR);
            goto error;
        }
    }

    printf("Get a new socket, socket fd = %d\n", fd_);

done:
    return;
error:
    goto done;

}


int ossSocket::initSocket()
{
    int rc = edb::EDB_OK;
    if (init_) {
        goto done;
    }

    fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (0 > fd_) {
        printf("Failed to init socket, err = %d\n", SOCKET_GETLASTERROR);
        rc = edb::EDB_NETWORK;
        goto error;
    }

    init_ = true;
    setTimeout(timeout_);
done:
    return rc;
error:
    goto done;
}


int ossSocket::setSocketLi(int lOnOff, int linger)
{
    struct linger li;
    li.l_onoff = lOnOff;
    li.l_linger = linger;
    return ::setsockopt(fd_, SOL_SOCKET, SO_LINGER, &li, sizeof(li));
}


void ossSocket::setAddress(const char *pHostname, unsigned int port)
{
    struct hostent *he;
    memset(&sockAddress_, 0, sizeof(sockaddr_in));
    memset(&peerAddress_, 0, sizeof(sockaddr_in));
    peerAddressLen_ = sizeof(peerAddress_);

    sockAddress_.sin_family = AF_INET;
    if ( (he = gethostbyname(pHostname))) {
        sockAddress_.sin_addr.s_addr = *((int *)he->h_addr_list[0]);
    } else {
        sockAddress_.sin_addr.s_addr = inet_addr(pHostname);
    }

    sockAddress_.sin_port = htons(port);
    addressLen_ = sizeof(sockAddress_);
}

int ossSocket::bindListen()
{
    int rc = edb::EDB_OK;
    int flag = 1;
    rc = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
    if (rc) {
        printf("Failed to setsockopt, rc = %d\n", SOCKET_GETLASTERROR);
    }
    rc = setSocketLi(1, 30);
    if (rc) {
        printf("Failed to set socket linger, rc = %d\n", SOCKET_GETLASTERROR);

    }
    rc = ::bind(fd_, (struct sockaddr *)&sockAddress_, sizeof(sockAddress_));
    if (rc) {
        printf("Failed to bind socket address, rc = %d\n", SOCKET_GETLASTERROR);
        goto error;
    }

    rc = ::listen(fd_, SOMAXCONN);
    if (rc) {
        printf("Failed to listen address, rc = %d\n", SOCKET_GETLASTERROR);
        goto error;
    }
done:
    return rc;
error:
    close();
    goto done;
}


int ossSocket::send(const char *msg, int len, int timeout, int flag)
{
    int rc = edb::EDB_OK;
    struct timeval timeout_val;
    fd_set fds;
    int max_fd = fd_;

    // set timeout val
    timeout_val.tv_sec = timeout / 1'000'000;
    timeout_val.tv_usec = timeout % 1'000'000;

    if (0 == len) {
        return rc;
    }
    while (true) {
        FD_ZERO(&fds);
        FD_SET(fd_, &fds);

        rc = select(max_fd + 1, NULL, &fds, NULL, 
            timeout >= 0 ? &timeout_val : NULL);
        if (0 == rc) {
            // timeout
            rc = edb::EDB_NETWORK;
            goto error;
        }
        if (0 > rc) {
            rc = SOCKET_GETLASTERROR;
            if (EINTR == rc) {
                // continue if interuprting
                continue;
            }
            printf("Failed to select socket, rc = %d\n", rc);
            goto error;
        }

        if (FD_ISSET(fd_, &fds)) {
            break;
        }
    }

    while (len) {
        // if the peer of the other side breaks the pipeline, then will receive a signal
        int sz = ::send(fd_, msg, len, MSG_NOSIGNAL | flag);
        if (0 > sz) {
            printf("Failed to send msg, rc = %d\n", SOCKET_GETLASTERROR);
            goto error;
        }
        msg += sz;
        len -= sz;
    }

    rc = EDB_OK;
done:
    return rc;
error:
    goto done;
}

int ossSocket::recv(char *msg, int len, int timeout, int flag)
{
    int rc = edb::EDB_OK;
    int max_fd = fd_;
    struct timeval timeout_val;
    fd_set fds;
    int retry = 0;

    timeout_val.tv_sec = timeout / 1'000'000;
    timeout_val.tv_usec = timeout % 1'000'000;

    if (0 == len) {
        return rc;
    }
    while (true) {
        FD_ZERO(&fds);
        FD_SET(fd_, &fds);

        rc = select(max_fd + 1, &fds, NULL, NULL,
            timeout >= 0 ? &timeout_val : NULL);
        if (0 == rc) {
            // timeout
            rc = edb::EDB_TIMEOUT;
            goto error;
        }
        if (0 > rc) {
            rc = SOCKET_GETLASTERROR;
            if (EINTR == rc) {
                continue;
            }
            printf("Failed to select in recving, rc = %d\n", rc);
            goto error;
        }

        if (FD_ISSET(fd_, &fds)) {
            break;
        }
    }

    while (len) {
        int sz = ::recv(fd_, msg, len, MSG_NOSIGNAL | flag);
        if (0 < sz) {
            if (MSG_PEEK & flag) {
                goto done;
            }
            len -= sz;
            msg += sz;
        } else if (0 == sz) {
            // may the peer shutdown the socket
            printf("Failed recv data, cause the peer shutdown");
            rc = edb::EDB_NETWORK_CLOSE;
            goto error;
        } else {
            rc = SOCKET_GETLASTERROR;
            if ((EAGAIN == rc || EWOULDBLOCK == rc) && timeout_ > 0) {
                printf("Recv timeout, rc = %d\n", rc);
                rc = edb::EDB_NETWORK;
                goto error;
            }
            if (EINTR == rc && retry < MAX_RECV_RETRIES) {
                retry ++;
                continue ;
            }
            printf("Recv failed, rc = %d\n", rc);
            rc = edb::EDB_NETWORK;
            goto error;
        }
    }
    rc = edb::EDB_OK;
done:
    return rc;
error:
    goto done;
}

int ossSocket::recvNonBlock(char *msg, int& len, int timeout, int flag)
{
    int rc = edb::EDB_OK;
    struct timeval timeout_val;
    fd_set fds;
    int max_fd = fd_;

    timeout_val.tv_sec = timeout / 1'000'000;
    timeout_val.tv_usec = timeout % 1'000'000;

    while (true) {
        FD_ZERO(&fds);
        FD_SET(fd_, &fds);
        rc = select(max_fd + 1, &fds, NULL, NULL, timeout >= 0 ? &timeout_val : NULL);
        if (0 == rc) {
            // timeout
            rc = edb::EDB_TIMEOUT;
            goto error;
        }
        if (0 > rc) {
            rc = SOCKET_GETLASTERROR;
            if (EINTR == rc) {
                continue;
            }
            printf("Failed to select in recving, rc = %d\n", rc);
            goto error;
        }

        if (FD_ISSET(fd_, &fds)) {
            break;
        }
    }

    rc = ::recv(fd_, msg, len, MSG_NOSIGNAL | flag);
    if (0 == rc) {
        printf("Failed to recv data, the peer may closed");
        rc = edb::EDB_NETWORK_CLOSE;
        goto error;
    }
    if (0 > rc) {
        rc = SOCKET_GETLASTERROR;
        if ((EAGAIN == rc || EWOULDBLOCK == rc) && timeout_ > 0) {
            printf("Recv data timeout, rc = %d\n", rc);
            rc = edb::EDB_NETWORK;
            goto error;
        }

        printf("Recv data failed, rc = %d\n", rc);
        goto error;        
    } else {        // return the recving data size
        len = rc;
    }

    rc = edb::EDB_OK;
done:
    return rc;
error:
    goto done;
}


int ossSocket::connect()
{
    int rc = edb::EDB_OK;
    rc = ::connect(fd_, (struct sockaddr *)&sockAddress_, addressLen_);
    if (rc) {
        printf("Failed to connect to peer, rc = %d\n", rc);
        rc = edb::EDB_NETWORK;
        goto error;
    }
    rc = getsockname(fd_, (sockaddr *)&sockAddress_, &addressLen_);
    if (rc) {
        printf("Failed to localaddress, rc = %d\n", rc);
        rc = edb::EDB_NETWORK;
        goto error;
    }
    rc = getpeername(fd_, (sockaddr *)&peerAddress_, &peerAddressLen_);
    if (rc) {
        printf("Failed to peer address, rc = %d\n", rc);
        rc = edb::EDB_NETWORK;
        goto error;
    }

done:
    return rc;
error:
    goto done;
}

bool ossSocket::isConnected() 
{
    int rc = edb::EDB_OK;
    rc = ::send(fd_, "", 0, MSG_NOSIGNAL);
    if (0 > rc) {
        return false;
    }
    return true;
}

void ossSocket::close()
{
    if (init_) {
        ::close(fd_);
        init_ = false;
    }
}

int ossSocket::accept(int *sock, struct sockaddr *addr, socklen_t *addrlen, int timeout)
{
    int rc = edb::EDB_OK;
    struct timeval timeout_val;
    int max_fd = fd_;
    fd_set fds;

    timeout_val.tv_sec = timeout / 1'000'000;
    timeout_val.tv_usec = timeout % 1'000'000;

    while (true) {
        FD_ZERO(&fds);
        FD_SET(fd_, &fds);
        rc = ::select(max_fd + 1, &fds, NULL, NULL, timeout >= 0 ? &timeout_val : NULL);
        if (0 == rc) {
            *sock = 0;
            rc = edb::EDB_TIMEOUT;
            goto done;
        }
        if (0 > rc) {   // error occurred
            rc = SOCKET_GETLASTERROR;
            if (EINTR == rc) {  // if caused by inter, then continue loop select
                continue ;
            }
            printf("Failed to select from socket, rc = %d\n", rc);
            goto error;
        }

        if (FD_ISSET(fd_, &fds)) {
            break;
        }
    }

    rc = edb::EDB_OK;
    *sock = ::accept(fd_, addr, addrlen);
    if (-1 == *sock) {
        printf("Failed to accept socket, rc = %d\n", SOCKET_GETLASTERROR);
        rc = edb::EDB_NETWORK;
        goto error;
    }
done:
    return rc;
error:
    close();
    goto done;
}

int ossSocket::disableNagle()
{
    int rc = edb::EDB_OK;
    int temp = 1;
    rc = ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, (char *)&temp, sizeof(int));
    if (rc) {
        printf("Failed to setsockopt, rc = %d\n", rc);
    }
    rc = ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, (char *)&temp, sizeof(int));
    if (rc) {
        printf("Failed to setsockopt, rc = %d\n", rc);
    }
    return rc;
}

unsigned int ossSocket::getPort(sockaddr_in *addr)
{
    return ntohs(addr->sin_port);
}

int ossSocket::getAddress(sockaddr_in *addr, char *pAddress, unsigned int len)
{
    int rc = edb::EDB_OK;
    len = len < NI_MAXHOST ? len : NI_MAXHOST;
    rc = getnameinfo((struct sockaddr *)addr, sizeof(sockAddress_), 
        pAddress, len, NULL, 0, NI_NUMERICHOST);
    if (rc) {
        printf("Failed to getnameinfo, rc = %d\n", rc);
        rc = edb::EDB_NETWORK;
        goto error;
    }


done:
    return rc;
error:
    goto done;
}

unsigned int ossSocket::getLocalPort() {
    return getPort(&sockAddress_);
}

unsigned int ossSocket::getPeerPort() {
    return getPort(&peerAddress_);
}

int ossSocket::getLocalAddress(char *pAddress, unsigned int len)
{
    return getAddress(&sockAddress_, pAddress, len);
}

int ossSocket::getPeerAddress(char *pAddress, unsigned int len)
{
    return getAddress(&peerAddress_, pAddress, len);
}

int ossSocket::setTimeout(int secs)
{
    int rc = edb::EDB_OK;
    struct timeval timeout_val;

    timeout_val.tv_sec = secs;
    timeout_val.tv_usec = 0;

    rc = ::setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_val, sizeof(timeout_val));
    if (rc) {
        printf("Failed to setsockopt recvtimeout, rc = %d\n", rc);
    }

    rc = ::setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout_val, sizeof(timeout_val));
    if (rc) {
        printf("Failed to setsockopt sndtimeout, rc = %d\n", rc);
    }
    return rc;
}

// static method
int ossSocket::getHostName(char *pName, int len)
{
    return gethostname(pName, len);
}

int ossSocket::getPort(const char *pServiceName, unsigned short& port)
{
    int rc = edb::EDB_OK;
    struct servent *sv;

    sv = getservbyname(pServiceName, "TCP");
    if (!sv) {
        port = atoi(pServiceName);
    } else {
        port = sv->s_port;
    }
    return rc;
}


}
