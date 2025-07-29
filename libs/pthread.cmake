set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
add_library(threads INTERFACE)
target_link_libraries(threads INTERFACE Threads::Threads)
