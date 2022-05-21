#pragma once

#include <cstdarg>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "HTTPMessage.hpp"

/// Describes a socket recv result
struct SocketResult
{
    HTTPMessage message; // Message
    ssize_t status;      // Status code from recv
    int err;             // cerrno created from the run
};

class GFD
{
  public:
    template <typename T, typename... Tetc>
    static void threadedCout(T v1, Tetc... v2)
    {
        coutMutex.lock();
        std::cout << printThread();
        threadedCout_internal(v1, v2...);
        coutMutex.unlock();
    }

    static void executeLockedFD(std::function<void()> param)
    {
        fdMutex.lock();
        param();
        fdMutex.unlock();
    }

  private:
    inline static std::mutex coutMutex;
    inline static std::mutex fdMutex;

    template <typename T, typename... Tetc>
    static void threadedCout_internal(T v1, Tetc... v2)
    {
        std::cout << v1;
        threadedCout_internal(v2...);
    }

    static void threadedCout_internal()
    {
        std::cout << std::endl;
    }

    static std::string printThread()
    {
        std::stringstream stream;
        stream << "Thread " << std::this_thread::get_id() << ": ";
        return stream.str();
    }
};
