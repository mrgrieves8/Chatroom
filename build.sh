#!/bin/bash

# Compile and build the server with debug symbols
g++ -g -std=c++11 Server/Server.cpp common/Message.cpp Server/main.cpp -o myserver

# Check if the server compilation was successful
if [ $? -eq 0 ]; then
    echo "Server compilation successful."
else
    echo "Server compilation failed."
    exit 1
fi

# Compile and build the client with debug symbols
g++ -g -std=c++11 Client/Client.cpp common/Message.cpp Client/main.cpp -o myclient

# Check if the client compilation was successful
if [ $? -eq 0 ]; then
    echo "Client compilation successful."
else
    echo "Client compilation failed."
    exit 1
fi

echo "Both server (myserver) and client (myclient) executables built successfully."
