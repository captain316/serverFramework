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
    captain/address.cpp
    captain/bytearray.cpp
    captain/config.cpp
    captain/fd_manager.cpp
    captain/fiber.cpp
    captain/http/http.cpp
    captain/http/http_parser.cpp
    captain/http/http_session.cpp
    captain/http/http_server.cpp
    captain/http/servlet.cpp
    captain/http/http11_parser.rl.cpp
    captain/http/httpclient_parser.rl.cpp
    captain/hook.cpp
    captain/iomanager.cpp
    captain/log.cpp
    captain/scheduler.cpp
    captain/socket.cpp
    captain/stream.cpp
    captain/streams/socket_stream.cpp
    captain/tcp_server.cpp
    captain/thread.cpp
    captain/timer.cpp
    captain/util.cpp
    )

add_library(captain SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(captain) #__FILE__ 它用于强制重新定义文件宏（__FILE__）变为相对路径显示 
#find_library(YAMLCPP yaml-cpp)
set(LIBS
        captain
        dl
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

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler captain)
force_redefine_file_macro_for_sources(test_scheduler) #__FILE__
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager captain)
force_redefine_file_macro_for_sources(test_iomanager) #__FILE__
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook captain)
force_redefine_file_macro_for_sources(test_hook) #__FILE__
target_link_libraries(test_hook ${LIBS})

add_executable(test_address tests/test_address.cpp)
add_dependencies(test_address captain)
force_redefine_file_macro_for_sources(test_address) #__FILE__
target_link_libraries(test_address ${LIBS})

add_executable(test_socket tests/test_socket.cpp)
add_dependencies(test_socket captain)
force_redefine_file_macro_for_sources(test_socket) #__FILE__
target_link_libraries(test_socket ${LIBS})

add_executable(test_bytearray tests/test_bytearray.cpp)
add_dependencies(test_bytearray captain)
force_redefine_file_macro_for_sources(test_bytearray) #__FILE__
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_http tests/test_http.cpp)
add_dependencies(test_http captain)
force_redefine_file_macro_for_sources(test_http) #__FILE__
target_link_libraries(test_http ${LIBS})

add_executable(test_http_parser tests/test_http_parser.cpp)
add_dependencies(test_http_parser captain)
force_redefine_file_macro_for_sources(test_http_parser) #__FILE__
target_link_libraries(test_http_parser ${LIBS})

add_executable(test_tcp_server tests/test_tcp_server.cpp)
add_dependencies(test_tcp_server captain)
force_redefine_file_macro_for_sources(test_tcp_server) #__FILE__
target_link_libraries(test_tcp_server ${LIBS})

add_executable(echo_server examples/echo_server.cpp)
add_dependencies(echo_server captain)
force_redefine_file_macro_for_sources(echo_server) #__FILE__
target_link_libraries(echo_server ${LIBS})

add_executable(test_http_server tests/test_http_server.cpp)
add_dependencies(test_http_server captain)
force_redefine_file_macro_for_sources(test_http_server) #__FILE__
target_link_libraries(test_http_server ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
