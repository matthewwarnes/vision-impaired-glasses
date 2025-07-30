
find_package( OpenCV REQUIRED )

add_library (libopencv INTERFACE)

target_link_libraries(libopencv INTERFACE ${OpenCV_LIBS})
target_include_directories(libopencv INTERFACE ${OpenCV_INCLUDE_DIRS})
