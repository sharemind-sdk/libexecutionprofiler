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

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(SharemindLibExecutionProfiler VERSION 0.4.0 LANGUAGES CXX)

INCLUDE("${CMAKE_CURRENT_SOURCE_DIR}/config.local" OPTIONAL)
INCLUDE("${CMAKE_CURRENT_BINARY_DIR}/config.local" OPTIONAL)

FIND_PACKAGE(SharemindCMakeHelpers 1.6 REQUIRED)

FIND_PACKAGE(LogHard 0.5.0 REQUIRED)
FIND_PACKAGE(SharemindCxxHeaders 0.8.0 REQUIRED)

# Headers:
FILE(GLOB_RECURSE SharemindLibExecutionProfiler_HEADERS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
INSTALL(FILES ${SharemindLibExecutionProfiler_HEADERS}
        DESTINATION "include/sharemind"
        COMPONENT dev)

# LibExecutionProfiler:
IF(SHAREMIND_NETWORK_STATISTICS)
    SharemindListAppendUnique(SharemindLibExecutionProfiler_DEFINITIONS
        "-DSHAREMIND_NETWORK_STATISTICS_ENABLE"
    )
ENDIF(SHAREMIND_NETWORK_STATISTICS)
FILE(GLOB_RECURSE SharemindLibExecutionProfiler_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
SharemindAddSharedLibrary(LibExecutionProfiler
    OUTPUT_NAME "sharemind_executionprofiler"
    SOURCES
        ${SharemindLibExecutionProfiler_SOURCES}
        ${SharemindLibExecutionProfiler_HEADERS}
)
TARGET_INCLUDE_DIRECTORIES(LibExecutionProfiler
    INTERFACE
        # $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> # TODO
        $<INSTALL_INTERFACE:include>
    )
TARGET_LINK_LIBRARIES(LibExecutionProfiler
    PUBLIC
        LogHard::LogHard
        Sharemind::CxxHeaders
    )
SharemindCreateCMakeFindFilesForTarget(LibExecutionProfiler
    DEPENDENCIES
        "LogHard 0.5.0"
        "SharemindCxxHeaders 0.8.0"
    )

# Packaging:
SharemindSetupPackaging()
SharemindAddComponentPackage("lib"
    NAME "libsharemind-executionprofiler"
    DESCRIPTION "Sharemind Execution Profiler library"
    DEB_SECTION "libs"
    DEB_DEPENDS
        "libloghard (>= 0.5.0)"
        "libstdc++6 (>= 4.8.0)"
)
SharemindAddComponentPackage("dev"
    NAME "libsharemind-executionprofiler-dev"
    DESCRIPTION "Sharemind Execution Profiler library development headers"
    DEB_SECTION "libdevel"
    DEB_DEPENDS
        "libsharemind-executionprofiler (= ${SharemindLibExecutionProfiler_DEB_lib_PACKAGE_VERSION})"
        "libloghard-dev (>= 0.4.0)"
        "libsharemind-cxxheaders-dev (>= 0.8.0)"
        "libstdc++-dev"
)
SharemindAddComponentPackage("debug"
    NAME "libsharemind-executionprofiler-dbg"
    DESCRIPTION "Sharemind Execution Profiler library debug symbols"
    DEB_SECTION "debug"
    DEB_DEPENDS
        "libsharemind-executionprofiler (= ${SharemindLibExecutionProfiler_DEB_lib_PACKAGE_VERSION})"
)
SharemindPackagingFinalize()
