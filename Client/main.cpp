#include "Client.h"
#include <iostream>
#include <string>

int main() {
    std::string serverIP = "127.0.0.1"; 
    int serverPort = 54000;           

    Client client(serverIP, serverPort);

    client.startChatSession();

    return 0;
}
