#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <string>
#include <sstream>
#include <vector>
#include <bits/stdc++.h>

using namespace std;

uint32_t num_points;
uint32_t num_dimensions;
uint32_t num_leaders;

struct Point
{
    uint32_t id;
    vector<int8_t> descriptors;
};

struct Node
{
    vector<Node> children;
    vector<Point> points;
    Point *get_leader() { return &points[0]; }
};

std::vector<uint32_t> get_random_unique_indexes(int amount, uint32_t container_size)
{
    std::vector<uint32_t> collected_samples;
    collected_samples.reserve(amount);

    std::unordered_set<uint32_t> visited_samples;
    std::random_device random_seed;        // Will be used to obtain a seed for the random number engine.
    std::mt19937 generator(random_seed()); // Standard mersenne_twister_engine seeded with rd().
    int start = container_size - amount;

    for (int j = start; j < container_size; ++j)
    {
        std::uniform_int_distribution<uint32_t> distribution(0, j); // To-from inclusive.
        uint32_t t = distribution(generator);

        std::unordered_set<uint32_t>::const_iterator iter = visited_samples.find(t);
        if (iter == visited_samples.end())
        { // Not found.
            visited_samples.insert(t);
            collected_samples.emplace_back(t);
        }
        else
        {
            visited_samples.insert(j); // Found.
            collected_samples.emplace_back(j);
        }
    }
    return collected_samples;
}

vector<vector<unsigned>> generate_leaders_indexes(size_t num_points, unsigned L)
{
    // Indexes picked randomly from input dataset used as leaders for each level
    vector<vector<unsigned>> random_leader_indexes(L);

    // Computing random indexes top-down, leader_indexes[0] is top level.
    // unsigned container_size = num_points;
    for (unsigned cur_lvl = 1; cur_lvl <= L; cur_lvl++)
    {
        // Calculate level sizes (i.e. how many clusters for level L)
        int leaders_per_lvl = ceil(pow(num_leaders, ((1.0 / L)) * cur_lvl));
        // unsigned level_size = ceil(pow(num_points, (i / (L + 1.00))));

        // Pick random leaders for current level
        random_leader_indexes[cur_lvl - 1].reserve(leaders_per_lvl);
        // FIXME: Time this to see if RVO or move/copy ctors are being utilized
        random_leader_indexes[cur_lvl - 1] = get_random_unique_indexes(leaders_per_lvl, num_points);

        // Set to the size of current level, because indexes are found from the level below
        // container_size = level_size;
    }

    return random_leader_indexes;
}

inline float euclidean_distance(const int8_t *a, const int8_t *b, const float &threshold = -1)
{
    float sums[] = {0.0, 0.0, 0.0, 0.0};
    for (unsigned int i = 0; i < num_dimensions; ++i)
    {
        float delta = a[i] - b[i];
        sums[i % 4] += delta * delta;
    }

    return sums[0] + sums[1] + sums[2] + sums[3];
}

// Node *get_closest_node(std::vector<Node> &nodes, const float *query)
// {
//     float max = numeric_limits<float>::max();
//     Node *closest = nullptr;

//     for (Node &node : nodes)
//     {
//         const float distance = euclidean_distance(query, node.get_leader()->descriptors, max);

//         if (distance < max)
//         {
//             max = distance;
//             closest = &node;
//         }
//     }
//     return closest;
// }

int main()
{
    int L = 3;

    // Open dataset
    fstream dataset("spacev1b_base.i8bin", ios::in | ios::binary);
    if (!dataset)
    {
        cout << "Cannot open file!" << endl;
        return 1;
    }

    // Read metadata

    dataset.read((char *)&num_points, sizeof(uint32_t));
    dataset.read((char *)&num_dimensions, sizeof(uint32_t));

    // Calculate num_leaders
    int desired_cluster_size = 512000; // 512000 byte is default block size for SSDs
    num_leaders = ceil(num_points / (desired_cluster_size / (num_dimensions + sizeof(uint32_t))));

    // Generate random leaders
    const auto random_leader_indexes = generate_leaders_indexes(num_points, L);

    vector<Node> upper_level;

    for (auto it = random_leader_indexes.begin(); it != random_leader_indexes.end(); it++)
    {
        vector<Node> current_level;

        // For top level
        if (upper_level.size() == 0)
        {

            for (auto index : *it)
            {
                // Read descriptors at index
                vector<int8_t> current_descriptors(num_dimensions);
                dataset.seekg((sizeof(num_points) + sizeof(num_dimensions) + index * num_dimensions * sizeof(int8_t)), dataset.beg);
                dataset.read(reinterpret_cast<char *>(current_descriptors.data()), sizeof(int8_t) * num_dimensions);

                // Create point with descriptors
                Point current_point;
                current_point.id = index;
                current_point.descriptors = current_descriptors;

                // Create node with point
                Node current_node;
                current_node.points.push_back(current_point);
                current_level.push_back(current_node);
            }
        }
        // For every level below the top level
        else
        {
            for (auto index : *it)
            {
                // Read descriptors at index
                vector<int8_t> current_descriptors(num_dimensions);
                dataset.seekg((sizeof(num_points) + sizeof(num_dimensions) + index * num_dimensions * sizeof(int8_t)), dataset.beg);
                dataset.read(reinterpret_cast<char *>(current_descriptors.data()), sizeof(int8_t) * num_dimensions);

                // Create point with descriptors
                Point current_point;
                current_point.id = index;
                current_point.descriptors = current_descriptors;

                // Create node with point
                Node current_node;
                current_node.points.push_back(current_point);

                // Pick previously randomly found nodes from level below to construct current level
                auto *node = &upper_level[index];
                // Reconstruct Node to not copy children/points into current level
                //current_level.emplace_back(Node{Point{*node->get_leader()}});
            }

            // Add all nodes from below level as children of current level
            for (auto node : upper_level)
            {
                //pre_processing_helpers::get_closest_node(current_level, node.get_leader()->descriptor)
                //    ->children.emplace_back(std::move(node));
            }
        }

        upper_level.swap(current_level);
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------
    // Generate leader_indices
    vector<uint32_t> leader_indices = get_random_unique_indexes(num_leaders, num_points);

    // Read actual leader data points
    vector<Point> leaders;
    for (uint32_t i : leader_indices)
    {
        vector<int8_t> data_point(num_dimensions);
        dataset.seekg((sizeof(num_points) + sizeof(num_dimensions) + i * num_dimensions * sizeof(int8_t)), dataset.beg);
        dataset.read(reinterpret_cast<char *>(data_point.data()), sizeof(int8_t) * num_dimensions);

        Point current_point;
        current_point.id = i;
        current_point.descriptors = data_point;
        leaders.push_back(current_point);
    }

    Node index;

    // Calculate number of leaders for each level of index tree
    for (int cur_lvl = 1; cur_lvl <= L; cur_lvl++)
    {
        int leaders_per_lvl = ceil(pow(num_leaders, ((1.0 / L)) * cur_lvl));

        if (cur_lvl == 1)
        {
            vector<Node> top_lvl;
            for (int i = 0; i < leaders_per_lvl; i++)
            {
                Node cur_node;
                cur_node.points.push_back(leaders[i]);
                top_lvl.push_back(cur_node);
            }
            index.children = top_lvl;
        }
        else
        {
            for (int i = 0; i < leaders_per_lvl; i++)
            {
                Node cur_node;
                cur_node.points.push_back(leaders[i]);
                for (Node n : index.children)
                {
                }
            }
        };
    }

    return 0;
}