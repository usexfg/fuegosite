// Copyright (c) 2017-2026 Fuego Developers
// Copyright (c) 2020-2026 Elderfire Privacy Group
//
// This file is part of Fuego.

#include "../../include/FeeEscrowManager.h"
// #include "../Common/StringTools.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <ctime>

namespace CryptoNote {

FeeEscrowManager::FeeEscrowManager(const std::string& dataPath, Logging::ILogger& logger)
    : m_dataPath(dataPath), m_logger(logger, "FeeEscrowManager") {
  // Attempt to load existing escrow data
  if (!load()) {
    m_logger(Logging::DEBUGGING, Logging::BRIGHT_WHITE) << "Starting with empty fee escrow";
  }
}

FeeEscrowManager::~FeeEscrowManager() {
  // Save on destruction
  save();
}

bool FeeEscrowManager::addFeeEscrow(uint64_t epochNumber, uint8_t elderfier_id,
                                   const std::string& elderfierAddress, uint64_t feeAmount) {
  std::unique_lock<std::mutex> lock(m_mutex);

  auto key = std::make_pair(epochNumber, elderfier_id);
  auto it = m_escrowEntries.find(key);

  FeeEscrowEntry entry;
  if (it != m_escrowEntries.end()) {
    // Entry exists - add to existing amount
    entry = it->second;
    entry.feeAmount += feeAmount;
  } else {
    // New entry
    entry.epochNumber = epochNumber;
    entry.elderfier_id = elderfier_id;
    entry.elderfierAddress = elderfierAddress;
    entry.feeAmount = feeAmount;
    entry.timestamp = std::time(nullptr);
    entry.claimed = false;
    entry.claimBlockHeight = 0;
  }

  m_escrowEntries[key] = entry;
  m_totalFeesCollected += feeAmount;
  m_totalFeesDistributed += feeAmount;

  m_logger(Logging::DEBUGGING, Logging::BRIGHT_GREEN)
      << "Added fee escrow: epoch " << epochNumber
      << ", elderfier " << static_cast<int>(elderfier_id)
      << ", amount: " << feeAmount;

  return save();
}

bool FeeEscrowManager::claimFees(uint64_t epochNumber, uint8_t elderfier_id,
                                 uint64_t blockHeight) {
  std::unique_lock<std::mutex> lock(m_mutex);

  auto key = std::make_pair(epochNumber, elderfier_id);
  auto it = m_escrowEntries.find(key);

  if (it == m_escrowEntries.end()) {
    m_logger(Logging::WARNING, Logging::BRIGHT_YELLOW)
        << "No escrow entry found to claim: epoch " << epochNumber
        << ", elderfier " << static_cast<int>(elderfier_id);
    return false;
  }

  if (it->second.claimed) {
    m_logger(Logging::WARNING, Logging::BRIGHT_YELLOW)
        << "Already claimed: epoch " << epochNumber
        << ", elderfier " << static_cast<int>(elderfier_id);
    return false;
  }

  FeeEscrowEntry& entry = it->second;
  entry.claimed = true;
  entry.claimBlockHeight = blockHeight;
  m_totalFeesClaimed += entry.feeAmount;

  m_logger(Logging::DEBUGGING, Logging::BRIGHT_GREEN)
      << "Claimed fees: epoch " << epochNumber
      << ", elderfier " << static_cast<int>(elderfier_id)
      << ", amount: " << entry.feeAmount;

  return save();
}

uint64_t FeeEscrowManager::getUnclaimedFees(uint8_t elderfier_id) const {
  std::unique_lock<std::mutex> lock(m_mutex);

  uint64_t total = 0;
  for (const auto& pair : m_escrowEntries) {
    if (pair.second.elderfier_id == elderfier_id && !pair.second.claimed) {
      total += pair.second.feeAmount;
    }
  }
  return total;
}

uint64_t FeeEscrowManager::getUnclaimedFeesForEpoch(uint64_t epochNumber, uint8_t elderfier_id) const {
  std::unique_lock<std::mutex> lock(m_mutex);

  auto key = std::make_pair(epochNumber, elderfier_id);
  auto it = m_escrowEntries.find(key);

  if (it == m_escrowEntries.end() || it->second.claimed) {
    return 0;
  }

  return it->second.feeAmount;
}

uint64_t FeeEscrowManager::getTotalFeeEscrow() const {
  std::unique_lock<std::mutex> lock(m_mutex);

  uint64_t total = 0;
  for (const auto& pair : m_escrowEntries) {
    if (!pair.second.claimed) {
      total += pair.second.feeAmount;
    }
  }
  return total;
}

uint64_t FeeEscrowManager::getTotalFeesClaimed() const {
  std::unique_lock<std::mutex> lock(m_mutex);
  return m_totalFeesClaimed;
}

std::vector<FeeEscrowEntry> FeeEscrowManager::getElderfierEscrowHistory(uint8_t elderfier_id) const {
  std::unique_lock<std::mutex> lock(m_mutex);

  std::vector<FeeEscrowEntry> result;
  for (const auto& pair : m_escrowEntries) {
    if (pair.second.elderfier_id == elderfier_id) {
      result.push_back(pair.second);
    }
  }

  // Sort by epoch number (oldest first)
  std::sort(result.begin(), result.end(),
            [](const FeeEscrowEntry& a, const FeeEscrowEntry& b) {
              return a.epochNumber < b.epochNumber;
            });

  return result;
}

std::vector<FeeEscrowEntry> FeeEscrowManager::getEpochEscrowEntries(uint64_t epochNumber) const {
  std::unique_lock<std::mutex> lock(m_mutex);

  std::vector<FeeEscrowEntry> result;
  for (const auto& pair : m_escrowEntries) {
    if (pair.second.epochNumber == epochNumber) {
      result.push_back(pair.second);
    }
  }

  // Sort by elderfier ID
  std::sort(result.begin(), result.end(),
            [](const FeeEscrowEntry& a, const FeeEscrowEntry& b) {
              return a.elderfier_id < b.elderfier_id;
            });

  return result;
}

FeeEscrowStats FeeEscrowManager::getStats() const {
  std::unique_lock<std::mutex> lock(m_mutex);

  FeeEscrowStats stats;
  stats.totalFeesCollected = m_totalFeesCollected;
  stats.totalFeesDistributed = m_totalFeesDistributed;
  stats.totalFeesClaimed = m_totalFeesClaimed;
  stats.pendingFeesInEscrow = getTotalFeeEscrow();

  // Count unique epochs
  std::set<uint64_t> uniqueEpochs;
  std::set<uint8_t> activeElderfiers;
  for (const auto& pair : m_escrowEntries) {
    uniqueEpochs.insert(pair.second.epochNumber);
    if (!pair.second.claimed) {
      activeElderfiers.insert(pair.second.elderfier_id);
    }
  }

  stats.totalActiveEpochs = uniqueEpochs.size();
  stats.activeElderfierCount = activeElderfiers.size();

  return stats;
}

std::string FeeEscrowManager::getEscrowDatabasePath() const {
  return m_dataPath + "/fee_escrow.db";
}

bool FeeEscrowManager::serializeEntry(const FeeEscrowEntry& entry, std::string& serialized) const {
  std::stringstream ss;
  ss << entry.epochNumber << "|"
     << static_cast<int>(entry.elderfier_id) << "|"
     << entry.elderfierAddress << "|"
     << entry.feeAmount << "|"
     << entry.timestamp << "|"
     << (entry.claimed ? 1 : 0) << "|"
     << entry.claimBlockHeight;

  serialized = ss.str();
  return true;
}

bool FeeEscrowManager::deserializeEntry(const std::string& serialized, FeeEscrowEntry& entry) const {
  std::stringstream ss(serialized);
  std::string token;
  int index = 0;
  int claimed_int = 0;

  try {
    while (std::getline(ss, token, '|')) {
      switch (index) {
        case 0:
          entry.epochNumber = std::stoull(token);
          break;
        case 1:
          entry.elderfier_id = static_cast<uint8_t>(std::stoi(token));
          break;
        case 2:
          entry.elderfierAddress = token;
          break;
        case 3:
          entry.feeAmount = std::stoull(token);
          break;
        case 4:
          entry.timestamp = std::stoull(token);
          break;
        case 5:
          claimed_int = std::stoi(token);
          entry.claimed = (claimed_int == 1);
          break;
        case 6:
          entry.claimBlockHeight = std::stoull(token);
          break;
        default:
          return false;
      }
      index++;
    }
    return index == 7;
  } catch (const std::exception& e) {
    m_logger(Logging::ERROR, Logging::BRIGHT_RED) << "Error deserializing entry: " << e.what();
    return false;
  }
}

bool FeeEscrowManager::save() {
  std::unique_lock<std::mutex> lock(m_mutex);

  std::string dbPath = getEscrowDatabasePath();
  std::ofstream file(dbPath);

  if (!file.is_open()) {
    m_logger(Logging::ERROR, Logging::BRIGHT_RED) << "Failed to open fee escrow database for writing: " << dbPath;
    return false;
  }

  try {
    // Write header with statistics
    file << "# Fee Escrow Database\n";
    file << "# Total Collected: " << m_totalFeesCollected << "\n";
    file << "# Total Distributed: " << m_totalFeesDistributed << "\n";
    file << "# Total Claimed: " << m_totalFeesClaimed << "\n";
    file << "\n";

    // Write entries
    for (const auto& pair : m_escrowEntries) {
      std::string serialized;
      if (serializeEntry(pair.second, serialized)) {
        file << serialized << "\n";
      }
    }

    file.close();

    m_logger(Logging::DEBUGGING, Logging::BRIGHT_GREEN)
        << "Fee escrow saved: " << m_escrowEntries.size() << " entries";

    return true;
  } catch (const std::exception& e) {
    m_logger(Logging::ERROR, Logging::BRIGHT_RED) << "Error saving fee escrow: " << e.what();
    return false;
  }
}

bool FeeEscrowManager::load() {
  std::unique_lock<std::mutex> lock(m_mutex);

  std::string dbPath = getEscrowDatabasePath();
  std::ifstream file(dbPath);

  if (!file.is_open()) {
    m_logger(Logging::DEBUGGING, Logging::BRIGHT_WHITE) << "Fee escrow database not found: " << dbPath;
    return false;  // Not an error - just first time
  }

  try {
    std::string line;
    int entriesLoaded = 0;

    while (std::getline(file, line)) {
      // Skip comments and empty lines
      if (line.empty() || line[0] == '#') {
        continue;
      }

      FeeEscrowEntry entry;
      if (deserializeEntry(line, entry)) {
        auto key = std::make_pair(entry.epochNumber, entry.elderfier_id);
        m_escrowEntries[key] = entry;
        entriesLoaded++;
      }
    }

    file.close();

    // Recalculate statistics
    m_totalFeesClaimed = 0;
    m_totalFeesDistributed = 0;
    m_totalFeesCollected = 0;

    for (const auto& pair : m_escrowEntries) {
      m_totalFeesCollected += pair.second.feeAmount;
      m_totalFeesDistributed += pair.second.feeAmount;
      if (pair.second.claimed) {
        m_totalFeesClaimed += pair.second.feeAmount;
      }
    }

    m_logger(Logging::DEBUGGING, Logging::BRIGHT_GREEN)
        << "Fee escrow loaded: " << entriesLoaded << " entries";

    return true;
  } catch (const std::exception& e) {
    m_logger(Logging::ERROR, Logging::BRIGHT_RED) << "Error loading fee escrow: " << e.what();
    return false;
  }
}

bool FeeEscrowManager::clear() {
  std::unique_lock<std::mutex> lock(m_mutex);

  m_escrowEntries.clear();
  m_totalFeesCollected = 0;
  m_totalFeesDistributed = 0;
  m_totalFeesClaimed = 0;

  m_logger(Logging::DEBUGGING, Logging::BRIGHT_GREEN) << "Fee escrow cleared";

  return save();
}

}  // namespace CryptoNote
