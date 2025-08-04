string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)

set(espeakng_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/espeak)

ExternalProject_Add(espeakng
  PREFIX ${espeakng_PREFIX}
  GIT_REPOSITORY https://github.com/espeak-ng/espeak-ng.git
  GIT_TAG 212928b394a96e8fd2096616bfd54e17845c48f6  # 2025-Mar-22
  GIT_SUBMODULES_RECURSE ON
  GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
  INSTALL_COMMAND ""
  LIST_SEPARATOR |
  CMAKE_CACHE_ARGS
        "-DCMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}:STRING=${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}"
        "-DCMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}:STRING=${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}"
        "-DCMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}:STRING=${CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}"
        "-DCMAKE_SHARED_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}:STRING=${CMAKE_SHARED_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}"
        "-DCMAKE_BUILD_TYPE:STRING=Release"
	      "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
        "-DBUILD_SHARED_LIBS:BOOL=OFF"
        "-DUSE_SPEECHPLAYER:BOOL=OFF"
)

add_library(espeak INTERFACE)
add_dependencies(espeak espeakng)

target_include_directories(espeak INTERFACE ${espeakng_PREFIX}/src/espeakng/src/libespeak-ng)
target_include_directories(espeak INTERFACE ${espeakng_PREFIX}/src/espeakng/src/include)
target_link_libraries(espeak INTERFACE "${espeakng_PREFIX}/src/espeakng-build/src/libespeak-ng/libespeak-ng.a")
target_link_libraries(espeak INTERFACE "${espeakng_PREFIX}/src/espeakng-build/src/ucd-tools/libucd.a")
