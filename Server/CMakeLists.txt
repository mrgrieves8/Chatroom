add_executable(Server main.cpp Server.cpp ../common/Message.cpp)

target_include_directories(Server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ../common)
