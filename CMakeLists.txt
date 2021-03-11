cmake_minimum_required(VERSION 3.14)
project(NVM_write_latency)

set(CMAKE_CXX_STANDARD 14)

include(FindThreads)

if(NOT WIN32)
        find_package(PkgConfig QUIET)
endif()

if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBPMEM REQUIRED libpmem)
else()
    find_package(LIBPMEM REQUIRED)
endif()

link_directories(${LIBPMEM_LIBRARY_DIRS})

add_executable(NVM_write_latency main.cpp)
target_include_directories(NVM_write_latency PUBLIC ${LIBPMEM_INCLUDE_DIRS})

target_link_libraries(NVM_write_latency ${LIBPMEM_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
