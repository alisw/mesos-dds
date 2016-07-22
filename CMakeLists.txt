# CMAKE Version and Project Name
cmake_minimum_required(VERSION 2.8)
project(dds-submit-mesos)

# Use C++11, enable all warnings and compile in debug mode
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
set(CMAKE_BUILD_TYPE DEBUG)

# Enable Testing
enable_testing()

# A definition required by the BOOST Log module - will make Boost link dynamically
add_definitions(-DBOOST_LOG_DYN_LINK)

# Find the system thread library and place in CMAKE_THREAD_LIBS_INIT - To link later
find_package(Threads)
find_package(OpenSSL)

# Select a specific Boost library, or use the default system one
find_library(BOOST_SYSTEM_LIBRARY boost_system
             PATHS ${BOOST_ROOT}/lib)
find_library(BOOST_FILESYSTEM_LIBRARY boost_filesystem
        PATHS ${BOOST_ROOT}/lib)
find_library(BOOST_THREAD_LIBRARY boost_thread
        PATHS ${BOOST_ROOT}/lib)
find_library(BOOST_LOG_SETUP_LIBRARY boost_log_setup
        PATHS ${BOOST_ROOT}/lib)
find_library(BOOST_LOG_LIBRARY boost_log
        PATHS ${BOOST_ROOT}/lib)
find_path(BOOST_INCDIR boost/foreach.hpp
        PATHS ${BOOST_ROOT}/include)

# Mesos
find_path(MESOS_INCDIR mesos/mesos.hpp
        PATHS ${MESOS_ROOT}/include)
find_library(MESOS_LIBRARY mesos 
             PATHS ${MESOS_ROOT}/lib)

# Proto buf
find_path(PROTOBUF_INCDIR google/protobuf/stubs/common.h
        PATHS ${PROTOBUF_ROOT}/include)
find_library(PROTOBUF_LIBRARY protobuf
             PATHS ${PROTOBUF_ROOT}/lib)

# DDS
find_path(DDS_INCDIR dds_intercom.h
        PATHS ${DDS_ROOT}/include)
find_library(DDS_LIBRARY dds_intercom_lib
          PATHS ${DDS_ROOT}/lib)

# Google Logs
find_path(GLOG_INCDIR glog/logging.h
          PATHS ${GLOG_ROOT}/include)

# CPPRESTSDK (Casablanca)
find_path(CPPRESTSDK_INCDIR cpprest/http_client.h
        PATHS ${CPPRESTSDK_ROOT}/include)
find_library(CPPRESTSDK_LIBRARY cpprest
        PATHS ${CPPRESTSDK_ROOT}/lib)

# Include directories for all targets
# None are needed at the moment

# Common functionality static library target
add_library(dds-mesos-common STATIC Common Common/Utils.cc Common/Constants.cc)
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
add_executable(dds-mesos-server DDSRestServer/main.cc DDSRestServer/Server.cc)
target_include_directories(dds-mesos-server PRIVATE
    ${CPPRESTSDK_INCDIR}
)
target_link_libraries(dds-mesos-server
    mesos-dds # Target link libraries linked in mesos-dds are linked here as well (includes too)
    dds-mesos-common
    ${CPPRESTSDK_LIBRARY}
    ${OPENSSL_LIBRARIES}
)

# dds-submit-mesos executable target
add_executable(dds-submit-mesos DDSPlugin/main.cc)
target_include_directories(dds-submit-mesos PRIVATE
    DDSPlugin
    ${DDS_INCDIR}
    ${CPPRESTSDK_INCDIR}
)
target_link_libraries(dds-submit-mesos
    ${DDS_LIBRARY}
    dds-mesos-common
    ${CPPRESTSDK_LIBRARY}
    ${OPENSSL_LIBRARIES}
)

# Installation options
install(TARGETS dds-submit-mesos 
        RUNTIME DESTINATION bin)
install(TARGETS dds-mesos-server
        RUNTIME DESTINATION bin)
