#include <fstream>
#include <iostream>
#include <math.h>
#include <random>
#include <string>
#include <sstream>
#include <vector>
#include <bits/stdc++.h>

using namespace std;

fstream dataset;
ofstream index_file;
ifstream index_file_read;

uint32_t already_read = 0;

uint32_t num_points;
uint32_t num_dimensions;
uint32_t num_leaders;

uint32_t unique_id = 0;
uint32_t num_top_lvl_leaders;

uint32_t num_nodes_in_index = 0;

struct Point
{
    uint32_t id;
    vector<int8_t> descriptors;
};

struct Node
{
    uint32_t id;
    vector<Node> children;
    vector<Point> points;
};

vector<uint32_t> get_random_unique_indexes(uint32_t amount, uint32_t max_index)
{
    vector<uint32_t> collected_samples;
    collected_samples.reserve(amount);
    unordered_set<uint32_t> visited_samples;
    random_device random_seed;
    mt19937 generator(random_seed());
    for (int i = 0; i < amount; i++)
    {
        uniform_int_distribution<uint32_t> distribution(0, max_index); // To-from inclusive
        uint32_t cur_rnd_num = distribution(generator);

        unordered_set<uint32_t>::const_iterator iter = visited_samples.find(cur_rnd_num);
        if (iter == visited_samples.end()) // Not found
        {
            visited_samples.insert(cur_rnd_num);
            collected_samples.emplace_back(cur_rnd_num);
        }
        else // Found
        {
            visited_samples.insert(i);
            collected_samples.emplace_back(i);
        }
    }
    return collected_samples;
}

Point read_point_from_binary(uint32_t index)
{
    vector<int8_t> descriptors(num_dimensions);
    dataset.seekg((sizeof(num_points) + sizeof(num_dimensions) + index * num_dimensions * sizeof(int8_t)), dataset.beg);
    dataset.read(reinterpret_cast<char *>(descriptors.data()), sizeof(int8_t) * num_dimensions);
    Point point;
    point.id = index;
    point.descriptors = descriptors;
    return point;
}

