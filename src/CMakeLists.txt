set(exec ${CMAKE_PROJECT_NAME}_main)

add_executable(${exec} main.cpp )
target_link_libraries(${exec} PUBLIC utils)

