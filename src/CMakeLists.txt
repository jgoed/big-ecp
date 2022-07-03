# file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${ECP_SOURCE_DIR}/src/ecp/*.cpp")
# file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${ECP_SOURCE_DIR}/src/ecp/*.hpp")

# add_library(ecpLib )

# target_include_directories(ecpLib
#     PUBLIC
#         ../include/
#         ./
# )

# target_link_libraries(ecpLib)


# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
# file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${ModernCMakeExample_SOURCE_DIR}/include/modern/*.hpp")
set(HEADER_LIST "${ecp_SOURCE_DIR}/include/ecp/ecp.hpp")

# Make an automatic library - will be static or dynamic based on user setting
add_library(ecp ecp.cpp ${HEADER_LIST} datastructure.cpp distance.cpp index.cpp assignment.cpp query.cpp)

# We need this directory, and users of our library will need it too
target_include_directories(ecp PUBLIC ../include)