Node read_node_from_binary(uint32_t index)
{
    Node node;
    node.id = unique_id;
    unique_id++;
    node.points.push_back(read_point_from_binary(index));
    return node;
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

Node *get_closest_node_from_uncomplete_tree(vector<Node> &nodes, int8_t *query, int deepth)
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
        closest = get_closest_node_from_uncomplete_tree(closest->children, query, deepth - 1);
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

Node *find_nearest_leaf(int8_t *query, std::vector<Node> &nodes)
{
    Node *closest_cluster = get_closest_node(nodes, query);
    if (!closest_cluster->children.empty())
    {
        return find_nearest_leaf(query, closest_cluster->children);
    }
    return closest_cluster;
}

bool is_leaf(Node &node)
{
    return node.children.empty();
}

void print_index_levels(vector<Node> &root)
{
    queue<Node> q;
    for (auto &cluster : root)
    {
        q.push(cluster);
    }
    while (!q.empty())
    {
        int n = q.size();
        while (n > 0)
        {
            Node node = q.front();
            q.pop();
            if (is_leaf(node))
            {
                cout << " [L: " << node.points.size() << "] ";
            }
            else
                cout << " [N: " << node.children.size() << "] ";
            for (unsigned int i = 0; i < node.children.size(); i++)
                q.push(node.children[i]);
            n--;
        }
        cout << "\n--------------\n";
    }
}

void write_node_to_binary(Node node)
{
    uint32_t cur_node_id = node.id;
    uint32_t cur_point_id = node.points[0].id;
    vector<int8_t> cur_descriptors = node.points.at(0).descriptors;
    // uint32_t cur_num_children = node.children.size();
    // vector<Node> cur_children = node.children;
    index_file.write(reinterpret_cast<char *>(&cur_node_id), sizeof(uint32_t));
    index_file.write(reinterpret_cast<char *>(&cur_point_id), sizeof(uint32_t));
    index_file.write(reinterpret_cast<char *>(cur_descriptors.data()), sizeof(int8_t) * node.points[0].descriptors.size());
    // index_file.write(reinterpret_cast<char *>(&cur_num_children), sizeof(cur_num_children));
    num_nodes_in_index++;
}

void save_node(Node node)
{
    num_nodes_in_index++;
    index_file.write(reinterpret_cast<char *>(&node.id), sizeof(uint32_t));
    index_file.write(reinterpret_cast<char *>(&node.points.at(0).id), sizeof(uint32_t));
    index_file.write(reinterpret_cast<char *>(node.points.at(0).descriptors.data()), sizeof(int8_t) * num_dimensions);
    uint32_t cur_num_children = node.children.size();
    index_file.write(reinterpret_cast<char *>(&cur_num_children), sizeof(uint32_t));
    for (int i = 0; i < node.children.size(); i++)
    {
        save_node(node.children[i]);
    }
}

Node load_node()
{
    already_read++;
    Node node;

    uint32_t read_node_id;
    uint32_t read_point_id;
    vector<int8_t> read_descriptors(num_dimensions);
    uint32_t read_num_children;

    index_file_read.read(reinterpret_cast<char *>(&read_node_id), sizeof(uint32_t));
    index_file_read.read(reinterpret_cast<char *>(&read_point_id), sizeof(uint32_t));
    index_file_read.read(reinterpret_cast<char *>(read_descriptors.data()), sizeof(int8_t) * num_dimensions);
    index_file_read.read(reinterpret_cast<char *>(&read_num_children), sizeof(uint32_t));
    node.id = read_node_id;
    Point point;
    point.id = read_point_id;
    point.descriptors = read_descriptors;
    node.points.push_back(point);
    for (int i = 0; i < read_num_children; i++)
    {
        node.children.push_back(load_node());
    }

    return node;
}

int main()
{
    int L = 3;

    // Open binary file
    dataset.open("spacev1b_base.i8bin", ios::in | ios::binary);

    // Read metadata from binary file
    dataset.read((char *)&num_points, sizeof(uint32_t));
    dataset.read((char *)&num_dimensions, sizeof(uint32_t));

    // Calculate the overall number of leaders
    int desired_cluster_size = 512000;                                                                              // 512000 byte is default block size for SSDs
    num_leaders = ceil(num_points / (desired_cluster_size / (sizeof(int8_t) * num_dimensions + sizeof(uint32_t)))); // N/ts

    // Generate random leaders
    // random_leader_indexes.at(i) vector<uint32_t> random_leader_indexes = get_random_unique_indexes(num_leaders, num_points - 1);

    // Tree index structure
    vector<Node> tree;

    for (int cur_lvl = 1; cur_lvl <= L; cur_lvl++)
    {
        // Calculate number of leaders for current level
        int cur_lvl_leaders = ceil(pow(num_leaders, ((1.0 / L)) * (cur_lvl)));

        if (cur_lvl == 1) // If current level is top level
        {
            num_top_lvl_leaders = cur_lvl_leaders;
            vector<Node> current_level;
            for (int i = 0; i < cur_lvl_leaders; i++)
            {
                current_level.push_back(read_node_from_binary(i));
            }
            tree.swap(current_level);
        }
        else // Every other level below top level
        {
            for (int i = 0; i < cur_lvl_leaders; i++)
            {
                Node current_node = read_node_from_binary(i);
                Node *clostest_node = get_closest_node_from_uncomplete_tree(tree, &current_node.points[0].descriptors[0], cur_lvl - 1);
                clostest_node->children.push_back(current_node);
            }
        }
    }

    // Add all points from input dataset to the index incl those duplicated in the index construction.
    for (uint32_t cur_index = 0; cur_index < num_points; cur_index++)
    {
        Point cur_point = read_point_from_binary(cur_index);
        auto *leaf = find_nearest_leaf(cur_point.descriptors.data(), tree);
        if (cur_index != leaf->points[0].id) // Prevent adding leader twice
        {
            leaf->points.emplace_back(cur_point);
        }
    }

    // Write index in binary file
    index_file.open("index.i8bin", ios::out | ios::binary);
    index_file.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));

    ///////////////////////////////////////////////TEST

    for (auto &node : tree)
    {
        save_node(node);
    }

    index_file.seekp(0, index_file.beg);
    index_file.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));
    index_file.close();

    index_file_read.open("index.i8bin", ios::in | ios::binary);
    uint32_t num_nodes_to_read;
    index_file_read.read(reinterpret_cast<char *>(&num_nodes_to_read), sizeof(uint32_t));

    vector<Node> read_tree;
    for (already_read; already_read < num_nodes_to_read;)
    {
        read_tree.push_back(load_node());
    }

    cout << endl;

    // uint32_t num_nodes_to_read;
    // index_file_read.read(reinterpret_cast<char *>(&num_nodes_to_read), sizeof(uint32_t));
    // vector<Node> index_nodes_read;
    // for (int i = 0; i < num_nodes_to_read; i++)
    // {
    //     uint32_t read_node_id;
    //     uint32_t read_point_id;
    //     vector<int8_t> read_vec(num_dimensions);
    //     index_file_read.read(reinterpret_cast<char *>(&read_node_id), sizeof(uint32_t));
    //     index_file_read.read(reinterpret_cast<char *>(&read_point_id), sizeof(uint32_t));
    //     index_file_read.read(reinterpret_cast<char *>(read_vec.data()), sizeof(int8_t) * num_dimensions);
    //     Node read_cur_node;
    //     read_cur_node.id = read_node_id;
    //     Point read_cur_point;
    //     read_cur_point.id = read_point_id;
    //     read_cur_point.descriptors = read_vec;
    //     read_cur_node.points.push_back(read_cur_point);
    //     index_nodes_read.push_back(read_cur_node);
    //     map_nodes[read_cur_node.id] = read_cur_node;
    // }

    ////////////////////////////////////////////////TEST

    // vector<tuple<uint32_t, uint32_t>> edges;

    // queue<Node> q;
    // for (auto &node : tree)
    // {
    //     q.push(node);
    // }
    // while (!q.empty())
    // {
    //     int n = q.size();
    //     while (n > 0)
    //     {
    //         Node node = q.front();
    //         q.pop();
    //         write_node_to_binary(node);
    //         if (!is_leaf(node))
    //         {
    //             for (unsigned int i = 0; i < node.children.size(); i++)
    //             {
    //                 q.push(node.children[i]);
    //                 edges.push_back({node.id, node.children[i].id});
    //             }
    //         }
    //         n--;
    //     }
    // }

    // uint32_t num_edges = edges.size();
    // index_file.write(reinterpret_cast<char *>(&num_edges), sizeof(uint32_t));
    // for (int i = 0; i < num_edges; i++)
    // {
    //     tuple<uint32_t, uint32_t> cur_edge = edges[i];
    //     index_file.write(reinterpret_cast<char *>(&cur_edge), sizeof(tuple<uint32_t, uint32_t>));
    //     // cout << "(" << get<0>(cur_edge) << ", " << get<1>(cur_edge) << ")" << endl;
    // }

    // index_file.seekp(0, index_file.beg);
    // index_file.write(reinterpret_cast<char *>(&num_nodes_in_index), sizeof(uint32_t));

    // index_file.close();

    // // Read index from binary file

    // map<uint32_t, Node> map_nodes;

    // ifstream index_file_read;
    // index_file_read.open("index.i8bin", ios::in | ios::binary);
    // uint32_t num_nodes_to_read;
    // index_file_read.read(reinterpret_cast<char *>(&num_nodes_to_read), sizeof(uint32_t));
    // vector<Node> index_nodes_read;
    // for (int i = 0; i < num_nodes_to_read; i++)
    // {
    //     uint32_t read_node_id;
    //     uint32_t read_point_id;
    //     vector<int8_t> read_vec(num_dimensions);
    //     index_file_read.read(reinterpret_cast<char *>(&read_node_id), sizeof(uint32_t));
    //     index_file_read.read(reinterpret_cast<char *>(&read_point_id), sizeof(uint32_t));
    //     index_file_read.read(reinterpret_cast<char *>(read_vec.data()), sizeof(int8_t) * num_dimensions);
    //     Node read_cur_node;
    //     read_cur_node.id = read_node_id;
    //     Point read_cur_point;
    //     read_cur_point.id = read_point_id;
    //     read_cur_point.descriptors = read_vec;
    //     read_cur_node.points.push_back(read_cur_point);
    //     index_nodes_read.push_back(read_cur_node);
    //     map_nodes[read_cur_node.id] = read_cur_node;
    // }

    // cout << map_nodes.find(1)->second.id << endl;

    // vector<tuple<uint32_t, uint32_t>> read_edges;
    // uint32_t read_num_edges;
    // index_file_read.read(reinterpret_cast<char *>(&read_num_edges), sizeof(uint32_t));
    // for (int i = 0; i < read_num_edges; i++)
    // {
    //     tuple<uint32_t, uint32_t> read_cur_edge;
    //     index_file_read.read(reinterpret_cast<char *>(&read_cur_edge), sizeof(tuple<uint32_t, uint32_t>));
    //     read_edges.push_back(read_cur_edge);
    // }

    // vector<Node> read_tree;

    // // TODO: Save num_top_lvl_leaders in binary
    // for (int i = 0; i < num_top_lvl_leaders; i++)
    // {
    //     read_tree.push_back(index_nodes_read[i]);
    // }

    // for (int i = 0; i < read_num_edges; i++)
    // {
    //     cout << "(" << get<0>(read_edges[i]) << ", " << get<1>(read_edges[i]) << ")" << endl;
    // }

    // // index_file_read.seekg(0, index_file_read.beg);

    // // print_index_levels(tree);

    return 0;
}