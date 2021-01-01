#ifndef __FLASHFAIRYPP__FLASHFAIRYPP_H__
#define __FLASHFAIRYPP__FLASHFAIRYPP_H__

#include <cstddef>
#include <cstdint>

#include "FlashFairyPP/BitArray.h"

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

  class FlashUnlock {
   public:
    FlashUnlock() { flash_unlock(); }
    ~FlashUnlock() { flash_lock(); }
  };

  class SingleElementVisitor {
   public:
    constexpr SingleElementVisitor(const key_type key, const value_type value) : first(key), second(value) {}

    bool contains(const key_type key) const { return key == first; }

    const SingleElementVisitor* begin() const { return this; }

    const SingleElementVisitor* end() const { return this + 1; }

    const key_type first;
    const value_type second;
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
    const value_type tmpValue = getValue(key);
    bool valueAvailable = (tmpValue != npos);
    if (valueAvailable) {
      value = static_cast<V>(tmpValue);
    }
    return valueAvailable;
  }

  /**
   * \brief Reader function that scans through the entire active flash page and calls Visitor for every value that was
   * encountered.
   *
   * Note that Visitor may be called multiple times for a single key - the last call contains the valid value.
   */
  template <class Visitor>
  void visitEntries(Visitor& visitor) const {
    const LinePtr_t pageEnd = activePage_ + linesPerPage();
    for (LinePtr_t linePtr = activePage_; linePtr < pageEnd; linePtr += kPtrLineIncrement) {
      if (!isEmptyLine(*linePtr)) {
        const key_type key = GetKey(*linePtr);
        const value_type value = GetValue(*linePtr);
        visitor(key, value);
      }
    }
  }

  template <class Visitor>
  bool storeVisitor(const Visitor& visitor) {
    bool pageFull = !copyFromVisitorToActivePage(visitor);
    if (pageFull) {
      switchPages(visitor);
      return copyFromVisitorToActivePage(visitor);
    }
    return true;
  }

  /**
   * \brief Forcefully clear both flash pages.
   */
  bool formatFlash();
  std::size_t numEntriesLeftOnActivePage() const;

 private:
  PagePtr_t activePage_;
  Config_t configuration_;

  static key_type GetKey(const FlashLine_t line) { return static_cast<key_type>(line >> (sizeof(value_type) * 8)); }
  static value_type GetValue(const FlashLine_t line) { return static_cast<value_type>(line); }

  static FlashLine_t SetLine(key_type key, value_type value) {
    return (static_cast<FlashLine_t>(key) << (sizeof(value) * 8)) | value;
  }

  template <class Visitor>
  bool copyFromVisitorToActivePage(const Visitor& visitor) {
    LinePtr_t freeLineInFlash = findFreeLine(activePage_);
    const LinePtr_t activePageEnd = getPageEnd(activePage_);

    FlashUnlock unlock;
    for (const auto visitorEntry : visitor) {
      if (freeLineInFlash == nullptr || freeLineInFlash > activePageEnd) {
        return false;
      } else {
        FlashLine_t line = SetLine(visitorEntry.first, visitorEntry.second);
        FlashFairy_Write_Word(freeLineInFlash, line);
        ++freeLineInFlash;
      }
    }
    return true;
  }

  /**
   * Compacts contents of active page to inactive page.
   * Swaps the active/inactive pointers.
   *
   * Does not copy any line that the Visitor claims to contain.
   *
   * Formats the now inactive page.
   *
   * \return the Address of the first free line in the new page or nullptr, if
   *         the new page is full.
   */
  template <class Visitor>
  LinePtr_t switchPages(const Visitor& visitor) {
    const PagePtr_t inactivePage = getInactivePage();

    LinePtr_t freeLine = inactivePage;

    {
      BitArray<uint32_t, kNumKeys> bitArray;
      const LinePtr_t inactivePageEnd = getPageEnd(inactivePage);
      const LinePtr_t activePageEnd = getPageEnd(activePage_);

      FlashUnlock unlock;

      if (!isEmptyPage(inactivePage)) {
        FlashFairy_Erase_Page(inactivePage);
      }

      for (LinePtr_t linePtr = activePageEnd - 1; linePtr >= activePage_ && freeLine < inactivePageEnd; --linePtr) {
        if (!isEmptyLine(*linePtr)) {
          auto lineKey = GetKey(*linePtr);
          if (!bitArray.isSet(lineKey) && !visitor.contains(lineKey)) {
            FlashFairy_Write_Word(freeLine, *linePtr);
            ++freeLine;
          }
          bitArray.setBit(lineKey);
        }
      }

      // Format the active page
      FlashFairy_Erase_Page(activePage_);
    }

    // swap the pointers.
    activePage_ = inactivePage;

    return freeLine;
  }

  static LinePtr_t findFreeLine(PagePtr_t page);

  constexpr static bool isEmptyLine(const FlashLine_t line) { return line == kFreePattern; }

  static bool isEmptyPage(const PagePtr_t page) {
    // A page is empty if its first line is the free patern.
    return isEmptyLine(*page);
  }

  constexpr static std::size_t linesPerPage() { return Config_t::pageSize / sizeof(FlashLine_t); }

  constexpr static LinePtr_t getPageEnd(PagePtr_t page) { return page + linesPerPage(); }

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
