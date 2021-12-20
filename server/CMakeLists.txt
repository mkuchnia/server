# CMakeList.txt : CMake project for server, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
find_package(Boost COMPONENTS thread REQUIRED)
find_package(jsoncpp)
add_executable(server "server.cpp" "server.hpp" "clientService.cpp" "clientService.hpp")
target_link_libraries(server Boost::thread)
target_link_libraries(server jsoncpp_lib)
target_link_libraries(server mysqlclient)

# TODO: Add tests and install targets if needed.
