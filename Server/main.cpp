#include <iostream>
#include <string>
#include "Server.h"

using namespace std;

int main() {
    string serverIP = "127.0.0.1";
    int serverPort = 50000;
    Server server(serverIP, serverPort);

    if (server.init()) {
        server.run();
    } else {
        cerr << "Server initialization failed." << endl;
    }

    return 0;
}
