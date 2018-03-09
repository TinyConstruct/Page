#include "aro_generic.h"

void reportAssertionFailure(char* expr, char* file, int line) {
  //TODO: actually OutputDebugString() this? Not really needed as of writing
  return;
}

uint32_t safeTruncateUInt64(uint64_t value) {
    assert(value <= 0xFFFFFFFF);
    uint32_t result = (uint32_t)value;
    return(result);
}