%module ecp_wrapper
%{
//#define SWIG_FILE_WITH_INIT
#include "../include/ecp/ecp.hpp"
%}

%include std_pair.i
%include std_string.i
%include std_vector.i
%include typemaps.i

namespace std {
  %template(UIntVector) std::vector<unsigned int>;
  %template(FloatFloatVector) std::vector<std::vector<float>>;
  %template(FloatPointerVector) std::vector<float*>;
}

%include "../include/ecp/ecp.hpp"