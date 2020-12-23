#include "Mocks.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace FlashFairyPP {

TEST_F(VirtualFlashFixture, Format) {
  EXPECT_THAT(*pages, ::testing::Each(0xFF));
  memset(pages, 0x00, sizeof(pages));
  EXPECT_THAT(*pages, ::testing::Each(0x00));
  flashFairy.Format();
  EXPECT_THAT(*pages, ::testing::Each(0xFF));
}

TEST_F(VirtualFlashFixture, Read_Empty) {
  EXPECT_EQ(flashFairy.getValue(42), 0xCAFE);
  EXPECT_EQ(flashFairy.numEntriesLeftOnPage(), 256);
}

TEST_F(VirtualFlashFixture, Write_Twice) {
  EXPECT_TRUE(flashFairy.setValue(42, 0xBEEF));
  EXPECT_TRUE(flashFairy.setValue(42, 0xBEEF));
  EXPECT_EQ(flashFairy.getValue(42), 0xBEEF);
  memoryIsEmpty((pages[0]) + 4, FlashFairyPP::Config_t::pageSize - 4);
  EXPECT_EQ(flashFairy.numEntriesLeftOnPage(), 255);
}

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
  EXPECT_EQ(flashFairy.numEntriesLeftOnPage(), 256 - 2);

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
  EXPECT_EQ(flashFairy2.numEntriesLeftOnPage(), 256 - 4);
}

TEST_F(VirtualFlashFixture, WriteSecondPage) {
  // Write a single value so often that the memory gets full

  std::size_t numFlashLines = FlashFairyPP::Config_t::pageSize / sizeof(FlashFairyPP::FlashLine_t);

  for (std::size_t i = 0; i < numFlashLines; ++i) {
    EXPECT_TRUE(flashFairy.setValue(i % (FlashFairyPP::kNumKeys / 2), i));
  }

  // pages[0] is now full, pages[1] is still untouched
  pageIsEmpty(pages[1]);

  for (std::size_t i = 0; i < 128; ++i) {
    EXPECT_EQ(flashFairy.getValue(i), i + 128);
  }
  EXPECT_EQ(flashFairy.numEntriesLeftOnPage(), 0);

  // Write another value, will cause an overflow to the second page.
  EXPECT_TRUE(flashFairy.setValue(25, 0x3456));

  for (std::size_t i = 0; i < 128; ++i) {
    if (i == 25) {
      EXPECT_EQ(flashFairy.getValue(i), 0x3456) << "i: " << i;
    } else {
      EXPECT_EQ(flashFairy.getValue(i), i + 128) << "i: " << i;
    }
  }

  // First page is now empty.
  pageIsEmpty(pages[0]);

  // Since we wrote to only half the keys, the second page should be half-filled.
  EXPECT_EQ(flashFairy.numEntriesLeftOnPage(), 128);
  memoryIsEmpty(pages[1] + (128 * 4), 1024 - (128 * 4));

  // Go on to fill the second page (127 entries left)
  for (std::size_t i = 0; i < 128; ++i) {
    EXPECT_TRUE(flashFairy.setValue(i, i));
  }
  EXPECT_EQ(flashFairy.numEntriesLeftOnPage(), 0);

  // Next write should roll over
  EXPECT_TRUE(flashFairy.setValue(25, 0xD017));

  // Second page is now empty.
  pageIsEmpty(pages[1]);

  // Since we wrote to only half the keys, the first page should be half-filled.
  EXPECT_EQ(flashFairy.numEntriesLeftOnPage(), 128);
  memoryIsEmpty(pages[0] + (128 * 4), 1024 - (128 * 4));

  for (std::size_t i = 0; i < 128; ++i) {
    if (i == 25) {
      EXPECT_EQ(flashFairy.getValue(i), 0xD017) << "i: " << i;
    } else {
      EXPECT_EQ(flashFairy.getValue(i), i) << "i: " << i;
    }
  }
}

/*
TEST_F(VirtualFlashFixture, BothPagesFull) {
        // TODO: Write to a FlashFairy where copying over the existing data will already fill the entire page.
}
*/


TEST_F(VirtualFlashFixture, WriteSecondPage_Reset_Load) {
 // Write a single value so often that the memory gets full

  std::size_t numFlashLines = FlashFairyPP::Config_t::pageSize / sizeof(FlashFairyPP::FlashLine_t);

  for (std::size_t i = 0; i < numFlashLines; ++i) {
    EXPECT_TRUE(flashFairy.setValue(i % (FlashFairyPP::kNumKeys / 2), i));
  }

  // pages[0] is now full, pages[1] is still untouched
  pageIsEmpty(pages[1]);

  for (std::size_t i = 0; i < 128; ++i) {
    EXPECT_EQ(flashFairy.getValue(i), i + 128);
  }
  EXPECT_EQ(flashFairy.numEntriesLeftOnPage(), 0);

  // Write another value, will cause an overflow to the second page.
  EXPECT_TRUE(flashFairy.setValue(25, 0x3456));

  // First page is now empty.
  EXPECT_EQ(flashFairy.numEntriesLeftOnPage(), 128);
  pageIsEmpty(pages[0]);

  // Now setup a new flashFairy on the result
  FlashFairyPP flashFairy2;
  flashFairy2.Init(config);

  EXPECT_EQ(flashFairy2.numEntriesLeftOnPage(), 128);
  
  for (std::size_t i = 0; i < 128; ++i) {
    if (i == 25) {
      EXPECT_EQ(flashFairy2.getValue(i), 0x3456) << "i: " << i;
    } else {
      EXPECT_EQ(flashFairy2.getValue(i), i + 128) << "i: " << i;
    }
  }
}


}  // namespace FlashFairyPP
