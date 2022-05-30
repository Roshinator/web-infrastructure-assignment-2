#pragma once

#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "GlobalItems.hpp"
#include "HTTPMessage.hpp"

using std::cout;
using std::endl;
using std::string;

/// Wrapper for a to server HTTP TCP socket
class ServerSocket
{
    static constexpr uint16_t RECV_BUFFER_SIZE = 1024;

    const int addr_len = sizeof(struct sockaddr_in);
    uint8_t RECV_BUFFER[RECV_BUFFER_SIZE];

    int sockfd = -1;
    struct addrinfo *server_addr = NULL;
    bool connected = false;

  public:
    ServerSocket(){};
    bool connectTo(int port, string addr);
    void send(const HTTPMessage &item);
    bool isConnected();
    void disconnect();
    SocketResult receive();
    ~ServerSocket();
};

/// Set server connection
/// @param port connection port
/// @param addr address to connect to
bool ServerSocket::connectTo(int port, string addr)
{
    // Close any existing socket
    if (sockfd != 0)
    {
        GFD::executeLockedFD([&] { close(sockfd); });
        freeaddrinfo(server_addr);
        sockfd = -1;
    }

    GFD::threadedCout("Resolving name from DNS");

    // Use getaddrinfo is thread safe, gethostbyname is not
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME | AI_ALL | AI_ADDRCONFIG;
    getaddrinfo(addr.c_str(), std::to_string(port).c_str(), &hints, &server_addr);

    if (server_addr == NULL)
    {
        GFD::threadedCout("DNS Resolution failed, ignoring request");

        GFD::executeLockedFD([&] { close(sockfd); });

        //        GFD::fdMutex.lock();
        //        close(sockfd);
        //        GFD::fdMutex.unlock();

        sockfd = -1;
        return false;
    }

    GFD::threadedCout("Opening server socket for host: ", addr);

    GFD::executeLockedFD(
        [&] { sockfd = socket(server_addr->ai_family, server_addr->ai_socktype, server_addr->ai_protocol); });

    GFD::threadedCout("Connecting to server");
    int conn = connect(sockfd, server_addr->ai_addr, server_addr->ai_addrlen);
    if (conn < 0)
    {
        GFD::threadedCout("Failed to connect to server\n",
                          inet_ntoa(((struct sockaddr_in *)server_addr->ai_addr)->sin_addr));
        GFD::executeLockedFD([&] { close(sockfd); });
        sockfd = -1;
        return false;
    }
    // Set non-blocking on the socket
    int flags = fcntl(sockfd, F_GETFL);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    GFD::threadedCout(
        "Connection established with server IP: ", inet_ntoa(((struct sockaddr_in *)server_addr->ai_addr)->sin_addr),
        " and port: ", ntohs(((struct sockaddr_in *)server_addr->ai_addr)->sin_port));
    connected = true;
    return true;
}

/// Returns true if socket is connected
bool ServerSocket::isConnected()
{
    return connected;
}

ServerSocket::~ServerSocket()
{
    GFD::executeLockedFD([&] { close(sockfd); });
    freeaddrinfo(server_addr);
}

/// Send a message over the socket
/// @param item HTTP message to send
void ServerSocket::send(const HTTPMessage &item)
{
    GFD::threadedCout("Sending message to server");
    const std::string &s = item.to_string();
    int len;
    while (true)
    {
        errno = 0;
        len = ::send(sockfd, s.data(), s.length(), 0);
        if (len > 0 || (errno != EWOULDBLOCK && errno != EAGAIN))
        {
            break;
        }
    }
    GFD::threadedCout("Sent ", len, " bytes from ", s.length(), " sized packet to server");
}

/// Receives a message and returns the message and error code from the recv call
SocketResult ServerSocket::receive()
{
    std::string s;
    ssize_t status;
    int count = 0;
    bool exitedBlock = false;
    while (!exitedBlock)
    {
        while ((status = recv(sockfd, RECV_BUFFER, RECV_BUFFER_SIZE, 0)) > 0)
        {
            exitedBlock = true;
            count += status;
            GFD::threadedCout("Receiving message from server");
            s.append((char *)RECV_BUFFER, status);
            std::fill_n(RECV_BUFFER, RECV_BUFFER_SIZE, 0);
        }
    }
    //    cout << "FILE DESC: " << sockfd << endl;
    int err = errno;
    errno = 0;
    return SocketResult{HTTPMessage(s), status, err};
}
