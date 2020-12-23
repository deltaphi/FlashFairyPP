#include "Mocks.h"

namespace FlashFairyPP {

extern "C" void FlashFairy_Erase_Page(void* pagePtr) { memset(pagePtr, 0xFF, 1024); }
extern "C" void FlashFairy_Write_Word(void* pagePtr, uint32_t line) { *static_cast<uint32_t*>(pagePtr) = line; }
extern "C" void flash_lock() {}
extern "C" void flash_unlock() {}

}  // namespace FlashFairyPP
