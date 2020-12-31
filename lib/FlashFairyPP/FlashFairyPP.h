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
  using PagePtr_t = FlashLine_t*;
  using LinePtr_t = FlashLine_t*;

  constexpr static const size_t kNumKeys = 256;
  constexpr static const FlashLine_t kFreePattern = 0xFFFFFFFF;
  constexpr static const value_type npos = 0xCAFE;

  constexpr static const std::size_t kPtrLineIncrement = sizeof(FlashLine_t) / 4;
  static_assert(kPtrLineIncrement > 0, "FlashLine_t has insufficient size");

  struct Config_t {
    PagePtr_t pages[2];
    constexpr static const size_t pageSize = 1024;
  };

  /**
   * \brief Initialize the FlashFairy for the given memory area.
   */
  bool initialize(const Config_t& configuration);

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
  bool readValueIfAvailable(const key_type key, V& value) const {
    value_type tmpValue = getValue(key);
    bool valueAvailable = (tmpValue != npos);
    if (valueAvailable) {
      value = static_cast<V>(tmpValue);
    }
    return valueAvailable;
  }

  /**
   * \brief Forcefully clear both flash pages.
   */
  bool formatFlash();

  std::size_t numEntriesLeftOnActivePage() const;

 private:
  /**
   * Lookup table from key to memory location.
   *
   * Holds as many keys as can be represented distinctively.
   */
  typedef LinePtr_t TranslationTable_t[kNumKeys];

  PagePtr_t activePage_;
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
  PagePtr_t switchPages(FlashLine_t updateLine = kFreePattern);

  /**
   * Iniitalize the TlTable from the active page.
   */
  void buildTranslationTable();

  static LinePtr_t findFreeLine(PagePtr_t page);

  static bool isEmptyLine(const FlashLine_t line) { return line == kFreePattern; }

  static bool isEmptyPage(const PagePtr_t page) {
    // A page is empty if its first line is the free patern.
    return isEmptyLine(*page);
  }

  constexpr static std::size_t linesPerPage() { return Config_t::pageSize / sizeof(FlashLine_t); }

  PagePtr_t getInactivePage() {
    if (activePage_ == configuration_.pages[0]) {
      return configuration_.pages[1];
    } else {
      return configuration_.pages[0];
    }
  }
};

}  // namespace FlashFairyPP

#endif  // __FLASHFAIRYPP__FLASHFAIRYPP_H__
