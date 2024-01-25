#include "Client.h"
#include <iostream>
#include <string>

int main() {
    std::string serverIP = "127.0.0.1"; // Change this to your server's IP
    int serverPort = 50000;             // Change this to your server's port

    Client client(serverIP, serverPort);

    client.startChatSession();

    return 0;
}
