#ifndef DATASTRUCTURE_HPP
#define DATASTRUCTURE_HPP

#include <bits/stdc++.h>
#include <fstream>
#include <vector>

extern std::fstream dataset;
extern std::fstream ecp_index;
extern uint32_t num_dimensions;
extern uint32_t chunk_size;

struct Point
{
    uint32_t id;
    std::vector<int8_t> descriptors;
};

struct Node
{
    uint32_t id;
    std::vector<Node> children;
    std::vector<Point> points;
};

#endif