string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)

set(whisper_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/whispercpp)

ExternalProject_Add(whispercpp
  PREFIX ${whisper_PREFIX}
  GIT_REPOSITORY https://github.com/ggml-org/whisper.cpp
  GIT_TAG v1.7.6
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
        "-DWHISPER_BUILD_EXAMPLES:BOOL=OFF"
        "-DWHISPER_BUILD_TESTS:BOOL=OFF"
)

add_library(whisper INTERFACE)
add_dependencies(whisper whispercpp)

target_include_directories(whisper INTERFACE ${whisper_PREFIX}/src/whispercpp/include)
target_include_directories(whisper INTERFACE ${whisper_PREFIX}/src/whispercpp/ggml/include)
target_link_libraries(whisper INTERFACE "${whisper_PREFIX}/src/whispercpp-build/src/libwhisper.a")
target_link_libraries(whisper INTERFACE "${whisper_PREFIX}/src/whispercpp-build/ggml/src/libggml.a")
target_link_libraries(whisper INTERFACE "${whisper_PREFIX}/src/whispercpp-build/ggml/src/libggml-base.a")
target_link_libraries(whisper INTERFACE "${whisper_PREFIX}/src/whispercpp-build/ggml/src/libggml-cpu.a")
