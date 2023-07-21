cmake_minimum_required(VERSION 3.15)
project(captain)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# 将当前目录添加到包含目录列表
include_directories(.)
# 将/usr/local/include添加到包含目录列表
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(LIB_SRC
    captain/log.cpp
    captain/util.cpp
    captain/config.cpp
    )

add_library(captain SHARED ${LIB_SRC})

#find_library(YAMLCPP yaml-cpp)
set(LIBS
        captain
        yaml-cpp)

#message("***", ${LIBS})

add_executable(test tests/test.cpp)
add_dependencies(test captain)
target_link_libraries(test ${LIBS})

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config captain)
target_link_libraries(test_config ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
