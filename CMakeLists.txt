cmake_minimum_required (VERSION 3.1)
project (ebusify)

set (CMAKE_CXX_STANDARD 11)
add_library(Greet SHARED src/hello_world.cpp)
set_target_properties(Greet
    PROPERTIES POSITION_INDEPENDENT_CODE ON
    PREFIX ""
)
target_compile_definitions(Greet PRIVATE BOOST_ALL_NO_LIB BOOST_NO_AUTO_PTR)

find_package(PythonLibs 2.7 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
target_link_libraries(Greet ${PYTHON_LIBRARIES})

find_library(BP_LIB boost_python27 PATH /opt/boost_1_70_0/lib)
target_link_libraries(Greet "${BP_LIB}")

find_library(BN_LIB boost_numpy27 PATH /opt/boost_1_70_0/lib)
target_link_libraries(Greet "${BN_LIB}")

include_directories(/opt/boost_1_70_0/include)
