add_executable(boxworld_test boxworld_test.cpp)
target_link_libraries(boxworld_test PUBLIC boxworld)
add_test(boxworld_test boxworld_test)

