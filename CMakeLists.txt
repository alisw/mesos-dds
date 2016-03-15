cmake_minimum_required(VERSION 2.8)
set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_FLAGS "-std=c++11")

enable_testing()

find_library(MESOS_LIBRARY mesos 
             PATHS ${MESOS_ROOT}/lib NO_DEFAULT_PATH)
find_library(PROTOBUF_LIBRARY protobuf 
             PATHS ${PROTOBUF_ROOT}/lib NO_DEFAULT_PATH)
find_library(DDS_LIBRARY dds_intercom_lib
          PATHS ${DDS_ROOT}/lib NO_DEFAULT_PATH)

find_path(MESOS_INCDIR mesos/mesos.hpp 
          PATHS ${MESOS_ROOT}/include NO_DEFAULT_PATH)
find_path(BOOST_INCDIR boost/foreach.hpp
          PATHS ${BOOST_ROOT}/include NO_DEFAULT_PATH)
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
