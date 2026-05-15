#ifndef BLOCK_NAME_LOOKUP_H
#define BLOCK_NAME_LOOKUP_H

#include <string>

#include "core/registers/BlockRegister.h"

// Thin helper that maps a block ID to its human-readable name.
//
// Intentionally kept in its own header (and out of any rendering code) so that
// HUD / UI code never reaches into the block registry directly.  The HUD only
// depends on the std::string returned here, which keeps rendering and the
// block model side cleanly separated.
inline std::string blockNameById(int id) {
    if (id < 0) return std::string();
    return BlockRegister::instance().getBlockByIndex(id).name;
}

#endif
