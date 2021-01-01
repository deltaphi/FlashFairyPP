#ifndef __FLASHFAIRYPP__BITARRAY_H__
#define __FLASHFAIRYPP__BITARRAY_H__

#include <cstring>

namespace FlashFairyPP {

template <typename StorageElement, StorageElement lengthInBits>
class BitArray {
 public:
  BitArray() { memset(storage_, 0, sizeof(Storage_t)); }

  constexpr static const auto kBitsPerElem = sizeof(StorageElement) * 8;

  using Storage_t = StorageElement[lengthInBits / kBitsPerElem];

  void setBit(const StorageElement bit) { getElem(bit) |= getMask(bit); }

  void clearBit(const StorageElement bit) { getElem(bit) &= ~getMask(bit); }

  bool isSet(const StorageElement bit) { return (getElem(bit) & getMask(bit)) != 0; }

 private:
  Storage_t storage_;

  StorageElement& getElem(StorageElement bit) {
    const auto idx = getIdx(bit);
    return storage_[idx];
  }

  constexpr auto getIdx(StorageElement bit) const {
    const auto idx = bit / kBitsPerElem;
    return idx;
  }

  constexpr StorageElement getMask(StorageElement bit) const {
    const auto idx = getIdx(bit);
    const auto shift = bit - (idx * kBitsPerElem);
    const StorageElement mask = 1 << shift;
    return mask;
  }
};

}  // namespace FlashFairyPP

#endif  // __FLASHFAIRYPP__BITARRAY_H__
