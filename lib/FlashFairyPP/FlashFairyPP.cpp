#include "FlashFairyPP/FlashFairyPP.h"

#include <cstring>

#define GetKey(line) (static_cast<key_type>(line >> sizeof(value_type)))
#define GetValue(line) (static_cast<value_type>(line))

#define SetLine(line, key, value) (line = (static_cast<FlashLine_t>(key) << sizeof(value)) | value)

#define SetKey(line, key) SetLine(line, key, GetValue(line))
#define SetValue(line, value) SetLine(line, GetKey(line), value)

namespace FlashFairyPP {

bool FlashFairyPP::Init(const Config_t& configuration) {
  this->configuration_ = configuration;
  if (isEmptyPage(configuration_.page1)) {
    activePage_ = configuration_.page2;
  } else {
    activePage_ = configuration_.page1;
  }
  BuildTlTable();
  return true;
}

FlashFairyPP::value_type FlashFairyPP::getValue(key_type key) const {
  if (key > kNumKeys) {
    key = kNumKeys;
  }
  if (tlTable_[key] == nullptr) {
    return npos;
  } else {
    return GetKey(*(tlTable_[key]));
  }
}

bool FlashFairyPP::setValue(key_type key, value_type value) {
  FlashLine_t line;
  SetLine(line, key, value);
  page_pointer_type nextFreeLine = findFreeLine(activePage_);
  if (nextFreeLine == nullptr) {
    // Switch pages
    nextFreeLine = SwitchPages(line);
  }
  if (nextFreeLine == nullptr) {
    return false;
  } else {
    tlTable_[key] = nextFreeLine;
    flash_write(nextFreeLine, line);
    return true;
  }
}

bool FlashFairyPP::Format() {
  memset(tlTable_, 0, sizeof(TranslationTable_t));
  flash_erase_page(configuration_.page1);
  flash_erase_page(configuration_.page2);
  return true;
}

FlashFairyPP::page_pointer_type FlashFairyPP::SwitchPages(FlashLine_t updateLine) {
  // Write all translated entries to the inactive page while updating the table
  // entries.
  page_pointer_type inactivePage;
  if (activePage_ == configuration_.page1) {
    inactivePage = configuration_.page2;
  } else {
    inactivePage = configuration_.page1;
  }

  key_type insertionKey = GetKey(updateLine);
  for (key_type i = 0; i < kNumKeys; ++i) {
    if (i == insertionKey) {
      flash_write(inactivePage + i, updateLine);
      tlTable_[insertionKey] = inactivePage + i;
    } else {
      flash_write(inactivePage + i, *tlTable_[i]);
      tlTable_[i] = inactivePage + i;
    }
  }

  // Format the active page
  flash_erase_page(activePage_);

  // swap the pointers.
  activePage_ = inactivePage;

  return activePage_;
}

void FlashFairyPP::BuildTlTable() {
  memset(tlTable_, 0, sizeof(TranslationTable_t));
  for (page_pointer_type i = activePage_; (i < activePage_ + Config_t::pageSize) && (*i != kFreePattern);
       i += sizeof(FlashLine_t) / 8) {
    key_type key = GetKey(*i);
    if (key < kNumKeys) {
      tlTable_[key] = i;
    }
  }
}

FlashFairyPP::page_pointer_type FlashFairyPP::findFreeLine(page_pointer_type page) {
  for (page_pointer_type i = page; (i < page + Config_t::pageSize); i += sizeof(FlashLine_t) / 8) {
    if (*i == kFreePattern) {
      return i;
    }
  }
  return nullptr;
}

}  // namespace FlashFairyPP
