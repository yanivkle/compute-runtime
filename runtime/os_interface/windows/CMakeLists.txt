#
# Copyright (C) 2018-2020 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(RUNTIME_SRCS_OS_INTERFACE_WINDOWS
  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
  ${CMAKE_CURRENT_SOURCE_DIR}/api_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/d3d10_11_sharing_functions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/d3d9_sharing_functions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/d3d_sharing_functions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/deferrable_deletion_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/deferrable_deletion_win.h
  ${CMAKE_CURRENT_SOURCE_DIR}/device_command_stream.inl
  ${CMAKE_CURRENT_SOURCE_DIR}/device_factory_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/driver_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/driver_info.h
  ${CMAKE_CURRENT_SOURCE_DIR}/environment_variables.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ocl_reg_path.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/os_context_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/os_context_win.h
  ${CMAKE_CURRENT_SOURCE_DIR}/os_interface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/os_interface.h
  ${CMAKE_CURRENT_SOURCE_DIR}/os_metrics_library.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/os_socket.h
  ${CMAKE_CURRENT_SOURCE_DIR}/os_time_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/os_time_win.h
  ${CMAKE_CURRENT_SOURCE_DIR}/performance_counters_win.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/performance_counters_win.h
  ${CMAKE_CURRENT_SOURCE_DIR}${BRANCH_DIR_SUFFIX}/wddm_additional_context_flags.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm_device_command_stream.h
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm_device_command_stream.inl
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm_memory_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm_memory_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}${BRANCH_DIR_SUFFIX}/wddm_memory_manager_allocate_in_device_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm_residency_allocations_container.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm_residency_allocations_container.h
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm_residency_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm_residency_controller.h
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm_memory_operations_handler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm_memory_operations_handler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/windows_inc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hw_info_config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm/wddm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm/wddm.h
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm/wddm_interface.h
  ${CMAKE_CURRENT_SOURCE_DIR}/wddm/wddm_interface.cpp
)

if(WIN32)
  target_sources(${NEO_STATIC_LIB_NAME} PRIVATE
    ${RUNTIME_SRCS_OS_INTERFACE_WINDOWS}
  )
endif()

add_subdirectories()
