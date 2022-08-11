#ifndef DATASTRUCTURE_HPP
#define DATASTRUCTURE_HPP

#include <bits/stdc++.h>
#include <string>
#include <vector>

#define DATATYPE uint8_t                                  // Datatype of points in dataset
#define DIMENSIONS 128                                    // Number of dimensions for each point in dataset
#define ECP_INDEX_FILE_NAME "ecp_index.bin"               // File name for index binary file
#define ECP_CLUSTERS_FILE_NAME "ecp_clusters.bin"         // File name for clusters binary file
#define ECP_CLUSTER_META_FILE_NAME "ecp_cluster_meta.bin" // File name for cluster meta data binary file
#define ECP_LOG_FILE_NAME "ecp_log_file.txt"              // File name for log file
#define MULTI_THREADING                                   // Uncomment to use multihreading
#define RANDOM_LEADER_IDS                                 // Uncomment to use random ids to build the index, using non random ids makes debugging easier

namespace globals // Global variables
{
    extern const float FLOAT_MAX;      // Max value a float can hold, needed to initalice threshold for distance computation
    extern uint32_t DIST_CALCULATIONS; // Counter for number of distance computation, can only be used with single core, because atomic overhead for multithreading slows everything down
}

struct Point // Data point from dataset in index tree structure
{
    uint32_t point_id;                // Unique point id, uint32_t enables only use of datasets up to 4B points
    DATATYPE descriptors[DIMENSIONS]; // Array of descriptors
};

struct Node // Node in index tree structure
{
    uint32_t node_id;           // Unique node id
    Point leader;               // Representative point for node
    std::vector<Node> children; // Children of node
};

struct BinaryPoint // Struct to extract points from binary file more easy
{
    DATATYPE descriptors[DIMENSIONS]; // Array of descriptors
};

struct PointMeta // Meta data for a point
{
    uint32_t buffer_position; // Start position of point in buffer
    uint32_t point_id;        // Unique point id, uint32_t enables only use of datasets up to 4B points
    uint32_t cluster_id;      // Assigned cluster id
};

struct ClusterMeta // meta data for all clusters aka leafs in the clusters binary file
{
    uint32_t cluster_id;         // Unique cluster id
    uint32_t num_points_in_leaf; // Number of points assigned to this leaf
    uint32_t offset;             // Offset there cluster starts from beginning of the file
};

struct ClusterPoint // Format of assigned points stored in clusters binary file
{
    uint32_t cluster_id;             // Unique cluster id
    uint32_t point_id;               // Unique point id, uint32_t enables only use of datasets up to 4B points
    DATATYPE descriptor[DIMENSIONS]; // Array of descriptors
};

struct QueryIndex // Struct to pack necessary information needed by query process
{
    std::vector<Node> top_level;        // In memory index tree structure
    std::vector<ClusterMeta> meta_data; // meta data of clusters file
    std::string clusters_file_path;     // Path to clusters file
};

#endif