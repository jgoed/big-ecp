#ifndef DATASTRUCTURE_HPP
#define DATASTRUCTURE_HPP

#include <bits/stdc++.h>
#include <vector>

extern uint32_t global_num_dimensions;
extern uint32_t global_point_size;

struct Point
{
    uint32_t id;
    int8_t descriptors[100];
};

struct Node
{
    uint32_t id;
    Point leader;
    std::vector<Node> children;
};

struct Binary_point
{
    int8_t descriptors[100];
};

struct Point_meta
{
    uint32_t buffer_position;
    uint32_t point_id;
    uint32_t cluster_id;
};

struct Cluster_meta
{
    uint32_t cluster_id;
    uint32_t num_points_in_leaf;
    uint32_t offset;
};

struct Cluster_point
{
    uint32_t cluster_id;
    uint32_t point_id;
    int8_t descriptor[100];
};

#endif