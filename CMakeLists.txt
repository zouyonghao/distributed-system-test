cmake_minimum_required(VERSION 3.4.3)
project(distributed_system_test)
set(CMAKE_CXX_STANDARD 14)

include(CTest)

set(DIST_FUZZ_STATIC_BUILD OFF CACHE BOOL "Build static?")
set(DIST_FUZZ_DEFAULT_TEST ON CACHE BOOL "Build default test like braft?")
set(DIST_FUZZ_BUILD_COVERAGE_COLLECTOR OFF CACHE BOOL "Build coverage collector with rdkafka?")

set(DIST_FUZZ_BPF OFF CACHE BOOL "Build bpf fault injector?")
set(DIST_FUZZ_RR ON CACHE BOOL "Build rr replay and fault injector?")
set(DIST_FUZZ_AEROSPIKE ON CACHE BOOL "Build aerospike?")

if (${DIST_FUZZ_STATIC_BUILD})
set(Boost_USE_STATIC_LIBS ON)
else()
# see https://stackoverflow.com/questions/23137637/linker-error-while-linking-boost-log-tutorial-undefined-references
add_compile_definitions(BOOST_LOG_DYN_LINK=1)
set(STATIC_RUNTIME OFF)
set(Boost_USE_STATIC_LIBS OFF)
set(OPENSSL_USE_STATIC_LIBS OFF)
endif()

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS log system filesystem)

add_subdirectory(event_control)
add_subdirectory(core)
add_subdirectory(fuzz)

if (${DIST_FUZZ_DEFAULT_TEST})
add_subdirectory(braft_test)
add_subdirectory(c-raft_test)
add_subdirectory(dqlite_test)
add_subdirectory(nuraft_test)
add_subdirectory(redisraft_test)
add_subdirectory(raftis_test)
add_subdirectory(rethinkdb_test)

if (${DIST_FUZZ_AEROSPIKE})
add_subdirectory(aerospike_test)
endif()

add_subdirectory(clickhouse_test)
add_subdirectory(zookeeper_test)
add_subdirectory(hdfs_test)
add_subdirectory(etcd_test)
endif()

add_subdirectory(general_test)

if (${DIST_FUZZ_BPF})
add_subdirectory(bpf)
endif()

if (${DIST_FUZZ_RR})
add_subdirectory(rr)
endif()