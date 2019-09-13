# CMakeLists.txt for <X264EncoderFilter> 
cmake_minimum_required(VERSION 3.11)

# hints from https://github.com/robotology/how-to-export-cpp-library/blob/master/CMakeLists.txt
project(X264EncoderFilter
        LANGUAGES CXX C
        VERSION 0.0.0.0)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")

include(FetchContent)

FetchContent_Declare(
  DirectShowExt
  GIT_REPOSITORY https://github.com/CSIR-RTVC/DirectShowExt
)
# Declare dependencies
find_package(DirectShowExt 1.0.0 REQUIRED)

FetchContent_Declare(
  Vpp
  GIT_REPOSITORY https://github.com/CSIR-RTVC/vpp
)
# Declare dependencies
find_package(Vpp 1.0.0 REQUIRED)

set (x264_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/vcpkg/installed/x86-windows/include)
set (x264_LIB_DIR ${PROJECT_SOURCE_DIR}/vcpkg/installed/x86-windows/lib)

SET(FLT_HDRS
X264EncoderFilter.h
X264EncoderProperties.h
resource.h
stdafx.h
VersionInfo.h
)

SET(FLT_SRCS 
DLLSetup.cpp
X264EncoderFilter.cpp
X264EncoderFilter.def
X264EncoderFilter.rc
stdafx.cpp
)

ADD_LIBRARY(
X264EncoderFilter SHARED ${FLT_SRCS} ${FLT_HDRS})

target_include_directories(X264EncoderFilter
    PUBLIC 
        $<INSTALL_INTERFACE:install>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
		$<BUILD_INTERFACE:${x264_INCLUDE_DIR}>
)

LINK_DIRECTORIES(
    $ENV{MSSDK}/Lib
)

TARGET_LINK_LIBRARIES (
X264EncoderFilter
DirectShowExt::DirectShowExt
Vpp::Vpp
${x264_LIB_DIR}/libx264.lib
) 

INSTALL(
  TARGETS X264EncoderFilter
  ARCHIVE DESTINATION bin
  LIBRARY DESTINATION bin
)

IF (REGISTER_DS_FILTERS)
ADD_CUSTOM_COMMAND(
TARGET X264EncoderFilter 
POST_BUILD COMMAND 
regsvr32 /s \"$(TargetPath)\"
)
ENDIF(REGISTER_DS_FILTERS)
