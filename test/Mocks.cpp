#include "Mocks.h"

namespace FlashFairyPP {

extern "C" void flash_erase_page(uint32_t *pagePtr) {}
extern "C" void flash_write(uint32_t *pagePtr, uint32_t line) {}

}  // namespace FlashFairyPP
