#include "FlashFairyPP/FlashFairyPP.h"

#include <cstring>

#define GetKey(line) (static_cast<key_type>(line >> (sizeof(value_type) * 8)))
#define GetValue(line) (static_cast<value_type>(line))

#define SetLine(line, key, value) (line = (static_cast<FlashLine_t>(key) << (sizeof(value) * 8)) | value)

#define SetKey(line, key) SetLine(line, key, GetValue(line))
#define SetValue(line, value) SetLine(line, GetKey(line), value)

namespace FlashFairyPP {

const FlashFairyPP::value_type FlashFairyPP::npos;

bool FlashFairyPP::initialize(const Config_t& configuration) {
  this->configuration_ = configuration;
  if (isEmptyPage(configuration_.pages[1])) {
    activePage_ = configuration_.pages[0];
  } else {
    activePage_ = configuration_.pages[1];
  }
  buildTranslationTable();
  return true;
}

FlashFairyPP::value_type FlashFairyPP::getValue(const key_type key) const {
  FlashFairyPP::value_type result = npos;
  if (key < kNumKeys && tlTable_[key] != nullptr) {
    result = GetValue(*(tlTable_[key]));
  }
  return result;
}

bool FlashFairyPP::setValue(const key_type key, const value_type value) {
  if (key >= kNumKeys) {
    return false;
  } else if (getValue(key) == value) {
    return true;
  } else {
    FlashLine_t line;
    SetLine(line, key, value);
    LinePtr_t freeLineInFlash = findFreeLine(activePage_);
    if (freeLineInFlash == nullptr) {
      switchPages(line);
      return true;
    } else {
      tlTable_[key] = freeLineInFlash;
      flash_unlock();
      FlashFairy_Write_Word(freeLineInFlash, line);
      flash_lock();
      return true;
    }
  }
}

bool FlashFairyPP::formatFlash() {
  memset(tlTable_, 0, sizeof(TranslationTable_t));
  flash_unlock();
  FlashFairy_Erase_Page(configuration_.pages[0]);
  FlashFairy_Erase_Page(configuration_.pages[1]);
  flash_lock();
  return true;
}

FlashFairyPP::PagePtr_t FlashFairyPP::switchPages(FlashLine_t updateLine) {
  // Write all translated entries to the inactive page while updating the table
  // entries.
  PagePtr_t inactivePage = getInactivePage();

  key_type insertionKey = GetKey(updateLine);
  std::size_t writeIdx = 0;

  flash_unlock();

  for (key_type i = 0; i < kNumKeys; ++i) {
    if (i == insertionKey) {
      FlashFairy_Write_Word(inactivePage + writeIdx, updateLine);
      tlTable_[insertionKey] = inactivePage + writeIdx;
      ++writeIdx;
    } else {
      if (tlTable_[i] != nullptr) {
        FlashFairy_Write_Word(inactivePage + writeIdx, *tlTable_[i]);
        tlTable_[i] = inactivePage + writeIdx;
        ++writeIdx;
      }
    }
  }

  // Format the active page
  FlashFairy_Erase_Page(activePage_);

  flash_lock();

  // swap the pointers.
  activePage_ = inactivePage;

  return activePage_;
}

void FlashFairyPP::buildTranslationTable() {
  memset(tlTable_, 0, sizeof(TranslationTable_t));
  for (std::size_t i = 0; (i < linesPerPage()) && !isEmptyLine(activePage_[i]); i += kPtrLineIncrement) {
    key_type key = GetKey(*(activePage_ + i));
    if (key < kNumKeys) {
      tlTable_[key] = activePage_ + i;
    }
  }
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
