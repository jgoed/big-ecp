#include "datastructure.hpp"

std::fstream dataset;
std::fstream ecp_index;
uint32_t num_dimensions = 0;
uint32_t chunk_size = 125000; // Number of data points per chunk