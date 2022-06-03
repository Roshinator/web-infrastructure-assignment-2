#include "HTTPMessage.hpp"
#include "GlobalItems.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>

#define CRLF "\r\n"
#define HEADER_SPLIT "\r\n\r\n"

using std::cout;
using std::endl;
using std::string;

/// Copy constructor
/// @param msg Other message
HTTPMessage::HTTPMessage(const HTTPMessage &msg)
{
    hostname = msg.hostname;
    raw_text = string(msg.raw_text);
}

/// Constructs and parses header
/// @param s string to construct a message from
HTTPMessage::HTTPMessage(const string &s)
{
    raw_text = string(s);
    hostname = parseHeader("Host");
}

/// Gets the hostname
string HTTPMessage::host()
{
    return hostname;
}

void HTTPMessage::append(std::string &s)
{
    raw_text.append(s);
}

/// Returns true if the message is empty
bool HTTPMessage::isEmpty() const
{
    return raw_text.empty();
}

int HTTPMessage::getStatusCode() const
{
    string top_line = raw_text.substr(9, 3);
    return std::atoi(top_line.data());
}

/// Parses header
std::string HTTPMessage::parseHeader(const std::string& header) const
{
    int split_loc = 0;
    if (!isEmpty())
    {
        split_loc = raw_text.find(HEADER_SPLIT);
        string headers = raw_text.substr(0, split_loc);
        // Search for "\r\nHost: *\r\n"
        const string pre = string("\r\n").append(header).append(": ");
        const string post = "\r\n";
        size_t pre_loc = headers.find(pre);
        if (pre_loc != string::npos)
        {
            pre_loc += pre.length();
            string pre_cut = headers.substr(pre_loc, headers.length() - pre_loc);
            size_t post_loc = pre_cut.find(post);
            if (post_loc != string::npos)
            {
                string post_cut = pre_cut.substr(0, post_loc);
                return post_cut;
            }
        }
    }
    return "";
}

/// Returns the body component of the message
std::string HTTPMessage::parseBody() const
{
    int split_loc = 0;
    if (!isEmpty())
    {
        split_loc = raw_text.find(HEADER_SPLIT);
        if (split_loc == string::npos)
        {
            return "";
        }
        split_loc += 4;
        return raw_text.substr(split_loc, raw_text.length() - split_loc);
    }
    return "";
}

std::ostream &operator<<(std::ostream &out, const HTTPMessage &msg)
{
    return out << msg.raw_text;
}

/// Gets the raw text from the message
const string &HTTPMessage::to_string() const
{
    return raw_text;
}

/// Adds an if modified since timestamp header
/// @param timestamp timestamp to use
void HTTPMessage::addIffModifiedSince(const std::time_t& timestamp)
{
    date_mutex.lock();
    
    std::tm* gmt_time = std::gmtime(&timestamp);
    std::stringstream data;
    data << "\r\nIf-Modified-Since: ";
    data << std::put_time(gmt_time, "%a, %d %b %Y %T GMT");
    GFD::threadedCout("ADDED MODIFICATION DATE: ", data.str());
    raw_text.insert(raw_text.find("\r\n\r\n"), data.str());
    date_mutex.unlock();
}

/// Returns true if there is still data to parse
bool HTTPMessage::contentComplete() const
{
    if (isEmpty())
    {
        return true;
    }
    std::string body = std::string(parseBody());
    std::string content_length_header = parseHeader("Content-Length");
    std::string chunked_header = parseHeader("Transfer-Encoding");
    bool is_chunked = chunked_header.find("chunked") != std::string::npos;
    bool is_content = !is_chunked && !content_length_header.empty();
    
    if (is_chunked)
    {
        if (body.empty())
            return false;
        if (body.find("0\r\n\r\n") != std::string::npos)
            return true;
        else
            return false;
    }
    else if (is_content)
    {
        if (body.empty())
            return false;
        return (std::stoi(content_length_header) - body.length()) == 0;
    }
    else
    {
        if (raw_text.find(HEADER_SPLIT) == std::string::npos) //Headers incomplete
        {
            return false;
        }
        return true;
    }
}
