// Copyright (c) 2017-2025 Fuego Developers
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#include "Currency.h"
#include "../CryptoNoteConfig.h"

namespace CryptoNote {

// Calculate banking fee: 0.1% per active EFier (dynamic rate)
// With 8 active EFiers: 0.8% total. With 3: 0.3%. With 0: 0%.
uint64_t Currency::calculateBankingFee(uint64_t depositAmount, uint32_t activeEfierCount) const {
    if (activeEfierCount == 0) return 0;

    // 0.1% per active EFier = (amount * activeCount * 10) / 10000
    // Simplified: (amount * activeCount) / 1000
    uint64_t fee = (depositAmount * activeEfierCount) / 1000;

    // Ensure minimum fee for small deposits (only if there are active EFiers)
    if (fee == 0 && depositAmount > 0) {
        fee = m_defaultDustThreshold;  // 0.0001 XFG (1000 atomic)
    }

    return fee;
}

} // namespace CryptoNote