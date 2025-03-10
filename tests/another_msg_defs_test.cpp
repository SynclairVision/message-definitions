#include <gtest/gtest.h>
#include "../msg_defs.hpp"

// Exempel på ett testfall för att testa en meddelandestruktur
TEST(MsgDefsTest, ExampleMessageTest) {
    // Antag att det finns en struktur 'Message' definierad i msg_defs.hpp
    Message msg;
    msg.id = 1;
    msg.content = "Hello, world!";
    EXPECT_EQ(msg.id, 1);
    EXPECT_STREQ(msg.content.c_str(), "Hello, world!");
}

// Ytterligare testfall
TEST(MsgDefsTest, AnotherMessageTest) {
    Message msg;
    msg.id = 2;
    msg.content = "Test message";
    EXPECT_EQ(msg.id, 2);
    EXPECT_STREQ(msg.content.c_str(), "Test message");
}

// Test för en funktion som returnerar ett meddelande
TEST(MsgDefsTest, GetMessageTest) {
    // Antag att det finns en funktion 'GetMessage' definierad i msg_defs.hpp
    Message msg = GetMessage(3, "This is a test");
    EXPECT_EQ(msg.id, 3);
    EXPECT_STREQ(msg.content.c_str(), "This is a test");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
