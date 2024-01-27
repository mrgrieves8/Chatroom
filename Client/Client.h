#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "../common/Message.h"
#include <atomic>
#include <condition_variable>



class Client {
public:
    enum class ClientState {
        PreLogin,               // The client didn't send it's username and not connected to the server's EPOLL
        SelectingChatroom,      // The client recieved the chatroom menu and is selecting a chatroom
        InChatroom,             // The client is in a chatroom
        Quitting                // The client is quitting the program
    };

    Client(const std::string& serverIP, int serverPort);
    virtual ~Client();
    void startChatSession();
    

private:
    bool connectToServer();
    void handleQuitting();
    void handleSelectingChatroom();
    void handleInChatroom();
    void sendMessage(const Message &message);
    Message receiveMessage();
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
    std::string getInputAndClearLine();

};

#endif // CLIENT_H

