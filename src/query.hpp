#ifndef QUERY_HPP
#define QUERY_HPP

#include <string>
#include <vector>

std::vector<std::vector<unsigned int>> process_query(std::string query_file_path, std::string index_file_path, std::string meta_data_file_path, int k, int b, int L);

#endif