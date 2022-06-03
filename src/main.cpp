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

void threadRunner(ClientSocket client)
{
    ServerSocket server;
    bool client_would_block, server_would_block;
    while (true)
    {
        // Read client message
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
        if (client_result.message.isEmpty())
        {
            continue;
        }
        GFD::threadedCout("Successful connection");
        if (server.connectTo(80, client_result.message.host()) == false)
        {
            client.send(HTTPMessage("HTTP/1.1 400 Bad Request\r\n\r\n"));
            break;
        }
        // If we have a server connection, check for caching and send
        bool cached_msg = false;
        HTTPMessage no_modified = HTTPMessage(client_result.message);
        if (cache.containsItem(client_result.message))
        {
            GFD::threadedCout("Found cached message");
            cached_msg = true;
            client_result.message.addIffModifiedSince(cache.getTimestamp(client_result.message));
        }
        server.send(client_result.message);
        SocketResult server_result = server.receive();
        server_would_block = server_result.err == EWOULDBLOCK;
        
        //If cached
        if (!server_result.message.isEmpty() && server_result.message.getStatusCode() == 304)
        {
            GFD::threadedCout("Message unmodified");
            string s = cache.getItem(no_modified).to_string();
            GFD::threadedCout("FOUND CACHED MESSAGE of size ", s.length(), " bytes");
            client.send(s);
            cache.refreshItem(no_modified);
        }
        else //If not cached
        {
            client.send(server_result.message.to_string());
            cache.insertItem(no_modified, server_result.message);
        }
        break;
    }
    client.disconnect();
}

void runProxy(int port)
{
    ClientSocketListener listener(port);
    while (true)
    {
        ClientSocket sock = listener.acceptClient();
        if (sock.getFD() >= 0)
        {
            std::thread th(threadRunner, std::move(sock));
            th.detach();
        }
    }
}

int main()
{
    runProxy(8082);
    return 0;
}
