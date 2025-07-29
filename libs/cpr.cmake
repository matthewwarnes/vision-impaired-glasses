string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)

set(libcpr_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libcpr)
set(libcpr_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libcpr-out)

ExternalProject_Add(cpr
  PREFIX ${libcpr_PREFIX}
  GIT_REPOSITORY https://github.com/libcpr/cpr
  GIT_TAG 1.10.1
  GIT_SUBMODULES_RECURSE ON
  GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
  INSTALL_COMMAND ""
  BINARY_DIR ${libcpr_OUTPUT}
  SOURCE_DIR ${libcpr_OUTPUT}
  LIST_SEPARATOR |
  CMAKE_CACHE_ARGS
        "-DCMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}:STRING=${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}"
        "-DCMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}:STRING=${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}"
        "-DCMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}:STRING=${CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}"
        "-DCMAKE_SHARED_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}:STRING=${CMAKE_SHARED_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}"
        "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
	      "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}"
        "-DCPR_ENABLE_SSL:BOOL=ON"
        "-DBUILD_SHARED_LIBS:BOOL=OFF"
)

add_library(libcpr INTERFACE)
add_dependencies(libcpr cpr)

target_include_directories(libcpr INTERFACE ${libcpr_OUTPUT}/include/)
target_include_directories(libcpr INTERFACE ${libcpr_OUTPUT}/cpr_generated_includes/)
target_include_directories(libcpr INTERFACE ${libcpr_OUTPUT}/_deps/curl-src/include)
target_link_libraries(libcpr INTERFACE "${libcpr_OUTPUT}/lib/libcpr.a")
target_link_libraries(libcpr INTERFACE "${libcpr_OUTPUT}/lib/libcurl-d.a")
if(WIN32)
  target_link_libraries(libcpr INTERFACE ssh2 ws2_32 ssl ws2_32 gdi32 crypt32 crypto ws2_32 gdi32 crypt32 z)

else()
  find_package(OpenSSL REQUIRED)
  target_link_libraries(libcpr INTERFACE "${libcpr_OUTPUT}/lib/libz.a")
  target_link_libraries(libcpr INTERFACE OpenSSL::SSL)
  target_link_libraries(libcpr INTERFACE OpenSSL::Crypto)
endif()
