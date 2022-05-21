#pragma once

#include <map>
#include <string>

/// Represents an http message
class HTTPMessage
{
    std::string hostname;
    std::string raw_text;

    void parseHeader();

  public:
    HTTPMessage(const std::string &s);
    HTTPMessage(const HTTPMessage &m);
    bool isEmpty();
    std::string host();

    friend std::ostream &operator<<(std::ostream &out, const HTTPMessage &msg);
    const std::string &to_string() const;
};
