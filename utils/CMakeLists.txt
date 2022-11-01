add_library(utils STATIC timer.h timer.cpp)
set_target_properties(utils PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
