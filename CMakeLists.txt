# CMAKE Version and Project Name
cmake_minimum_required(VERSION 2.8)
project(dds-submit-mesos)

# Use C++11, enable all warnings and compile in debug mode
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
set(CMAKE_BUILD_TYPE DEBUG)
#set(CMAKE_VERBOSE_MAKEFILE ON)

# Enable Testing
enable_testing()

# Some values defined for user friendly IDE use
#set(BOOST_ROOT "/home/kevin/DDSAli/sw/ubuntu1404_x86-64/boost/latest")
#set(DDS_ROOT "/home/kevin/DDSAli/sw/ubuntu1404_x86-64/DDS/latest")
#set(GLOG_ROOT "/home/kevin/DDSAli/sw/ubuntu1404_x86-64/glog/latest")
#set(MESOS_ROOT "/home/kevin/DDSAli/sw/ubuntu1404_x86-64/mesos/latest")
#set(PROTOBUF_ROOT "/home/kevin/DDSAli/sw/ubuntu1404_x86-64/protobuf/latest")

# A definition required by the BOOST Log module - will make Boost link dynamically
add_definitions(-DBOOST_LOG_DYN_LINK)

# Find the system thread library and place in CMAKE_THREAD_LIBS_INIT - To link later
find_package (Threads)

# Select a specific Boost library, or use the default system one
if(BOOST_ROOT)
    find_library(BOOST_SYSTEM_LIBRARY boost_system
                 PATHS ${BOOST_ROOT}/lib NO_DEFAULT_PATH)
    find_library(BOOST_FILESYSTEM_LIBRARY boost_filesystem
            PATHS ${BOOST_ROOT}/lib NO_DEFAULT_PATH)
    find_library(BOOST_THREAD_LIBRARY boost_thread
            PATHS ${BOOST_ROOT}/lib NO_DEFAULT_PATH)
    find_library(BOOST_LOG_SETUP_LIBRARY boost_log_setup
            PATHS ${BOOST_ROOT}/lib NO_DEFAULT_PATH)
    find_library(BOOST_LOG_LIBRARY boost_log
            PATHS ${BOOST_ROOT}/lib NO_DEFAULT_PATH)
else()
    find_library(BOOST_SYSTEM_LIBRARY boost_system)
    find_library(BOOST_FILESYSTEM_LIBRARY boost_filesystem)
    find_library(BOOST_THREAD_LIBRARY boost_thread)
    find_library(BOOST_LOG_SETUP_LIBRARY boost_log_setup)
    find_library(BOOST_LOG_LIBRARY boost_log)
endif()
if(BOOST_ROOT)
    find_path(BOOST_INCDIR boost/foreach.hpp
            PATHS ${BOOST_ROOT}/include NO_DEFAULT_PATH)
endif()

# Mesos
find_path(MESOS_INCDIR mesos/mesos.hpp
        PATHS ${MESOS_ROOT}/include NO_DEFAULT_PATH)
find_library(MESOS_LIBRARY mesos 
             PATHS ${MESOS_ROOT}/lib NO_DEFAULT_PATH)
#message(${MESOS_INCDIR})

# Proto buf
find_path(PROTOBUF_INCDIR google/protobuf/stubs/common.h
        PATHS ${PROTOBUF_ROOT}/include NO_DEFAULT_PATH)
find_library(PROTOBUF_LIBRARY protobuf
             PATHS ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)

# DDS
find_path(DDS_INCDIR dds_intercom.h
        PATHS ${DDS_ROOT}/include NO_DEFAULT_PATH)
find_library(DDS_LIBRARY dds_intercom_lib
          PATHS ${DDS_ROOT}/lib NO_DEFAULT_PATH)

# Google Logs
find_path(GLOG_INCDIR glog/logging.h
          PATHS ${GLOG_ROOT}/include NO_DEFAULT_PATH)

# Include directories for all targets
# None are needed at the moment

# Common functionality static library target
add_library(dds-mesos-common STATIC Common Common/Utils.cc)
target_include_directories(dds-mesos-common PUBLIC
    Common
    ${BOOST_INCDIR}
)
target_link_libraries(dds-mesos-common
    ${BOOST_FILESYSTEM_LIBRARY}
    ${BOOST_LOG_SETUP_LIBRARY}
    ${BOOST_LOG_LIBRARY}
    ${BOOST_THREAD_LIBRARY}
    ${BOOST_SYSTEM_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
)

# mesos-dds static library target
add_library(mesos-dds STATIC DDSRestServer/DDSScheduler.cc)
target_include_directories(mesos-dds PUBLIC
    DDSRestServer
    ${MESOS_INCDIR}
    ${PROTOBUF_INCDIR}
    ${GLOG_INCDIR}
)
target_link_libraries(mesos-dds
    dds-mesos-common
    ${MESOS_LIBRARY}
    ${PROTOBUF_LIBRARY}
)

# dds-mesos-server executable target
add_executable(dds-mesos-server DDSRestServer/main.cc)
target_link_libraries(dds-mesos-server
    mesos-dds # Target link libraries linked in mesos-dds are linked here as well (includes too)
    dds-mesos-common
)

# dds-submit-mesos executable target
add_executable(dds-submit-mesos DDSPlugin/main.cc)
target_include_directories(dds-submit-mesos PUBLIC
    DDSPlugin
    ${DDS_INCDIR}
)
target_link_libraries(dds-submit-mesos
    ${DDS_LIBRARY}
    dds-mesos-common
)

# Installation options
install(TARGETS dds-submit-mesos 
        RUNTIME DESTINATION bin)
