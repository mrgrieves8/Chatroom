cmake_minimum_required(VERSION 3.10)
project(Chatroom)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add subdirectories
add_subdirectory(Server)
add_subdirectory(Client)
