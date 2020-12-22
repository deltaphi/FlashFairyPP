#include "Mocks.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace FlashFairyPP {

TEST_F(VirtualFlashFixture, Read_Empty) { EXPECT_EQ(flashFairy.getValue(42), 0xCAFE); }

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

/*
TEST_F(VirtualFlashFixture, BothPagesFull) {
        // TODO: Write to a FlashFairy where copying over the existing data will already fill the entire page.
}
*/

}  // namespace FlashFairyPP
