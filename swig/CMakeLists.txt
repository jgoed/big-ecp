include(UseSWIG)

# Check and include Python libraries
find_package(PythonLibs)
include_directories(${PYTHON_INCLUDE_PATH})

set_property(SOURCE ecp.i PROPERTY CPLUSPLUS ON "-builtin" "-extranative" "-Wall")
set_property(SOURCE ecp.i PROPERTY SWIG_MODULE_NAME ecp_wrapper)

swig_add_library(ecp_wrapper
  TYPE SHARED
  LANGUAGE python
  SOURCES ecp.i
)

target_include_directories(ecp_wrapper
  PUBLIC
    ../include/
)

swig_link_libraries(ecp_wrapper
  PUBLIC
    ${PYTHON_LIBRARIES}
    ecp
)