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
    std::string parseHeader(const std::string& header) const;
    std::string parseBody() const;

  public:
    HTTPMessage(const std::string &s);
    HTTPMessage(const HTTPMessage &m);
    bool isEmpty() const;
    std::string host();
    void addIffModifiedSince(const std::time_t& timestamp);
    int getStatusCode() const;
    bool contentComplete() const;
    void append(std::string& s);

    friend std::ostream &operator<<(std::ostream &out, const HTTPMessage &msg);
    HTTPMessage operator=(HTTPMessage in)
    {
        raw_text = in.raw_text;
        hostname = parseHeader("Host");
        return *this;
    }
    const std::string &to_string() const;
};
