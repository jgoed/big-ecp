#ifndef DISTANCE_HPP
#define DISTANCE_HPP

#include <bits/stdc++.h>
#include "datastructure.hpp"

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
        EUCLIDEAN_OPT_UNROLL = 0,
        ANGULAR,
        EUCLIDEAN_HALT_OPT_UNROLL
    };

    /**
     * Set the globally used distance function.
     * @param Metric defines what functions will be used.
     */
    void set_distance_function(Metric);

}

#endif