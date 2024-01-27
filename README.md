# Chatroom Project

## Description
This project implements a simple chatroom server and client using C++. It allows multiple clients to join chatrooms, send messages, and handle forbidden words.

## Requirements
- C++11
- Linux environment
- CMake (minimum version 3.10)

## Installation
1. Clone the repository: `git clone [Repository Link]`
2. Navigate to the project directory: `cd [Project Directory]`

## Build
Run the provided script to build the project:
```
./cMakeBuild.sh
```

## Running the Application

### Server
1. Navigate to the build directory: `cd build/Server`
2. Start the server: `./Server [ip] [port]`
 - for example:      `./Server 127.0.0.1 54000`

### Client
1. In a new terminal, navigate to the build directory: `cd build/Client`
2. Start a client instance: `./Client [ip] [port]`
 - for example:             `./Client 127.0.0.1 54000`

## Features
- Create and join chatrooms
- Send and receive messages in real-time
- Handle forbidden words in chatrooms
