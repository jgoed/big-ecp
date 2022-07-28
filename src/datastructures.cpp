#include "datastructures.hpp"

namespace globals // Global variables
{
    const float FLOAT_MAX = std::numeric_limits<float>::max(); // Max value a float can hold, needed to initalice threshold for distance computation
    uint32_t DIST_CALCULATIONS = 0;                            // Counter for number of distance computation, can only be used with single core, because atomic overhead for multithreading slows everything down
}
