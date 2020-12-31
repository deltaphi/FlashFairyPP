#ifndef __MOCKS_H__
#define __MOCKS_H__

#include "FlashFairyPP/FlashFairyPP.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace FlashFairyPP {

class VirtualFlashFixture : public ::testing::Test {
 public:
  void SetUp() {
    memset(pages, 0xFF, sizeof(pages));
    config.pages[0] = reinterpret_cast<uint32_t*>(&pages[0]);
    config.pages[1] = reinterpret_cast<uint32_t*>(&pages[1]);
    flashFairy.initialize(config);
  }

  void TearDown() {}

  using PageType = uint8_t[1024];

  PageType pages[2];
  FlashFairyPP::Config_t config;

  FlashFairyPP flashFairy;

  static void pageIsEmpty(uint8_t* page) { memoryIsEmpty(page, FlashFairyPP::Config_t::pageSize); }

  static void memoryIsEmpty(uint8_t* page, std::size_t len) {
    for (std::size_t i = 0; i < len; ++i) {
      EXPECT_EQ(page[i], 0xFF) << "Page " << page << ", Byte " << i;
    }
  }
};

}  // namespace FlashFairyPP

#endif  // __MOCKS_H__
