#ifndef DISTANCE_HPP
#define DISTANCE_HPP

#include <bits/stdc++.h>
#include "datastructures.hpp"

/**
 * Namespaces contains distance functions used in the index.
 */
namespace distance
{

    /**
     * External linkage. Globally scoped pointer to the used distance function.
     */
    extern float (*g_distance_function)(const DATATYPE *, const DATATYPE *, const float &);

    /**
     * @brief The Metric enum is used to define globally the type of distance function used.
     */
    enum Metric
    {
        EUCLIDEAN = 0,
        EUCLIDEAN_HALT,
        EUCLIDEAN_UNROLL,
        EUCLIDEAN_HALT_UNROLL
    };

    /**
     * Set the globally used distance function.
     * @param Metric defines what functions will be used.
     */
    void set_distance_function(Metric);

}

#endif