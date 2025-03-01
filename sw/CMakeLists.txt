cmake_minimum_required(VERSION 2.8)
project(F-RPC)

# Options
option(PLATFORM_BDX "Build for BDX platform" OFF)
option(PLATFORM_SDP "Build for Skylake platform" ON)
option(PLATFORM_ASE "Build for the ASE simulation environment" ON)
option(BUILD_WITH_PHY_NETWORK "With physical networking" OFF)
option(BUILD_TESTS "Build with unit and system tests" ON)
option(BUILD_MICROBENCHMARKS "Build microbenchmarks" ON)
option(BUILD_APPLICATIONS "Build end-to-end applications" ON)

set(CMAKE_CXX_COMPILER g++)

add_definitions(-std=c++11 -O3 -march=native)
add_definitions(-Wall -Wextra -Wabi -Wsign-conversion -Wformat -Wformat-security)
# TODO: make it compilable with -Werror
#add_definitions(-Werror)
add_definitions(-pedantic)
add_definitions(-fstack-protector -fPIE -fPIC)
add_definitions(-Wno-unused-function)
add_definitions(-D_FORTIFY_SOURCE=2)

# Debug and logging config
#add_definitions(-DFRPC_LOG_LEVEL=3)
#add_definitions(-DENABLE_DEBUG=1)
#add_definitions(-DENABLE_ASSERT=1)

# CCI-P config
add_definitions(-DNIC_CCIP_POLLING)
#add_definitions(-DNIC_CCIP_MMIO)
#add_definitions(-DNIC_CCIP_DMA)

# Networking config
if (BUILD_WITH_PHY_NETWORK)
    message(STATUS "Bulding WITH physical networking enabled" )
    if (PLATFORM_BDX or PLATFORM_SDP)
        message(FATAL_ERROR "Physical networking can only be enabled on the PAC_A10 platform" )
    else()
        add_definitions(-DNIC_PHY_NETWORK)
    endif()
endif()

# Platform config
if (PLATFORM_BDX)
    message(STATUS "Bulding for BDX platform" )
    add_definitions(-DPLATFORM_BDX)
elseif(PLATFORM_SDP)
    message(STATUS "Bulding for Skylake platform" )
    add_definitions(-DPLATFORM_SDP)
else()
    message(STATUS "Bulding for PAC_A10 platform" )
    add_definitions(-DPLATFORM_PAC_A10)
endif()

set(LIBRARIES ${LIBRARIES} -z noexecstack)
set(LIBRARIES ${LIBRARIES} -z relro -z now)
set(LIBRARIES ${LIBRARIES} pthread -pie)
set(LIBRARIES ${LIBRARIES} -luuid)

# Testing
if (BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
endif()

# FPGA OPAE
set(FPGA_LIBS -lopae-c)
set(ASE_LIBS -lopae-c-ase)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/nic_impl)
include_directories(${CMAKE_SOURCE_DIR}/src/utils)
include_directories(${CMAKE_SOURCE_DIR}/src/network_ctl)

link_directories(${CMAKE_CURRENT_BINARY_DIR})

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(RPC_CODEGEN_PATH ${ROOT_DIR}/codegen)

set(SOURCES
    src/nic_impl/nic_ccip.cc
    src/nic_impl/nic_ccip_polling.cc
    src/nic_impl/nic_ccip_mmio.cc
    src/nic_impl/nic_ccip_dma.cc
    src/rpc_server_thread.cc
    src/rpc_threaded_server.cc
    src/tx_queue.cc
    src/rx_queue.cc
    src/completion_queue.cc
    src/rpc_client_nonblocking_base.cc
    src/connection_manager.cc
    )

set(PHY_NET_SRC
    src/network_ctl/fpga_hssi_common.c
    src/network_ctl/fpga_hssi_e40.c
    )

# Prepare afu_json_info.h
if (BUILD_WITH_PHY_NETWORK)
    set(JSON_FILENAME ${CMAKE_SOURCE_DIR}/../hw/rtl/ccip_std_afu_hssi.json)
else()
    set(JSON_FILENAME ${CMAKE_SOURCE_DIR}/../hw/rtl/ccip_std_afu.json)
endif()

set(JSON_HEADER ${CMAKE_CURRENT_BINARY_DIR}/afu_json_info.h)
execute_process(COMMAND afu_json_mgr json-info --afu-json=${JSON_FILENAME} --c-hdr=${JSON_HEADER}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE JSON_GEN_RESULT)
if(NOT JSON_GEN_RESULT EQUAL "0")
    message(FATAL_ERROR "failed to generate AFU json")
endif()
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#
# Aux functions
#
FUNCTION(PREPEND var prefix)
  SET(listVar "")
  FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
  ENDFOREACH(f)
  SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

#
# Build PHY network controller as a shared library
#
add_library(dagger-phy-network SHARED ${PHY_NET_SRC})
target_link_libraries(dagger-phy-network ${LIBRARIES} -shared)

set(LIBRARIES ${LIBRARIES} -ldagger-phy-network)

#
# Build FPGA Dagger as a shared library
#
add_library(dagger SHARED ${SOURCES})
add_dependencies(dagger dagger-phy-network)
target_compile_definitions(dagger PRIVATE PROFILE_LATENCY=1)
target_link_libraries(dagger ${FPGA_LIBS} ${LIBRARIES} -shared)

#
# Build ASE Dagger as a shared library
#
if (PLATFORM_ASE)
    add_library(dagger_ase SHARED ${SOURCES})
    add_dependencies(dagger_ase dagger-phy-network)
    target_compile_definitions(dagger_ase PRIVATE -DASE_SIMULATION)
    target_link_libraries(dagger_ase ${ASE_LIBS} ${LIBRARIES} -shared)
endif()

#
# Build tests
#
if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

#
# Build ASE samples
#
if (PLATFORM_ASE)
    add_subdirectory(ase_samples)
endif()

#
# Build microbenchmarks
#
if (BUILD_MICROBENCHMARKS)
    add_subdirectory(microbenchmarks)
endif()

#
# Build end-to-end applications
#
if (BUILD_APPLICATIONS)
    add_subdirectory(apps)
endif()
