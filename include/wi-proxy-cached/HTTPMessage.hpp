#pragma once

#include <map>
#include <string>
#include <chrono>
#include <mutex>

/// Represents an http message
class HTTPMessage
{
    std::string hostname;
    std::string raw_text;
    std::mutex date_mutex;
    void parseHeader();

  public:
    HTTPMessage(const std::string &s);
    HTTPMessage(const HTTPMessage &m);
    bool isEmpty();
    std::string host();
    void addIffModifiedSince(const std::time_t& timestamp);
    int getStatusCode() const;

    friend std::ostream &operator<<(std::ostream &out, const HTTPMessage &msg);
    const std::string &to_string() const;
};
