add_executable(Client main.cpp Client.cpp ../common/Message.cpp)

target_include_directories(Client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ../common)
