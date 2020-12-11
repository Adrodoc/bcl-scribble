add_library(bcl_ext INTERFACE)
target_include_directories(bcl_ext INTERFACE .)
target_link_libraries(bcl_ext INTERFACE bcl)
