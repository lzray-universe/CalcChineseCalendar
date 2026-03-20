#pragma once

#include<filesystem>
#include<string>

std::string test_ephem();
bool has_test_ephem();

std::filesystem::path make_temp_path(const char*stem,const char*ext);

std::string read_file_text(const std::filesystem::path&path);

std::string txt_value(const std::string&text,const std::string&key);
