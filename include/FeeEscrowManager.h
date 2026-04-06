// Copyright (c) 2017-2026 Fuego Developers
// Copyright (c) 2020-2026 Elderfire Privacy Group
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful- but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You are encouraged to redistribute it and/or modify it
// under the terms of the GNU General Public License v3 or later
// versions as published by the Free Software Foundation.
// You should receive a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>

#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <cstdint>
#include <memory>
#include "../src/Logging/ILogger.h"
#include "../src/Logging/LoggerRef.h"

namespace CryptoNote {

// Fee escrow entry for a single elderfier in an epoch
struct FeeEscrowEntry {
  uint64_t epochNumber;
  uint8_t elderfier_id;  // EFiD (0-255)
  std::string elderfierAddress;  // Wallet address for claiming
  uint64_t feeAmount;  // Total fees earned in this epoch
  uint64_t timestamp;  // When this entry was recorded
  bool claimed;  // Whether fees have been claimed
  uint64_t claimBlockHeight;  // Block height when claimed (0 if not claimed)
};

// Fee escrow statistics
struct FeeEscrowStats {
  uint64_t totalFeesCollected;  // All-time total fees collected
  uint64_t totalFeesDistributed;  // All-time total fees distributed to elderfiers
  uint64_t totalFeesClaimed;  // Total fees actually claimed by elderfiers
  uint64_t pendingFeesInEscrow;  // Fees waiting to be claimed
  uint64_t totalActiveEpochs;  // Number of completed epochs
  uint64_t activeElderfierCount;  // Current number of active elderfiers
};

// Fee escrow manager - handles persistence of fee distributions
class IFeeEscrowManager {
public:
  virtual ~IFeeEscrowManager() = default;

  // Add fees for an elderfier in a specific epoch
  virtual bool addFeeEscrow(uint64_t epochNumber, uint8_t elderfier_id,
                            const std::string& elderfierAddress, uint64_t feeAmount) = 0;

  // Claim fees for an elderfier
  virtual bool claimFees(uint64_t epochNumber, uint8_t elderfier_id,
                         uint64_t blockHeight) = 0;

  // Get unclaimed fees for an elderfier
  virtual uint64_t getUnclaimedFees(uint8_t elderfier_id) const = 0;

  // Get unclaimed fees for specific epoch
  virtual uint64_t getUnclaimedFeesForEpoch(uint64_t epochNumber, uint8_t elderfier_id) const = 0;

  // Get total fees in escrow (all epochs, unclaimed)
  virtual uint64_t getTotalFeeEscrow() const = 0;

  // Get fees claimed
  virtual uint64_t getTotalFeesClaimed() const = 0;

  // Get all escrow entries for an elderfier
  virtual std::vector<FeeEscrowEntry> getElderfierEscrowHistory(uint8_t elderfier_id) const = 0;

  // Get all escrow entries for an epoch
  virtual std::vector<FeeEscrowEntry> getEpochEscrowEntries(uint64_t epochNumber) const = 0;

  // Get statistics
  virtual FeeEscrowStats getStats() const = 0;

  // Persistence
  virtual bool save() = 0;
  virtual bool load() = 0;
  virtual bool clear() = 0;
};

class FeeEscrowManager : public IFeeEscrowManager {
public:
  FeeEscrowManager(const std::string& dataPath, Logging::ILogger& logger);
  ~FeeEscrowManager() override;

  // Add fees for an elderfier in a specific epoch
  bool addFeeEscrow(uint64_t epochNumber, uint8_t elderfier_id,
                    const std::string& elderfierAddress, uint64_t feeAmount) override;

  // Claim fees for an elderfier
  bool claimFees(uint64_t epochNumber, uint8_t elderfier_id,
                 uint64_t blockHeight) override;

  // Get unclaimed fees for an elderfier
  uint64_t getUnclaimedFees(uint8_t elderfier_id) const override;

  // Get unclaimed fees for specific epoch
  uint64_t getUnclaimedFeesForEpoch(uint64_t epochNumber, uint8_t elderfier_id) const override;

  // Get total fees in escrow (all epochs, unclaimed)
  uint64_t getTotalFeeEscrow() const override;

  // Get fees claimed
  uint64_t getTotalFeesClaimed() const override;

  // Get all escrow entries for an elderfier
  std::vector<FeeEscrowEntry> getElderfierEscrowHistory(uint8_t elderfier_id) const override;

  // Get all escrow entries for an epoch
  std::vector<FeeEscrowEntry> getEpochEscrowEntries(uint64_t epochNumber) const override;

  // Get statistics
  FeeEscrowStats getStats() const override;

  // Persistence
  bool save() override;
  bool load() override;
  bool clear() override;

private:
  std::string m_dataPath;
  mutable Logging::LoggerRef m_logger;
  mutable std::mutex m_mutex;

  // In-memory storage: (epochNumber, elderfier_id) -> FeeEscrowEntry
  std::map<std::pair<uint64_t, uint8_t>, FeeEscrowEntry> m_escrowEntries;

  // Statistics tracking
  uint64_t m_totalFeesCollected = 0;
  uint64_t m_totalFeesDistributed = 0;
  uint64_t m_totalFeesClaimed = 0;

  // Helper methods
  std::string getEscrowDatabasePath() const;
  bool serializeEntry(const FeeEscrowEntry& entry, std::string& serialized) const;
  bool deserializeEntry(const std::string& serialized, FeeEscrowEntry& entry) const;
};

}  // namespace CryptoNote
