#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "../common/Message.h"
#include <atomic>
#include <condition_variable>



class Client {
public:
    enum class ClientState {
        PreLogin,
        SelectingChatroom,
        InChatroom,
        Quitting
    };

    Client(const std::string& serverIP, int serverPort);
    virtual ~Client();
    bool connectToServer();
    void startChatSession();
    void handleQuitting();
    void handleSelectingChatroom();
    void handleInChatroom();
    void sendMessage(const Message &message);
    Message receiveMessage();

private:
    std::string serverIP;
    int serverPort;
    int clientSocket;
    bool isConnected;
    void startReceivingMessages();
    void notifyReadyToSend();
    void waitForMessageReady();
    void setNotReadyToSend();
    void handleServerResponse();
    void displayChatInterface();
    std::atomic<ClientState> state;
    std::mutex mtx;
    std::condition_variable cv;
    bool readyToSend = false;


    // Add other private methods as necessary
};

#endif // CLIENT_H

std::string getInputAndClearLine();
