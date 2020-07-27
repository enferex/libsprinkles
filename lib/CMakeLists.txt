add_library(libsprinkles sprinkles.cc)
#add_compile_options("-fsanitize=address")
#link_libraries("-fsanitize=address")
set_property(TARGET libsprinkles PROPERTY CXX_STANDARD 17)
