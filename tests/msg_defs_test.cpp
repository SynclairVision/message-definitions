#include <gtest/gtest.h>
#include "../msg_defs.hpp"

// Exempel på en testfall för en meddelandedefinition
TEST(MsgDefsTest, ExampleMessageTest) {
    // Här kan du lägga till kod för att testa meddelandedefinitioner
    // från msg_defs.hpp

    // Exempel på en enkel testfall
    EXPECT_TRUE(true);  // Ersätt med dina faktiska testfall
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
