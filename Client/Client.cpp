#include "Client.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sstream>
#include <thread>
#include "../common/Message.h"
#include <atomic>



Client::Client(const std::string& serverIP, int serverPort)
    : serverIP(serverIP), serverPort(serverPort), clientSocket(-1), state(ClientState::PreLogin) {
    // Constructor initialization if needed
}

Client::~Client() {
    // Cleanup resources if needed
    if (clientSocket != -1) {
        close(clientSocket);
    }
}

bool Client::connectToServer() {
    // Create a socket to connect to the server
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }

    // Initialize server address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error connecting to server" << std::endl;
        return false;
    }

    return true;
}

void Client::startReceivingMessages() {
    std::thread receiveThread([this]() {
        while (true) {
            Message response = receiveMessage();
            switch (response.getType()) {
                case MessageType::JOIN:
                    state = ClientState::InChatroom;
                    std::cout << response.getBody() << std::endl;
                    notifyReadyToSend();
                    break;
                case MessageType::MENU:
                    state = ClientState::SelectingChatroom;
                    std::cout << response.getBody() << std::endl;
                    notifyReadyToSend();
                    break;
                case MessageType::QUIT:
                    std::cerr << "Connection closed." << std::endl;
                    return; // Exiting the thread
                case MessageType::POST:
                    std::cout << response.getBody() << std::endl;
                    notifyReadyToSend();
                    break;
                default:
                    std::cerr << "Unknown message type received." << std::endl;
                    break;
            }
        }
    });
    receiveThread.detach();
}

void Client::notifyReadyToSend() {
    std::lock_guard<std::mutex> lock(mtx);
    readyToSend = true;
    cv.notify_one();
}

void Client::startChatSession() {
    if (!connectToServer()) {
        std::cerr << "Failed to connect to server." << std::endl;
        return;
    }
    startReceivingMessages();
    if (state == ClientState::PreLogin) {
        handleServerResponse(); // Handle initial setup like username
        state = ClientState::SelectingChatroom;
    }

    std::string message;
    while (true) {
        waitForMessageReady();
        if (state == ClientState::SelectingChatroom) {
            std::getline(std::cin, message);
            sendMessage(Message(MessageType::JOIN, message));
        } else if (state == ClientState::InChatroom) {
            std::getline(std::cin, message);
            if (message == "/leave") {
                sendMessage(Message(MessageType::MENU, ""));
            } else {
                sendMessage(Message(MessageType::POST, message));
            }
        }
        setNotReadyToSend();
    }
}

void Client::waitForMessageReady() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]{ return readyToSend; });
}

void Client::setNotReadyToSend() {
    std::lock_guard<std::mutex> lock(mtx);
    readyToSend = false;
}


void Client::sendMessage(const Message& message) {
    std::string serializedMessage = message.serialize();
    send(clientSocket, serializedMessage.c_str(), serializedMessage.length(), 0);
}

Message Client::receiveMessage() {
    char buffer[1024] = {0};
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived <= 0) {
        std::cout << "Connection error or server closed the connection" << std::endl;
        return Message(MessageType::QUIT, "Connection error or server closed the connection");
    }
    
    std::string receivedData(buffer, bytesReceived);
    Message deserializedMessage = Message::deserialize(receivedData);
    return deserializedMessage;
}

void Client::handleServerResponse() {
    // Getting username from the user
    std::string username;
    std::getline(std::cin, username);
    sendMessage(Message(MessageType::LOGIN, username)); // Sending username to the server

    Message chatroomOptions = receiveMessage();
    std::cout << chatroomOptions.getBody(); // Displaying chatroom options
}