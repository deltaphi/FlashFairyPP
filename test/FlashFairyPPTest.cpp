#include "Mocks.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace FlashFairyPP {

TEST_F(VirtualFlashFixture, Read_Empty) { EXPECT_EQ(flashFairy.getValue(42), 0xCAFE); }

TEST_F(VirtualFlashFixture, Write_OutOfBounds) {
  EXPECT_EQ(flashFairy.getValue(FlashFairyPP::kNumKeys - 1), 0xCAFE);
  EXPECT_FALSE(flashFairy.setValue(FlashFairyPP::kNumKeys, 0xDEAD));
  EXPECT_EQ(flashFairy.getValue(FlashFairyPP::kNumKeys), 0xCAFE);
  EXPECT_EQ(flashFairy.getValue(FlashFairyPP::kNumKeys - 1), 0xCAFE);
  pageIsEmpty(pages[0]);
  pageIsEmpty(pages[1]);
}

TEST_F(VirtualFlashFixture, Store_Load_Store_SingleValue) {
  EXPECT_TRUE(flashFairy.setValue(42, 0xBEEF));
  EXPECT_EQ(flashFairy.getValue(42), 0xBEEF);
  EXPECT_TRUE(flashFairy.setValue(42, 0xAFFE));
  EXPECT_EQ(flashFairy.getValue(42), 0xAFFE);

  // At least on x86_64, the uint32s are stored in LSB.
  EXPECT_EQ(pages[0][0], 0xEF);
  EXPECT_EQ(pages[0][1], 0xBE);
  EXPECT_EQ(pages[0][2], 0x2A);
  EXPECT_EQ(pages[0][3], 0x00);
  EXPECT_EQ(pages[0][4], 0xFE);
  EXPECT_EQ(pages[0][5], 0xAF);
  EXPECT_EQ(pages[0][6], 0x2A);
  EXPECT_EQ(pages[0][7], 0x00);
  EXPECT_EQ(pages[0][8], 0xFF);
  memoryIsEmpty((pages[0]) + 8, FlashFairyPP::Config_t::pageSize - 8);

  pageIsEmpty(pages[1]);
}

TEST_F(VirtualFlashFixture, MultipleValues) {
  EXPECT_TRUE(flashFairy.setValue(42, 0xBEEF));
  EXPECT_TRUE(flashFairy.setValue(0, 0xDEAF));
  EXPECT_TRUE(flashFairy.setValue(1, 0xDEAD));
  EXPECT_TRUE(flashFairy.setValue(254, 0xDEAF));

  EXPECT_EQ(flashFairy.getValue(41), 0xCAFE);
  EXPECT_EQ(flashFairy.getValue(42), 0xBEEF);
  EXPECT_EQ(flashFairy.getValue(43), 0xCAFE);

  EXPECT_EQ(flashFairy.getValue(0), 0xDEAF);
  EXPECT_EQ(flashFairy.getValue(1), 0xDEAD);
  EXPECT_EQ(flashFairy.getValue(2), 0xCAFE);

  EXPECT_EQ(flashFairy.getValue(254), 0xDEAF);

  // Stored four values -> 16 bytes. Remainder should be empty.
  memoryIsEmpty(pages[0] + 16, FlashFairyPP::Config_t::pageSize - 16);

  pageIsEmpty(pages[1]);
}

TEST_F(VirtualFlashFixture, Write_Reset_Load) {
  // Set a bunch of values
  EXPECT_TRUE(flashFairy.setValue(42, 0xBEEF));
  EXPECT_TRUE(flashFairy.setValue(0, 0xDEAF));
  EXPECT_TRUE(flashFairy.setValue(1, 0xDEAD));
  EXPECT_TRUE(flashFairy.setValue(255, 0xDEAF));

  // Verify they were written
  ASSERT_EQ(flashFairy.getValue(42), 0xBEEF);
  ASSERT_EQ(flashFairy.getValue(0), 0xDEAF);
  ASSERT_EQ(flashFairy.getValue(1), 0xDEAD);
  ASSERT_EQ(flashFairy.getValue(255), 0xDEAF);

  // Setup another flashFairy
  FlashFairyPP flashFairy2;
  flashFairy2.Init(config);

  // Read the elements that were previously written plus elements around it.
  EXPECT_EQ(flashFairy2.getValue(41), 0xCAFE);
  EXPECT_EQ(flashFairy2.getValue(42), 0xBEEF);
  EXPECT_EQ(flashFairy2.getValue(43), 0xCAFE);

  EXPECT_EQ(flashFairy2.getValue(0), 0xDEAF);
  EXPECT_EQ(flashFairy2.getValue(1), 0xDEAD);
  EXPECT_EQ(flashFairy2.getValue(2), 0xCAFE);

  EXPECT_EQ(flashFairy2.getValue(255), 0xDEAF);
}

/*
TEST_F(VirtualFlashFixture, BothPagesFull) {
        // TODO: Write to a FlashFairy where copying over the existing data will already fill the entire page.
}
*/

/*

TEST_F(VirtualFlashFixture, WriteSecondPage_Reset_Load) {
  // Set a bunch of values
EXPECT_TRUE(flashFairy.setValue(42, 0xBEEF));
EXPECT_TRUE(flashFairy.setValue(0, 0xDEAF));
EXPECT_TRUE(flashFairy.setValue(1, 0xDEAD));
EXPECT_TRUE(flashFairy.setValue(1010, 0xDEAF));

// Verify they were written
ASSERT_EQ(flashFairy.getValue(42), 0xBEEF);
ASSERT_EQ(flashFairy.getValue(0), 0xDEAF);
ASSERT_EQ(flashFairy.getValue(1), 0xDEAD);
ASSERT_EQ(flashFairy.getValue(1010), 0xDEAF);

// Setup another flashFairy
FlashFairyPP flashFairy2;
flashFairy2.Init(config);

// Read the elements that were previously written plus elements around it.
EXPECT_EQ(flashFairy2.getValue(41), 0xCAFE);
EXPECT_EQ(flashFairy2.getValue(42), 0xBEEF);
EXPECT_EQ(flashFairy2.getValue(43), 0xCAFE);

EXPECT_EQ(flashFairy2.getValue(0), 0xDEAF);
EXPECT_EQ(flashFairy2.getValue(1), 0xDEAD);
EXPECT_EQ(flashFairy2.getValue(2), 0xCAFE);

EXPECT_EQ(flashFairy2.getValue(1010), 0xDEAF);
}
*/

}  // namespace FlashFairyPP
