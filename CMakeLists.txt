cmake_minimum_required(VERSION 3.5)

if (${CMAKE_VERSION} VERSION_GREATER "3.23")
  cmake_policy(SET CMP0135 NEW)
endif()

# Version (should be updated on release)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

# Set project name
project(vision-impaired-glasses)
set(PROJ_NAME "\"Vision Impaired Glasses\"")
set(PROJECT_DESCRIPTION "\"Software to display captured video to smart glass display and provide voice control and AI integration\"")

set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
add_definitions(-DVIG_VERSION="${VERSION}")

# Set C++ standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(ExternalProject)

# Build type
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as no build type was specified")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type (Debug/Release)" FORCE)
endif (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

# Compiler flags
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-unused-result")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
else(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(FATAL_ERROR "No compatible compiler selected for this project")
endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# Define output dirs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Installation prefix" FORCE)
        message(STATUS "CMake install prefix defaulting to ${CMAKE_INSTALL_PREFIX}")
endif()

# Set up Linux install directories
include(GNUInstallDirs)

# Prefix-less install locations
set(DATA_INSTALLATION_DEST "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}")
set(BIN_INSTALLATION_DEST "${CMAKE_INSTALL_BINDIR}")
set(LIB_INSTALLATION_DEST "${CMAKE_INSTALL_LIBDIR}")
set(INCLUDE_INSTALLATION_DEST "${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_PROJECT_NAME}")
set(DOC_INSTALLATION_DEST "share/doc/${CMAKE_PROJECT_NAME}")
set(SYSCONF_INSTALLATION_DEST "${CMAKE_INSTALL_SYSCONFDIR}")
set(LOCALSTATE_INSTALLATION_DEST "${CMAKE_INSTALL_LOCALSTATEDIR}/${CMAKE_PROJECT_NAME}")

# Prefixed install locations
set(DATA_INSTALLATION_FULL_DEST "${CMAKE_INSTALL_FULL_DATADIR}/${CMAKE_PROJECT_NAME}")
set(BIN_INSTALLATION_FULL_DEST "${CMAKE_INSTALL_FULL_BINDIR}")
set(LIB_INSTALLATION_FULL_DEST "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDE_INSTALLATION_FULL_DEST "${CMAKE_INSTALL_FULL_INCLUDEDIR}/${CMAKE_PROJECT_NAME}")
set(DOC_INSTALLATION_FULL_DEST "${CMAKE_INSTALL_PREFIX}/${DOC_INSTALLATION_DEST}")
set(SYSCONF_INSTALLATION_FULL_DEST "${CMAKE_INSTALL_FULL_SYSCONFDIR}")
set(LOCALSTATE_INSTALLATION_FULL_DEST "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/${CMAKE_PROJECT_NAME}")

add_subdirectory(package)
add_subdirectory(samples)

include(libs/pthread.cmake)
include(libs/json.cmake)
include(libs/yaml.cmake)
include(libs/cpr.cmake)
include(libs/base64.cmake)
include(libs/libsdl.cmake)
include(libs/portaudio.cmake)
include(libs/opencv.cmake)
include(libs/whisper.cmake)


# -> Executables
add_executable(glasses
  src/config/config.cpp
  src/config/anyoption.cpp
  src/openai/ai_wrapper.cpp
  src/audio/audio_wrapper.cpp
  src/audio/whisper_wrapper.cpp
  src/main.cpp
  src/control_thread.cpp
  src/image_thread.cpp
)

# Includes
# -> Base of the source tree
target_include_directories(glasses
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(glasses threads)
target_link_libraries(glasses json)
target_link_libraries(glasses yaml)
target_link_libraries(glasses libcpr)
target_link_libraries(glasses libbase64)
target_link_libraries(glasses libsdl)
target_link_libraries(glasses portaudio)
target_link_libraries(glasses libopencv)
target_link_libraries(glasses whisper)

install(TARGETS glasses RUNTIME DESTINATION ${BIN_INSTALLATION_DEST})
