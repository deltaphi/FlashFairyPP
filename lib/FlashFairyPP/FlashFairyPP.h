#ifndef __FLASHFAIRYPP__FLASHFAIRYPP_H__
#define __FLASHFAIRYPP__FLASHFAIRYPP_H__

#include <cstddef>
#include <cstdint>

extern "C" {
void FlashFairy_Erase_Page(void* pagePtr);
void FlashFairy_Write_Word(void* pagePtr, uint32_t line);
void flash_lock();
void flash_unlock();
}

namespace FlashFairyPP {

/*
 * \brief Class FlashFairyPP
 */
class FlashFairyPP {
 public:
  using key_type = uint16_t;
  using value_type = uint16_t;

  using FlashLine_t = uint32_t;
  using page_pointer_type = FlashLine_t*;

  constexpr static const size_t kNumKeys = 256;
  constexpr static const FlashLine_t kFreePattern = 0xFFFFFFFF;
  constexpr static const value_type npos = 0xCAFE;

  constexpr static const std::size_t kPtrLineIncrement = sizeof(FlashLine_t) / 4;
  static_assert(kPtrLineIncrement > 0, "FlashLine_t has insufficient size");

  struct Config_t {
    page_pointer_type pages[2];
    constexpr static const size_t pageSize = 1024;
  };

  /**
   * \brief Initialize the FlashFairy for the given memory area.
   */
  bool Init(const Config_t& configuration);

  /**
   * \brief Read a value from flash storage.
   */
  value_type getValue(const key_type key) const;

  /**
   * \brief Commit a new value to flash storage.
   *
   * If the new value is identical to the old value, do nothing.
   *
   * \return If the value was stored or the value equals the stored value.
   */
  bool setValue(const key_type key, const value_type value);

  /**
   * \brief Sets a value only of the key is found.
   *
   * \return If a value was read from flash, return the new value. Otherwise, return the original value passed to this
   * funciton.
   */
  template <typename V>
  V readValueIfAvailable(const key_type key, V& value) const {
    value_type tmpValue = getValue(key);
    if (tmpValue != npos) {
      value = static_cast<V>(tmpValue);
    }
    return value;
  }

  /**
   * \brief Forcefully clear both flash pages.
   */
  bool Format();

  std::size_t numEntriesLeftOnPage() const;

 private:
  /**
   * Lookup table from key to memory location.
   *
   * Holds as many keys as can be represented distinctively.
   */
  typedef page_pointer_type TranslationTable_t[kNumKeys];

  page_pointer_type activePage_;
  TranslationTable_t tlTable_;
  Config_t configuration_;

  /**
   *	Compacts contents of active page to inactive page.
   * Swaps the active/inactive pointers.
   * Rebuilds the Translation Table.
   *
   * If a new line is passed, that line is used instead of copying the old line.
   * If nothing is to be replaced, pass in kFreePattern.
   *
   * Formats the now inactive page.
   *
   * \return the Address of the first free line in the new page or nullptr, if
   *the new page is full.
   */
  page_pointer_type SwitchPages(FlashLine_t updateLine = kFreePattern);

  /**
   * Iniitalize the TlTable from the active page.
   */
  void BuildTlTable();

  static page_pointer_type findFreeLine(page_pointer_type page);

  static bool isEmptyPage(page_pointer_type page) {
    // A page is empty if its first line is the free patern.
    return *page == kFreePattern;
  }
};

}  // namespace FlashFairyPP

#endif  // __FLASHFAIRYPP__FLASHFAIRYPP_H__
