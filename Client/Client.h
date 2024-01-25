#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "../common/Message.h"
#include <atomic>
#include <condition_variable>
#include <mutex>


class Client {
public:
    enum class ClientState {
        PreLogin,
        SelectingChatroom,
        InChatroom
    };

    Client(const std::string& serverIP, int serverPort);
    virtual ~Client();
    bool connectToServer();
    void startChatSession();
    void sendMessage(const Message& message);
    Message receiveMessage();

private:
    std::string serverIP;
    int serverPort;
    int clientSocket;
    bool isConnected;
    void startReceivingMessages();
    void handleServerResponse();
    void displayChatInterface();
    std::atomic<ClientState> state;
    std::mutex stateMutex;
    std::condition_variable stateCondition;


    // Add other private methods as necessary
};

#endif // CLIENT_H
