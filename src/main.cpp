#include "ClientSocket.hpp"
#include "ClientSocketListener.hpp"
#include "GlobalItems.hpp"
#include "HTTPMessage.hpp"
#include "ServerSocket.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include "CacheStorage.hpp"

using namespace std::chrono_literals;

using std::cout;
using std::endl;
using std::string;

static CacheStorage cache;
constexpr uint64_t spin_wait_threshold = 500000;

std::chrono::milliseconds calculateThreadWait(uint64_t spin_wait_count)
{
    if (spin_wait_count > spin_wait_threshold / 10)
    {
        double wait_time = 5e-9 * std::pow(spin_wait_count - spin_wait_threshold / 10, 2);
        wait_time = std::min(1000.0, wait_time);
        return std::chrono::milliseconds((int)wait_time);
    }
    return 0ms;
}

void threadRunner(ClientSocket client)
{
    ServerSocket server;
    bool client_would_block, server_would_block;
    uint64_t spin_wait_count = 0;
    while (true)
    {
        // Poll read client message
        SocketResult client_result = client.receive();
        client_would_block = client_result.err == EWOULDBLOCK;
        // If an error occurred, reset connection
        if (client_result.status <= 0 && !client_would_block)
        {
            GFD::threadedCout("Client disconnected, resetting socket");

            if (client_result.status < 0)
            {
                GFD::threadedCout(strerror(client_result.err));
            }
            break;
        }
        // If message has content, check if server connection needs to be updated
        if (!client_result.message.isEmpty())
        {
            GFD::threadedCout("Successful connection");
            if (server.connectTo(80, client_result.message.host()) == false)
            {
                client.send(HTTPMessage("HTTP/1.1 400 Bad Request\r\n\r\n"));
                break;
            }
        }
        // If we have a server connection, send packet and poll receive
        if (server.isConnected())
        {
            if (!client_result.message.isEmpty()) // If client message has content, send to server
            {
                spin_wait_count = 0;
                server.send(client_result.message);
            }
            SocketResult server_result = server.receive();
            server_would_block = server_result.err == EWOULDBLOCK;

            if (!server_result.message.isEmpty()) // Forward back to client
            {
                //                cout << server_msg.to_string() << endl;
                client.send(server_result.message.to_string());
                spin_wait_count = 0;
            }
        }

        spin_wait_count++;
        // Slow down polling if nothing has come through in a while to ease cpu
        // usage
        std::chrono::milliseconds wait_time = calculateThreadWait(spin_wait_count);
        if (wait_time >= 0ms)
        {
            std::this_thread::sleep_for(wait_time);
        }
    }
    client.disconnect();
}

void runProxy(int port)
{
    ClientSocketListener listener(port);
    uint64_t spin_wait_count = 0;
    while (true)
    {
        ClientSocket sock = listener.acceptClient();
        if (sock.getFD() >= 0)
        {
            spin_wait_count = 0;
            std::thread th(threadRunner, std::move(sock));
            th.detach();
        }
        spin_wait_count++;
        // Slow down polling if nothing has come through in a while to ease cpu
        // usage
        std::chrono::milliseconds wait_time = calculateThreadWait(spin_wait_count);
        if (wait_time >= 0ms)
        {
            std::this_thread::sleep_for(wait_time);
        }
    }
}

int main()
{
    runProxy(8081);
    return 0;
}
