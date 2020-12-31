#include "FlashFairyPP/FlashFairyPP.h"

#include <cstring>

#define GetKey(line) (static_cast<key_type>(line >> (sizeof(value_type) * 8)))
#define GetValue(line) (static_cast<value_type>(line))

#define SetLine(line, key, value) (line = (static_cast<FlashLine_t>(key) << (sizeof(value) * 8)) | value)

#define SetKey(line, key) SetLine(line, key, GetValue(line))
#define SetValue(line, value) SetLine(line, GetKey(line), value)

namespace FlashFairyPP {

const FlashFairyPP::value_type FlashFairyPP::npos;

bool FlashFairyPP::Init(const Config_t& configuration) {
  this->configuration_ = configuration;
  if (isEmptyPage(configuration_.pages[1])) {
    activePage_ = configuration_.pages[0];
  } else {
    activePage_ = configuration_.pages[1];
  }
  BuildTlTable();
  return true;
}

FlashFairyPP::value_type FlashFairyPP::getValue(const key_type key) const {
  if (key >= kNumKeys) {
    return npos;
  } else {
    if (tlTable_[key] == nullptr) {
      return npos;
    } else {
      value_type value = GetValue(*(tlTable_[key]));
      return value;
    }
  }
}

bool FlashFairyPP::setValue(const key_type key, const value_type value) {
  if (key >= kNumKeys) {
    return false;
  } else if (getValue(key) == value) {
    return true;
  } else {
    FlashLine_t line;
    SetLine(line, key, value);
    page_pointer_type nextFreeLine = findFreeLine(activePage_);
    if (nextFreeLine == nullptr) {
      // Switch pages
      SwitchPages(line);
      return true;
    } else {
      if (nextFreeLine == nullptr) {
        return false;
      } else {
        tlTable_[key] = nextFreeLine;
        flash_unlock();
        FlashFairy_Write_Word(nextFreeLine, line);
        flash_lock();
        return true;
      }
    }
  }
}

bool FlashFairyPP::Format() {
  memset(tlTable_, 0, sizeof(TranslationTable_t));
  flash_unlock();
  FlashFairy_Erase_Page(configuration_.pages[0]);
  FlashFairy_Erase_Page(configuration_.pages[1]);
  flash_lock();
  return true;
}

FlashFairyPP::page_pointer_type FlashFairyPP::SwitchPages(FlashLine_t updateLine) {
  // Write all translated entries to the inactive page while updating the table
  // entries.
  page_pointer_type inactivePage;
  if (activePage_ == configuration_.pages[0]) {
    inactivePage = configuration_.pages[1];
  } else {
    inactivePage = configuration_.pages[0];
  }

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

void FlashFairyPP::BuildTlTable() {
  memset(tlTable_, 0, sizeof(TranslationTable_t));
  for (std::size_t i = 0; (i < Config_t::pageSize / 4) && (*(activePage_ + i) != kFreePattern);
       i += kPtrLineIncrement) {
    key_type key = GetKey(*(activePage_ + i));
    if (key < kNumKeys) {
      tlTable_[key] = activePage_ + i;
    }
  }
}

FlashFairyPP::page_pointer_type FlashFairyPP::findFreeLine(page_pointer_type page) {
  for (std::size_t i = 0; (i < Config_t::pageSize / 4); i += kPtrLineIncrement) {
    FlashLine_t line = *(page + i);
    if (line == kFreePattern) {
      return page + i;
    }
  }
  return nullptr;
}

std::size_t FlashFairyPP::numEntriesLeftOnPage() const {
  page_pointer_type nextFreeLine = findFreeLine(activePage_);
  if (nextFreeLine == nullptr) {
    return 0;
  } else {
    std::size_t usedEntries = (nextFreeLine - activePage_);
    std::size_t totalEntries = (Config_t::pageSize / sizeof(FlashLine_t));
    std::size_t availableEntries = totalEntries - usedEntries;
    return availableEntries;
  }
}

}  // namespace FlashFairyPP
