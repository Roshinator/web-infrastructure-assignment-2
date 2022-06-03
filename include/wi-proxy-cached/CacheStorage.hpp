#pragma once

#include <filesystem>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iterator>
#include "HTTPMessage.hpp"
#include "GlobalItems.hpp"

typedef std::time_t SystemTimestamp;

struct CacheItem
{
    int index;
    SystemTimestamp timestamp;
};

class CacheStorage
{
    const std::filesystem::path cache_folder_name = "cache_data";
    const std::string_view cache_file_name = "cache_cell_";
    const int cache_size = 20;
    
    std::unordered_map<std::string, CacheItem> lookup;
    std::mutex lookup_lock;
    
    void writeCell(std::string s, int index);
    std::string readCell(int index);
    SystemTimestamp getTime();
    
public:
    CacheStorage();
    
    bool containsItem(HTTPMessage& msg);
    SystemTimestamp getTimestamp(HTTPMessage& msg);
    void insertItem(HTTPMessage& msg, HTTPMessage& response);
    void refreshItem(HTTPMessage& msg);
    HTTPMessage getItem(HTTPMessage& msg);
};

CacheStorage::CacheStorage()
{
    std::filesystem::create_directory(cache_folder_name);
    for (int i = 1; i <= cache_size; i++) //Fill the table with garbage
    {
        lookup.insert
        (
            {
                std::to_string(i),
                CacheItem
                {
                    i,
                    getTime()
                }
            }
         );
    }
}

/// Replaces a cache file
/// @param s file contents
/// @param index cache index being replaced
void CacheStorage::writeCell(std::string s, int index)
{
    using namespace std::filesystem;
    
    path p = "." / cache_folder_name / std::string(cache_file_name).append(std::to_string(index)).append(".txt");
    std::ofstream ofs(p, std::ios::trunc); //Clear the file when it's opened
    ofs << s;
    ofs.close();
}

/// Reads a cache file
/// @param index index of the cache cell
std::string CacheStorage::readCell(int index)
{
    using namespace std::filesystem;
    
    path p = "." / cache_folder_name / std::string(cache_file_name).append(std::to_string(index)).append(".txt");
    std::ifstream ifs(p);
    string s(std::istreambuf_iterator<char>{ifs}, {});
    ifs.close();
    return s;
}

/// Returns true if the message is in the cache
/// @param msg message to check
bool CacheStorage::containsItem(HTTPMessage &msg)
{
    lookup_lock.lock();
    bool contained = lookup.count(msg.to_string());
    lookup_lock.unlock();
    return contained;
}

/// Gets the timestamp of a cached message
/// @param msg message to check
SystemTimestamp CacheStorage::getTimestamp(HTTPMessage &msg)
{
    lookup_lock.lock();
    auto t = lookup.at(msg.to_string()).timestamp;
    lookup_lock.unlock();
    return t;
}

/// Inserts an item into the cache following LRU
/// @param msg message to check
/// @param response response paired with the message
void CacheStorage::insertItem(HTTPMessage &msg, HTTPMessage& response)
{
    lookup_lock.lock();
    
    std::string key = msg.to_string();
    if (lookup.find(key) != lookup.end()) //If message is in the cache
    {
        lookup[key].timestamp = getTime();
    }
    else
    {
        std::pair<std::string, CacheItem> lowest;
        for (auto item : lookup)
        {
            if (lowest.first.empty())
                lowest = item;
            if (item.second.timestamp < lowest.second.timestamp)
            {
                lowest = item;
            }
        }
        lookup.erase(lowest.first);
        lowest.second.timestamp = getTime();
        lowest.first = msg.to_string();
        lookup.insert(lowest);
        writeCell(response.to_string(), lowest.second.index);
    }
    
    lookup_lock.unlock();
}

/// Updates the timestamp of a cache (recently used)
/// @param msg message to update
void CacheStorage::refreshItem(HTTPMessage &msg)
{
    lookup_lock.lock();
    std::string key = msg.to_string();
    if (lookup.find(key) != lookup.end()) //If message is in the cache
    {
        lookup[key].timestamp = getTime();
    }
    lookup_lock.unlock();
}

/// Gets an item from the cache
/// @param msg description of the item
HTTPMessage CacheStorage::getItem(HTTPMessage &msg)
{
    lookup_lock.lock();
    auto item = lookup.at(msg.to_string());
    int index = item.index;
    item.timestamp = getTime();
    std::string data = readCell(index);
    lookup_lock.unlock();
    return HTTPMessage(data);
}

/// Gets the current time as a timestamp
SystemTimestamp CacheStorage::getTime()
{
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}
