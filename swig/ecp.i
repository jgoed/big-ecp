%module ecp
%{
#include "../include/ecp/ecp.hpp"
%}

%include <std_string.i>
%include typemaps.i

std::string create_index(std::string dataset_file_path, int L, int desired_cluster_size);
std::string assign_points_to_cluster(std::string dataset_file_path, std::string index_file_path, int chunk_size);
extern uint32_t global_num_dimensions;
extern uint32_t global_point_size;