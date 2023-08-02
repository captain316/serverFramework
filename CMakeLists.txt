cmake_minimum_required(VERSION 3.15)
project(captain)

#用于强制重新定义文件宏（__FILE__）
include (cmake/utils.cmake) 

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# 将当前目录添加到包含目录列表
include_directories(.)
# 将/usr/local/include添加到包含目录列表
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(LIB_SRC
    captain/config.cpp
    captain/fiber.cpp
    captain/log.cpp
    captain/scheduler.cpp
    captain/thread.cpp
    captain/util.cpp
    )

add_library(captain SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(captain) #__FILE__ 它用于强制重新定义文件宏（__FILE__）变为相对路径显示 
#find_library(YAMLCPP yaml-cpp)
set(LIBS
        captain
        pthread
        yaml-cpp)

#message("***", ${LIBS})

add_executable(test tests/test.cpp)
add_dependencies(test captain)
force_redefine_file_macro_for_sources(test) #__FILE__
target_link_libraries(test ${LIBS})

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config captain)
force_redefine_file_macro_for_sources(test_config) #__FILE__
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread captain)
force_redefine_file_macro_for_sources(test_thread) #__FILE__
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util captain)
force_redefine_file_macro_for_sources(test_util) #__FILE__
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber captain)
force_redefine_file_macro_for_sources(test_fiber) #__FILE__
target_link_libraries(test_fiber ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
