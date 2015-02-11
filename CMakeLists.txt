#
# Copyright (C) 2015 Cybernetica
#
# Research/Commercial License Usage
# Licensees holding a valid Research License or Commercial License
# for the Software may use this file according to the written
# agreement between you and Cybernetica.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3.0 as published by the Free Software
# Foundation and appearing in the file LICENSE.GPL included in the
# packaging of this file.  Please review the following information to
# ensure the GNU General Public License version 3.0 requirements will be
# met: http://www.gnu.org/copyleft/gpl-3.0.html.
#
# For further information, please contact us at sharemind@cyber.ee.
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(SharemindLibExecutionProfiler CXX)

INCLUDE("${CMAKE_CURRENT_SOURCE_DIR}/config.local" OPTIONAL)
INCLUDE("${CMAKE_CURRENT_BINARY_DIR}/config.local" OPTIONAL)

SET(LIBEXECUTIONPROFILER_VERSION "0.1.0")

SET(Boost_USE_STATIC_RUNTIME OFF)
FIND_PACKAGE(LogHard REQUIRED)
FIND_PACKAGE(SharemindCxxHeaders REQUIRED)

SET(LIBEXECUTIONPROFILER_EXTERNAL_LINK_LIBRARIES "")
LIST(APPEND LIBEXECUTIONPROFILER_EXTERNAL_LINK_LIBRARIES
    ${SharemindCxxHeaders_LIBRARIES}
    ${LogHard_LIBRARIES}
)
SET(LIBEXECUTIONPROFILER_EXTERNAL_INCLUDE_DIRS "")
LIST(APPEND LIBEXECUTIONPROFILER_EXTERNAL_INCLUDE_DIRS
    ${SharemindCxxHeaders_INCLUDE_DIRS}
    ${LogHard_INCLUDE_DIRS}
)
SET(LIBEXECUTIONPROFILER_EXTERNAL_DEFINITIONS "")
LIST(APPEND LIBEXECUTIONPROFILER_EXTERNAL_DEFINITIONS
    ${SharemindCxxHeaders_DEFINITIONS}
    ${LogHard_DEFINITIONS}
)
LIST(REMOVE_DUPLICATES LIBEXECUTIONPROFILER_EXTERNAL_LINK_LIBRARIES)
LIST(REMOVE_DUPLICATES LIBEXECUTIONPROFILER_EXTERNAL_INCLUDE_DIRS)
LIST(REMOVE_DUPLICATES LIBEXECUTIONPROFILER_EXTERNAL_DEFINITIONS)


SET(LIBEXECUTIONPROFILER_INSTALL_LINK_LIBRARIES
    ${LIBEXECUTIONPROFILER_EXTERNAL_LINK_LIBRARIES}
    "-L${CMAKE_INSTALL_PREFIX}/lib" "-lsharemind_executionprofiler")
SET(LIBEXECUTIONPROFILER_INSTALL_INCLUDE_DIRS
    "${CMAKE_INSTALL_PREFIX}/include"
    ${LIBEXECUTIONPROFILER_EXTERNAL_INCLUDE_DIRS})
SET(LIBEXECUTIONPROFILER_INSTALL_DEFINITIONS
    ${LIBEXECUTIONPROFILER_EXTERNAL_DEFINITIONS})
LIST(REMOVE_DUPLICATES LIBEXECUTIONPROFILER_INSTALL_INCLUDE_DIRS)

INCLUDE_DIRECTORIES(${LIBEXECUTIONPROFILER_EXTERNAL_INCLUDE_DIRS})
ADD_DEFINITIONS(${LIBEXECUTIONPROFILER_EXTERNAL_DEFINITIONS})
ADD_COMPILE_OPTIONS(
  "-std=c++11" "-Wall" "-Wextra" "-O2" "-fPIC"
  "$<$<NOT:$<STREQUAL:$<CONFIGURATION>,Release>>:-ggdb>"
  "$<$<NOT:$<STREQUAL:$<CONFIGURATION>,Release>>:-fno-omit-frame-pointer>"
  "$<$<STREQUAL:$<CONFIGURATION>,Release>:-DNDEBUG>"
  "$<$<STREQUAL:$<CONFIGURATION>,Release>:-fomit-frame-pointer>"
)

# For enabling network statistics in the profiling results
IF(SHAREMIND_NETWORK_STATISTICS)
    ADD_DEFINITIONS("-DSHAREMIND_NETWORK_STATISTICS_ENABLE")
ENDIF(SHAREMIND_NETWORK_STATISTICS)

FILE(GLOB_RECURSE LIBEXECUTIONPROFILER_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
FILE(GLOB_RECURSE LIBEXECUTIONPROFILER_HEADERS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
ADD_LIBRARY("libexecutionprofiler" SHARED ${LIBEXECUTIONPROFILER_SOURCES}
                                          ${LIBEXECUTIONPROFILER_HEADERS})
SET_TARGET_PROPERTIES("libexecutionprofiler" PROPERTIES
                      OUTPUT_NAME "sharemind_executionprofiler"
                      VERSION "${LIBEXECUTIONPROFILER_VERSION}"
                      SOVERSION "0.1")
TARGET_LINK_LIBRARIES("libexecutionprofiler"
                      ${LIBEXECUTIONPROFILER_EXTERNAL_LINK_LIBRARIES})
INSTALL(TARGETS "libexecutionprofiler" LIBRARY DESTINATION "lib")

INSTALL(FILES ${LIBEXECUTIONPROFILER_HEADERS} DESTINATION "include/sharemind")

FILE(GLOB_RECURSE LIBEXECUTIONPROFILER_INFILES
     "${CMAKE_CURRENT_SOURCE_DIR}/*.in")
ADD_CUSTOM_TARGET("include_LIBEXECUTIONPROFILER_INFILES_in_IDE"
                  SOURCES ${LIBEXECUTIONPROFILER_INFILES})

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/SharemindLibExecutionProfilerConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/SharemindLibExecutionProfilerConfig.cmake"
  @ONLY)
INSTALL(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/SharemindLibExecutionProfilerConfig.cmake"
  DESTINATION "lib/SharemindLibExecutionProfiler")
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/SharemindLibExecutionProfilerConfigVersion.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/SharemindLibExecutionProfilerConfigVersion.cmake"
  @ONLY)
INSTALL(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/SharemindLibExecutionProfilerConfigVersion.cmake"
  DESTINATION "lib/SharemindLibExecutionProfiler")

