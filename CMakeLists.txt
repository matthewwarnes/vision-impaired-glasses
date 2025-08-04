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
include(libs/espeak.cmake)
include(libs/unialgo.cmake)


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
  src/audio/espeak_wrapper.cpp
)



if(NOT DEFINED ONNXRUNTIME_DIR)
    if(NOT DEFINED ONNXRUNTIME_VERSION)
        set(ONNXRUNTIME_VERSION "1.22.0")
    endif()

    if(WIN32)
        # Windows x86-64
        set(ONNXRUNTIME_PREFIX "onnxruntime-win-x64-${ONNXRUNTIME_VERSION}")
        set(ONNXRUNTIME_EXT "zip")
    elseif (APPLE)
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
            # MacOS x86-64
            set(ONNXRUNTIME_PREFIX "onnxruntime-osx-x86_64-${ONNXRUNTIME_VERSION}")
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL arm64)
            # MacOS Apple Silicon
            set(ONNXRUNTIME_PREFIX "onnxruntime-osx-arm64-${ONNXRUNTIME_VERSION}")
        else()
            message(FATAL_ERROR "Unsupported architecture for onnxruntime")
        endif()

        set(ONNXRUNTIME_EXT "tgz")
    else()
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
            # Linux x86-64
            set(ONNXRUNTIME_PREFIX "onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}")
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
            # Linux ARM 64-bit
            set(ONNXRUNTIME_PREFIX "onnxruntime-linux-aarch64-${ONNXRUNTIME_VERSION}")
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL armv7l)
            # Linux ARM 32-bit
            set(ONNXRUNTIME_PREFIX "onnxruntime-linux-arm32-${ONNXRUNTIME_VERSION}")
            set(ONNXRUNTIME_URL "https://github.com/synesthesiam/prebuilt-apps/releases/download/v1.0/onnxruntime-linux-arm32-${ONNXRUNTIME_VERSION}.tgz")
        else()
            message(FATAL_ERROR "Unsupported architecture for onnxruntime")
        endif()

        set(ONNXRUNTIME_EXT "tgz")
    endif()

    if(NOT DEFINED ONNXRUNTIME_URL)
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${ONNXRUNTIME_PREFIX}.${ONNXRUNTIME_EXT}")
    endif()

    set(ONNXRUNTIME_FILENAME "${ONNXRUNTIME_PREFIX}.${ONNXRUNTIME_EXT}")
    set(ONNXRUNTIME_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/${ONNXRUNTIME_PREFIX}")

    if(NOT EXISTS "${ONNXRUNTIME_DIR}")
        if(NOT EXISTS "download/${ONNXRUNTIME_FILENAME}")
            # Download onnxruntime release
            message("Downloading ${ONNXRUNTIME_URL}")
            file(DOWNLOAD "${ONNXRUNTIME_URL}" "download/${ONNXRUNTIME_FILENAME}")
        endif()

        # Extract .zip or .tgz to a directory like lib/onnxruntime-linux-x64-1.22.0/
        file(ARCHIVE_EXTRACT INPUT "download/${ONNXRUNTIME_FILENAME}" DESTINATION "${CMAKE_CURRENT_LIST_DIR}/lib")
    endif()
endif()



target_include_directories(glasses PUBLIC
    ${ONNXRUNTIME_DIR}/include
)
target_link_directories(glasses PUBLIC
    ${ONNXRUNTIME_DIR}/lib
)
target_link_libraries(glasses
    onnxruntime
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
target_link_libraries(glasses espeak)
target_link_libraries(glasses uni-algo)

install(TARGETS glasses RUNTIME DESTINATION ${BIN_INSTALLATION_DEST})
