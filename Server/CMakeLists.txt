# Define the server target
add_executable(Server main.cpp Server.cpp)

# Include directories
target_include_directories(Server PRIVATE ${PROJECT_SOURCE_DIR}/Server)
