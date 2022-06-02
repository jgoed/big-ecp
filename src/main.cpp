#include <fstream>
#include <iostream>
#include <math.h>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

struct
{
    uint32_t n;  // Numer of data points
    uint32_t ds; // Descriptor size
} dm;            // Dataset metadata

struct point
{
    int cluster_id;
    int id;
    vector<int8_t> descriptors;
};

struct node
{
    std::vector<node> children;
    std::vector<point> points;
};

float euclidean_distance(const float *a, const float *b, const float &threshold = -1)
{
    float sums[] = {0.0, 0.0, 0.0, 0.0};
    for (unsigned int i = 0; i < dm.ds; ++i)
    {
        float delta = a[i] - b[i];
        sums[i % 4] += delta * delta;
    }

    return sums[0] + sums[1] + sums[2] + sums[3];
}

int main()
{
    // Open dataset
    fstream dataset("spacev1b_base.i8bin", ios::in | ios::binary);
    if (!dataset)
    {
        cout << "Cannot open file!" << endl;
        return 1;
    }

    // Read metadata

    dataset.read((char *)&dm.n, sizeof(uint32_t));
    dataset.read((char *)&dm.ds, sizeof(uint32_t));
    int desired_cluster_size = 512000;                   // 512000 byte / (1 byte * descriptor_size)
    int l = ceil(dm.n / (desired_cluster_size / dm.ds)); // number of leaders

    vector<point> leaders;
    for (int i = 0; i < l; i++)
    {

        vector<int8_t> vector_point(dm.ds);
        dataset.read(reinterpret_cast<char *>(vector_point.data()), sizeof(int8_t) * dm.ds);

        point current_point;
        current_point.id = i;
        current_point.descriptors = vector_point;
        leaders.push_back(current_point);
    }

    return 0;
}