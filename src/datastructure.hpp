#ifndef DATASTRUCTURE_HPP
#define DATASTRUCTURE_HPP

#include <bits/stdc++.h>
#include <string>
#include <vector>

#define DATATYPE int8_t
#define DIMENSIONS 100
#define ECP_INDEX_FILE_NAME "ecp_index.bin"
#define ECP_CLUSTERS_FILE_NAME "ecp_clusters.bin"
#define ECP_CLUSTER_META_FILE_NAME "ecp_cluster_meta.bin"

namespace globals
{
    extern const float FLOAT_MAX;
    extern const float FLOAT_MIN;
    extern uint32_t DIST_CALCULATIONS;
}

struct Point
{
    uint32_t id;
    DATATYPE descriptors[DIMENSIONS];
};

struct Node
{
    uint32_t id;
    Point leader;
    std::vector<Node> children;
};

struct BinaryPoint
{
    DATATYPE descriptors[DIMENSIONS];
};

struct PointMeta
{
    uint32_t buffer_position;
    uint32_t point_id;
    uint32_t cluster_id;
};

struct ClusterMeta
{
    uint32_t cluster_id;
    uint32_t num_points_in_leaf;
    uint32_t offset;
};

struct ClusterPoint
{
    uint32_t cluster_id;
    uint32_t point_id;
    DATATYPE descriptor[DIMENSIONS];
};

struct QueryIndex
{
    std::vector<Node> top_level;
    std::vector<ClusterMeta> meta_data;
    std::string clusters_file_path;
};

#endif