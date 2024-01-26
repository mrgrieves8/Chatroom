# Define the client target
add_executable(Client main.cpp Client.cpp)

# Include directories
target_include_directories(Client PRIVATE ${PROJECT_SOURCE_DIR}/Client)
