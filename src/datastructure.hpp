#ifndef DATASTRUCTURE_HPP
#define DATASTRUCTURE_HPP

#include <bits/stdc++.h>
#include <vector>

extern uint32_t global_num_dimensions;
extern uint32_t global_point_size;

struct Point
{
    unsigned int id;
    int8_t descriptors[100];
};

struct Node
{
    unsigned int id;
    Point leader;
    std::vector<Node> children;
};

struct Binary_point
{
    int8_t descriptors[100];
};

struct Point_meta
{
    unsigned int buffer_position;
    unsigned int point_id;
    unsigned int cluster_id;
};

struct Cluster_meta
{
    unsigned int cluster_id;
    unsigned int num_points_in_leaf;
    unsigned int offset;
};

struct Cluster_point
{
    unsigned int cluster_id;
    unsigned int point_id;
    int8_t descriptor[100];
};

#endif