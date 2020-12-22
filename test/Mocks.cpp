#include "Mocks.h"

namespace FlashFairyPP {

extern "C" void flash_erase_page(uint32_t *pagePtr) { memset(pagePtr, 0xFF, 1024); }
extern "C" void flash_write(uint32_t *pagePtr, uint32_t line) { *pagePtr = line; }

}  // namespace FlashFairyPP
