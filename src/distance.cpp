#include <ecp/datastructure.hpp>
#include <ecp/distance.hpp>

/**
 * Calculate euclidean distance for two points in n dimensional space
 * @param a Point A
 * @param b Point B
 */
float euclidean_distance(const int8_t *a, const int8_t *b)
{
    float sums[] = {0.0, 0.0, 0.0, 0.0};
    for (unsigned int i = 0; i < global_num_dimensions; ++i)
    {
        float delta = a[i] - b[i];
        sums[i % 4] += delta * delta; // Skip square_root because exact distance is not needed
    }
    return sums[0] + sums[1] + sums[2] + sums[3];
}