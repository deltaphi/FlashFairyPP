#include "FlashFairyPP/FlashFairyPP.h"

#include <cstring>

namespace FlashFairyPP {

const FlashFairyPP::value_type FlashFairyPP::npos;

bool FlashFairyPP::initialize(const Config_t& configuration) {
  this->configuration_ = configuration;
  if (isEmptyPage(configuration_.pages[1])) {
    activePage_ = configuration_.pages[0];
  } else {
    activePage_ = configuration_.pages[1];
  }
  return true;
}

FlashFairyPP::value_type FlashFairyPP::getValue(const key_type key) const {
  FlashFairyPP::value_type result = npos;
  auto visitor = [key, &result](auto flash_key, auto value) {
    if (flash_key == key) {
      result = value;
    }
  };
  visitEntries(visitor);
  return result;
}

bool FlashFairyPP::setValue(const key_type key, const value_type value) {
  if (key >= kNumKeys) {
    return false;
  } else if (value == getValue(key)) {
    return true;
  } else {
    const SingleElementVisitor visitor(key, value);
    return storeVisitor(visitor);
  }
}

bool FlashFairyPP::formatFlash() {
  FlashUnlock unlock;
  FlashFairy_Erase_Page(configuration_.pages[0]);
  FlashFairy_Erase_Page(configuration_.pages[1]);
  return true;
}

FlashFairyPP::LinePtr_t FlashFairyPP::findFreeLine(PagePtr_t page) {
  for (std::size_t i = 0; i < linesPerPage(); i += kPtrLineIncrement) {
    if (page[i] == kFreePattern) {
      return &page[i];
    }
  }
  return nullptr;
}

std::size_t FlashFairyPP::numEntriesLeftOnActivePage() const {
  PagePtr_t nextFreeLine = findFreeLine(activePage_);
  if (nextFreeLine == nullptr) {
    return 0;
  } else {
    std::size_t usedEntries = (nextFreeLine - activePage_) / kPtrLineIncrement;
    std::size_t availableEntries = linesPerPage() - usedEntries;
    return availableEntries;
  }
}

}  // namespace FlashFairyPP
