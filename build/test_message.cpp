#include "msg_defs.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

int main() {
    std::cout << "Testing message struct and MessageType enum" << std::endl;

    // Skapa ett meddelande
    message msg;

    // Sätt värden för meddelandet
    msg.message_type = MessageType::VIDEO_OUTPUT;  // Använd enum för att sätta message_type
    msg.version = 0x01;
    msg.command = 0x02;
    msg.length = 128;
    msg.param_type = 0x10;
    std::strcpy(msg.data, "This is a test message.");  // Lägg till textdata i char arrayen

    // Kontrollera att värdena har satts korrekt
    assert(msg.message_type == MessageType::VIDEO_OUTPUT);
    assert(msg.version == 0x01);
    assert(msg.command == 0x02);
    assert(msg.length == 128);
    assert(msg.param_type == 0x10);
    assert(std::strcmp(msg.data, "This is a test message.") == 0);

    std::cout << "All assertions passed!" << std::endl;

    // Utskrift för att kontrollera värdena
    std::cout << "Message Type: " << msg.message_type << std::endl;
    std::cout << "Version: " << msg.version << std::endl;
    std::cout << "Command: " << msg.command << std::endl;
    std::cout << "Length: " << msg.length << std::endl;
    std::cout << "Param Type: " << msg.param_type << std::endl;
    std::cout << "Data: " << msg.data << std::endl;

    return 0;
}
