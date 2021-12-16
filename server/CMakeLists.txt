# CMakeList.txt : CMake project for server, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
find_package(Boost COMPONENTS thread REQUIRED)
add_executable (server "server.cpp" "server.h")
target_link_libraries(server Boost::thread)

# TODO: Add tests and install targets if needed.
