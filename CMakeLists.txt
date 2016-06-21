cmake_minimum_required(VERSION 2.8)
set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_FLAGS "-std=c++11")

enable_testing()

#
#set(BOOST_ROOT "/home/kevin/DDSAli/sw/ubuntu1404_x86-64/boost/latest")
#set(DDS_ROOT "/home/kevin/DDSAli/sw/ubuntu1404_x86-64/DDS/latest")
#set(GLOG_ROOT "/home/kevin/DDSAli/sw/ubuntu1404_x86-64/glog/latest")
#set(MESOS_ROOT "/home/kevin/DDSAli/sw/ubuntu1404_x86-64/mesos/latest")
#set(PROTOBUF_ROOT "/home/kevin/DDSAli/sw/ubuntu1404_x86-64/protobuf/latest")
#set(BOOST_INCDIR "")
#

add_definitions(-DBOOST_LOG_DYN_LINK)

find_package (Threads)

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
find_library(MESOS_LIBRARY mesos 
             PATHS ${MESOS_ROOT}/lib NO_DEFAULT_PATH)
find_library(PROTOBUF_LIBRARY protobuf 
             PATHS ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_library(DDS_LIBRARY dds_intercom_lib
          PATHS ${DDS_ROOT}/lib NO_DEFAULT_PATH)

find_path(MESOS_INCDIR mesos/mesos.hpp 
          PATHS ${MESOS_ROOT}/include NO_DEFAULT_PATH)
if(BOOST_ROOT)
find_path(BOOST_INCDIR boost/foreach.hpp
          PATHS ${BOOST_ROOT}/include NO_DEFAULT_PATH)
endif()
find_path(PROTOBUF_INCDIR google/protobuf/stubs/common.h
          PATHS ${PROTOBUF_ROOT}/include NO_DEFAULT_PATH)
find_path(GLOG_INCDIR glog/logging.h
          PATHS ${GLOG_ROOT}/include NO_DEFAULT_PATH)
find_path(DDS_INCDIR dds_intercom.h
          PATHS ${DDS_ROOT}/include NO_DEFAULT_PATH)

message(${MESOS_INCDIR})

include_directories(${MESOS_INCDIR}
                    ${PROTOBUF_INCDIR}
                    ${BOOST_INCDIR}
                    ${GLOG_INCDIR}
                    ${DDS_INCDIR})

add_library(mesos-dds STATIC 
            DDSScheduler.cc)
target_link_libraries(mesos-dds
                      ${MESOS_LIBRARY}
                      ${PROTOBUF_LIBRARY}
                      ${DDS_LIBRARY}
                      ${BOOST_FILESYSTEM_LIBRARY}
                      ${BOOST_LOG_SETUP_LIBRARY}
                      ${BOOST_LOG_LIBRARY}
                      ${BOOST_THREAD_LIBRARY}
                      ${BOOST_SYSTEM_LIBRARY}
                      ${CMAKE_THREAD_LIBS_INIT}
                      curl)
add_executable(dds-submit-mesos 
               main.cc)
target_include_directories(mesos-dds 
                           PUBLIC ${MESOS_INCDIR}
                           ${PROTOBUF_INCDIR}
                           ${BOOST_INCDIR}
                           ${GLOG_INCDIR}
                           ${DDS_INCDIR})
target_link_libraries(dds-submit-mesos 
                      mesos-dds
                      ${MESOS_LIBRARY}
                      ${PROTOBUF_LIBRARY}
                      ${DDS_LIBRARY}
                      curl)
install(TARGETS dds-submit-mesos 
        RUNTIME DESTINATION bin)
