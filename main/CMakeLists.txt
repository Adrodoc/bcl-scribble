add_executable(main src/main.cpp)
target_include_directories(main PUBLIC src)
target_link_libraries(main bcl_ext)
