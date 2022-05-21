#pragma once

#include <filesystem>
#include <map>
#include <mutex>
#include <fstream>
#include <iostream>

class CacheStorage
{
    const std::filesystem::path cache_folder_name = "cache_data";
    const std::string_view cache_file_name = "cache_cell_";
    const int max_cache_size = 100;
    
    std::map<std::string, std::filesystem::path> lookup;
    std::mutex lookup_lock;
    
    void writeCell(std::string s, int index);
    
public:
    CacheStorage();
};

CacheStorage::CacheStorage()
{
    std::filesystem::create_directory(cache_folder_name);
}


/// Replaces a cache file
/// @param s file contents
/// @param index cache index being replaced
void CacheStorage::writeCell(std::string s, int index)
{
    using namespace std::filesystem;
    
    path p = cache_folder_name / cache_file_name / std::to_string(index);
    std::ofstream ofs(p, std::ios::trunc); //Clear the file when it's opened
    ofs << s;
    ofs.close();
}
