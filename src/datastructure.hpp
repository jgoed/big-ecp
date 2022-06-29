#ifndef DATASTRUCTURE_HPP
#define DATASTRUCTURE_HPP

#include <bits/stdc++.h>
#include <vector>

extern uint32_t num_dimensions;

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