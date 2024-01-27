#include <iostream>
#include <string>
#include "Client.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << endl;
        return 1;
    }

    string serverIP = argv[1];
    int serverPort = atoi(argv[2]);

    Client client(serverIP, serverPort);

    client.startChatSession();

    return 0;
}
