#include "datastructure.hpp"

namespace globals
{
    const float FLOAT_MAX = std::numeric_limits<float>::max();
    const float FLOAT_MIN = std::numeric_limits<float>::min();
    std::atomic<uint32_t> DIST_CALCULATIONS;
    uint32_t NUM_DIMENSIONS;
}
