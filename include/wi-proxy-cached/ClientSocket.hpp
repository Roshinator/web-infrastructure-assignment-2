#pragma once

#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "GlobalItems.hpp"
#include "HTTPMessage.hpp"

using std::cout;
using std::endl;
using std::string;

/// Wrapper for a to client HTTP TCP socket
class ClientSocket
{
    static constexpr uint16_t RECV_BUFFER_SIZE = 1024;
    const int test = EWOULDBLOCK;

    const int addr_len = sizeof(struct sockaddr_in);
    uint8_t RECV_BUFFER[RECV_BUFFER_SIZE];

    int client_sockfd;
    struct sockaddr_in client_addr;

  public:
    ClientSocket(struct sockaddr_in addr, int sockfd);
    void listenAndAccept();
    void send(const HTTPMessage &item);
    int getFD();
    void disconnect();
    SocketResult receive();
};

/// Constructs a client socket
/// @param port port to listen on
ClientSocket::ClientSocket(struct sockaddr_in addr, int sockfd)
{
    client_addr = addr;
    client_sockfd = sockfd;
}

void ClientSocket::disconnect()
{
    if (client_sockfd != 0)
    {
        GFD::executeLockedFD([&] { close(client_sockfd); });
    }
}

int ClientSocket::getFD()
{
    return client_sockfd;
}

/// Sends a message to the client
/// @param item message to send
void ClientSocket::send(const HTTPMessage &item)
{
    const string &s = item.to_string();
    GFD::threadedCout("Sending message to client");
    int len;
    len = ::send(client_sockfd, s.data(), s.length(), 0);
    errno = 0;
    GFD::threadedCout("Sent ", len, " bytes from ", s.length(), " sized packet to client");
}

/// Receives a message and returns the message and error code from the recv call
SocketResult ClientSocket::receive()
{
    ssize_t status;
    HTTPMessage m("");
    while ((status = recv(client_sockfd, RECV_BUFFER, RECV_BUFFER_SIZE, 0)) > 0)
    {
        GFD::threadedCout("Receiving message from client");
        std::string s;
        s.append((char *)RECV_BUFFER, status);
        m = HTTPMessage(s);
        std::fill_n(RECV_BUFFER, RECV_BUFFER_SIZE, 0);
        bool complete = m.contentComplete();
        if (complete)
        {
            break;
        }
    }
    //    cout << "FILE DESC: " << client_sockfd << endl;
    int err = errno;
    errno = 0;
    return SocketResult{m, status, err};
}
