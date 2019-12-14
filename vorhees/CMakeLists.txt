add_executable(vorhees main.cc)
include_directories(${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(vorhees LLVM libsprinkles)
