cmake_minimum_required (VERSION 3.1)
project (ebusify)
set (CMAKE_CXX_STANDARD 11)

add_library(Greet SHARED 
    src/hello_world.cpp
)
set_target_properties(Greet
    PROPERTIES POSITION_INDEPENDENT_CODE ON
    PREFIX ""
)
target_compile_definitions(Greet PRIVATE
    BOOST_NO_AUTO_PTR
)

add_executable(Gurobi
    src/gurobi_test.cpp
)


add_library(UtilityManager SHARED
    src/utility_manager.cpp
    src/energy_source.cpp
    src/nuclear_plant.cpp
    src/biomass_plant.cpp
    src/solar_plant.cpp
    src/wind_plant.cpp
    src/coal_plant.cpp
    src/naturalgas_plant.cpp
    src/hydro_plant.cpp
)
set_target_properties(UtilityManager
    PROPERTIES POSITION_INDEPENDENT_CODE ON
    PREFIX ""
)
target_compile_definitions(UtilityManager PRIVATE
    BOOST_NO_AUTO_PTR
)

find_package(PythonLibs 2.7 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
target_link_libraries(Greet ${PYTHON_LIBRARIES})
target_link_libraries(UtilityManager ${PYTHON_LIBRARIES})

find_library(BP_LIB boost_python27 PATH /opt/boost_1_70_0/lib)
target_link_libraries(Greet "${BP_LIB}")
target_link_libraries(UtilityManager "${BP_LIB}")

find_library(BN_LIB boost_numpy27 PATH /opt/boost_1_70_0/lib)
target_link_libraries(Greet "${BN_LIB}")
target_link_libraries(UtilityManager "${BN_LIB}")

include_directories(/opt/boost_1_70_0/include)


find_library(GRBC_LIB gurobi_c++ PATH /opt/gurobi811/linux64/lib)
target_link_libraries(Gurobi "${GRBC_LIB}")
target_link_libraries(UtilityManager "${GRBC_LIB}")

find_library(GRB_LIB gurobi81 PATH /opt/gurobi811/linux64/lib)
target_link_libraries(Gurobi "${GRB_LIB}")
target_link_libraries(UtilityManager "${GRB_LIB}")

include_directories(/opt/gurobi811/linux64/include)
