#include "datastructures.hpp"
#include "distance.hpp"

namespace distance
{

    /// Definition of global distance function, extern in header
    float (*g_distance_function)(const DATATYPE *, const DATATYPE *, const float &);

    inline float euclidean_distance(const DATATYPE *a, const DATATYPE *b, const float &threshold = -1)
    {
#ifndef MULTI_THREADING
        globals::DIST_CALCULATIONS++;
#endif
        float sums[] = {0.0, 0.0, 0.0, 0.0};
        for (unsigned int i = 0; i < DIMENSIONS; ++i)
        {
            float delta = a[i] - b[i];
            sums[i % 4] += delta * delta;
        }

        return sums[0] + sums[1] + sums[2] + sums[3];
    }

    inline float euclidean_distance_halt(const DATATYPE *a, const DATATYPE *b, const float &threshold)
    {
#ifndef MULTI_THREADING
        globals::DIST_CALCULATIONS++;
#endif
        float sum = 0;
        for (unsigned int i = 0; i < DIMENSIONS; i++)
        {
            sum += (a[i] - b[i]) * (a[i] - b[i]);

            if (sum > threshold)
            {
                return globals::FLOAT_MAX;
            }
        }
        return sum;
    }

    inline float euclidean_distance_unroll(const DATATYPE *a, const DATATYPE *b, const float &threshold = -1)
    {
#ifndef MULTI_THREADING
        globals::DIST_CALCULATIONS++;
#endif
        float sums[] = {0.0, 0.0, 0.0, 0.0};
        for (unsigned int i = 0; i < DIMENSIONS; i = i + 4)
        {
            sums[0] += (a[i] - b[i]) * (a[i] - b[i]);
            sums[1] += (a[i + 1] - b[i + 1]) * (a[i + 1] - b[i + 1]);
            sums[2] += (a[i + 2] - b[i + 2]) * (a[i + 2] - b[i + 2]);
            sums[3] += (a[i + 3] - b[i + 3]) * (a[i + 3] - b[i + 3]);
        }

        return sums[0] + sums[1] + sums[2] + sums[3];
    }

    inline float euclidean_distance_halt_unroll(const DATATYPE *a, const DATATYPE *b, const float &threshold)
    {
#ifndef MULTI_THREADING
        globals::DIST_CALCULATIONS++;
#endif
        float sum = 0;
        for (unsigned int i = 0; i < DIMENSIONS; i = i + 4)
        {
            sum += ((a[i] - b[i]) * (a[i] - b[i])) + ((a[i + 1] - b[i + 1]) * (a[i + 1] - b[i + 1])) +
                   ((a[i + 2] - b[i + 2]) * (a[i + 2] - b[i + 2])) + ((a[i + 3] - b[i + 3]) * (a[i + 3] - b[i + 3]));
            if (sum > threshold)
            {
                return globals::FLOAT_MAX;
            }
        }
        return sum;
    }

    void set_distance_function(Metric metric)
    {
        auto is_dimensionality_divisable_by_4 = ((DIMENSIONS % 4) == 0);
        if (!is_dimensionality_divisable_by_4)
        {
            throw std::invalid_argument("Unrolling is not supported for this dimensionality!");
        }

        std::cout << "ECP: METRIC IS ";
        switch (metric)
        {
        case Metric::EUCLIDEAN:
            std::cout << "euclidean_distance" << std::endl;
            g_distance_function = &euclidean_distance;
            break;
        case Metric::EUCLIDEAN_HALT:
            std::cout << "euclidean_distance_halt" << std::endl;
            g_distance_function = &euclidean_distance_halt;
            break;
        case Metric::EUCLIDEAN_UNROLL:
            std::cout << "euclidean_distance_unroll" << std::endl;
            g_distance_function = &euclidean_distance_unroll;
            break;
        case Metric::EUCLIDEAN_HALT_UNROLL:
            std::cout << "euclidean_distance_halt_unroll" << std::endl;
            g_distance_function = &euclidean_distance_halt_unroll;
            break;
        default:
            throw std::invalid_argument("Invalid metric.");
        }
    }

}