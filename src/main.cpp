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
uint32_t test_count = 0;

struct Point
{
    uint32_t id;
    vector<int8_t> descriptors;
};

struct Node
{
    vector<Node> children;
    vector<Point> points;
};

vector<uint32_t> get_random_unique_indexes(int amount, uint32_t container_size)
{
    vector<uint32_t> collected_samples;
    collected_samples.reserve(amount);

    unordered_set<uint32_t> visited_samples;
    random_device random_seed;        // Will be used to obtain a seed for the random number engine.
    mt19937 generator(random_seed()); // Standard mersenne_twister_engine seeded with rd().
    int start = container_size - amount;

    for (int j = start; j < container_size; ++j)
    {
        uniform_int_distribution<uint32_t> distribution(0, j); // To-from inclusive.
        uint32_t t = distribution(generator);

        unordered_set<uint32_t>::const_iterator iter = visited_samples.find(t);
        if (iter == visited_samples.end()) // Not found.
        {
            visited_samples.insert(t);
            collected_samples.emplace_back(t);
        }
        else // Found
        {
            visited_samples.insert(j);
            collected_samples.emplace_back(j);
        }
    }
    return collected_samples;
}

float euclidean_distance(const int8_t *a, const int8_t *b)
{
    float sums[] = {0.0, 0.0, 0.0, 0.0};
    for (unsigned int i = 0; i < num_dimensions; ++i)
    {
        float delta = a[i] - b[i];
        sums[i % 4] += delta * delta;
    }

    return sums[0] + sums[1] + sums[2] + sums[3];
}

Node *get_closest_node_tree(vector<Node> &nodes, int8_t *query, int deepth)
{
    float max = numeric_limits<float>::max();
    Node *closest = nullptr;

    for (Node &node : nodes)
    {
        const float distance = euclidean_distance(query, &node.points[0].descriptors[0]);

        if (distance < max)
        {
            max = distance;
            closest = &node;
        }
    }

    if (deepth > 1)
    {
        closest = get_closest_node_tree(closest->children, query, deepth - 1);
    }

    return closest;
}

Node *get_closest_node(std::vector<Node> &nodes, int8_t *query)
{
    float max = numeric_limits<float>::max();
    Node *closest = nullptr;

    for (Node &node : nodes)
    {
        const float distance = euclidean_distance(query, &node.points[0].descriptors[0]);

        if (distance < max)
        {
            max = distance;
            closest = &node;
        }
    }
    return closest;
}

bool is_leaf(Node &node) { return node.children.empty(); }

void print_index_levels(std::vector<Node> &root)
{
    // Standard level order traversal code
    // using queue
    std::queue<Node> q; // Create a queue
                        // Enqueue top_level
    for (auto &cluster : root)
    {
        q.push(cluster);
    }

    while (!q.empty())
    {
        int n = q.size();

        // If this node has children
        while (n > 0)
        {
            // Dequeue an item from queue and print it
            Node node = q.front();
            q.pop();

            if (is_leaf(node))
            {
                std::cout << " [L: " << node.points.size() << " ]";
            }
            else
                std::cout << " [N: " << node.children.size() << "] ";

            // Enqueue all children of the dequeued item
            for (unsigned int i = 0; i < node.children.size(); i++)
                q.push(node.children[i]);
            n--;
        }

        std::cout << "\n"; // Print new line between two levels
    }
    std::cout << "--------------\n";
}

Node *find_nearest_leaf(int8_t *query, std::vector<Node> &nodes)
{
    Node *closest_cluster = get_closest_node(nodes, query);

    if (!closest_cluster->children.empty())
    {
        return find_nearest_leaf(query, closest_cluster->children);
    }

    return closest_cluster;
}

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

    // Read metadata from binary file
    dataset.read((char *)&num_points, sizeof(uint32_t));
    dataset.read((char *)&num_dimensions, sizeof(uint32_t));

    // Calculate the overall number of leaders
    int desired_cluster_size = 512000;                                                                              // 512000 byte is default block size for SSDs
    num_leaders = ceil(num_points / (desired_cluster_size / (sizeof(int8_t) * num_dimensions + sizeof(uint32_t)))); // N/ts

    // Generate random leaders
    // vector<uint32_t> random_leader_indexes = get_random_unique_indexes(num_leaders, num_points);

    vector<Node> tree;

    for (int cur_lvl = 1; cur_lvl <= L; cur_lvl++)
    {
        int leaders_per_lvl = ceil(pow(num_leaders, ((1.0 / L)) * (cur_lvl)));

        if (cur_lvl == 1)
        {
            vector<Node> current_level;

            for (int i = 0; i < leaders_per_lvl; i++)
            {
                // Read descriptors at index
                vector<int8_t> current_descriptors(num_dimensions);
                dataset.seekg((sizeof(num_points) + sizeof(num_dimensions) + i * num_dimensions * sizeof(int8_t)), dataset.beg);
                dataset.read(reinterpret_cast<char *>(current_descriptors.data()), sizeof(int8_t) * num_dimensions);

                // Create point with descriptors
                Point current_point;
                current_point.id = i;
                current_point.descriptors = current_descriptors;

                // Create node with point
                Node current_node;
                current_node.points.push_back(current_point);
                current_level.push_back(current_node);
            }
            tree.swap(current_level);
        }
        else
        {
            for (int i = 0; i < leaders_per_lvl; i++)
            {
                // Read descriptors at index
                vector<int8_t> current_descriptors(num_dimensions);
                dataset.seekg((sizeof(num_points) + sizeof(num_dimensions) + i * num_dimensions * sizeof(int8_t)), dataset.beg);
                dataset.read(reinterpret_cast<char *>(current_descriptors.data()), sizeof(int8_t) * num_dimensions);

                // Create point with descriptors
                Point current_point;
                current_point.id = i;
                current_point.descriptors = current_descriptors;

                // Create node with point
                Node current_node;
                current_node.points.push_back(current_point);

                Node *clostest_node = get_closest_node_tree(tree, &current_node.points[0].descriptors[0], cur_lvl - 1);
                clostest_node->children.push_back(current_node);
            }
        }
    }

    // Add all points from input dataset to the index incl those duplicated in the index construction.
    
    for (uint32_t id = 0; id < num_points; id++)
    {
        // Read descriptors at index
        vector<int8_t> current_descriptors(num_dimensions);
        dataset.seekg((sizeof(num_points) + sizeof(num_dimensions) + id * num_dimensions * sizeof(int8_t)), dataset.beg);
        dataset.read(reinterpret_cast<char *>(current_descriptors.data()), sizeof(int8_t) * num_dimensions);

        // Create point with descriptors
        Point current_point;
        current_point.id = id;
        current_point.descriptors = current_descriptors;

        auto *leaf = find_nearest_leaf(current_point.descriptors.data(), tree);
        // Because the leader was added to the cluster when the index was built
        if (id != leaf->points[0].id)
        {
            leaf->points.emplace_back(current_point);
        }
        test_count++;
    }

    cout << test_count << endl;

    print_index_levels(tree);

    cout << sizeof(tree) << endl;

    return 0;
}