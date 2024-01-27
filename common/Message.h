#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <set>

enum class MessageType {
    JOIN, // the client uses JOIN msgs to ask the server to enter a chatroom, 
          // the server uses JOIN msgs to notify the client that he succeded in joining a room.
    
    MENU, // the client uses MENU msgs to ask the server to leave a chatroom and recieve the chatroom menu,
          // the server uses MENU msgs to deliver the chatroom menu to the user.
    
    QUIT, // the client uses QUIT msgs to notify the server that it's closing their socket,
          // the server uses QUIT msgs to notify the client that it's closing their socket.
    
    POST, // the client uses POST msgs to send a msg to the chatroom,
          // the server uses POST msgs to send a msg to the client.
    
    LOGIN, // the client uses LOGIN msgs to send it's username to the server.

    CREATE // the client uses CREATE msgs to ask the server to create an new chatroom. 
};

class Message {
private:
    MessageType type;
    std::string body;

public:
    Message(MessageType messageType, const std::string& messageBody);
    ~Message();

    MessageType getType() const;
    const std::string& getBody() const;

    std::string serialize() const;
    static Message deserialize(const std::string& serializedData);
    void censorWords(const std::set<std::string>& forbiddenWords);
};

#endif