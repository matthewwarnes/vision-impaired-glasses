string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)

set(libportaudio_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libportaudio)

ExternalProject_Add(libportaudio
  PREFIX ${libportaudio_PREFIX}
  GIT_REPOSITORY https://github.com/PortAudio/portaudio
  GIT_TAG v19.7.0
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
        "-DPA_BUILD_SHARED:BOOL=OFF"
)

add_library(portaudio INTERFACE)
add_dependencies(portaudio libportaudio)

target_include_directories(portaudio INTERFACE ${libportaudio_PREFIX}/src/libportaudio/include)
target_link_libraries(portaudio INTERFACE "${libportaudio_PREFIX}/src/libportaudio-build/libportaudio.a")
target_link_libraries(portaudio INTERFACE asound)
target_link_libraries(portaudio INTERFACE jack)
