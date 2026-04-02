// Copyright (c) 2017-2026 Fuego Developers
// Copyright (c) 2020-2026 Elderfire Privacy Group
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

#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <queue>
#include <cstdint>
#include <vector>
#include <map>
#include "crypto/crypto.h"
#include "Logging/ILogger.h"
#include "Logging/LoggerRef.h"

namespace CryptoNote {

// Forward declarations
class ICore;
class IEldernodeIndexManager;
class IP2pEndpoint;

// Elderfier signature daemon - generates and broadcasts signatures
class ElderfierSignatureDaemon {
public:
  ElderfierSignatureDaemon(
    ICore& core,
    IEldernodeIndexManager& eldernodeIndex,
    IP2pEndpoint* p2pEndpoint,
    Logging::ILogger& logger);

  ~ElderfierSignatureDaemon();

  // Lifecycle management
  void start();
  void stop();
  bool isRunning() const;

  // Configuration
  void setSigningEnabled(bool enabled);
  void setElderfierIds(const std::vector<uint8_t>& elfIds);
  void addElderfier(uint8_t elderfier_id);
  void removeElderfier(uint8_t elderfier_id);
  void setSigningKeys(const Crypto::PublicKey& pub, const Crypto::SecretKey& sec);
  void setElderfierPublicKey(uint8_t efid, const Crypto::PublicKey& pubkey);

  // Statistics
  uint64_t getSignaturesGenerated() const;
  uint64_t getSignaturesFailed() const;
  uint64_t getLastSignatureHeight() const;
  uint64_t getLastSignatureTime() const;

  // Trigger signature generation for current block
  void onNewBlock(uint32_t blockHeight, const Crypto::Hash& merkleRoot);

  // Check if this elderfier should sign at this height
  bool shouldSign(uint8_t elderfier_id, uint32_t blockHeight) const;

private:
  // Main daemon thread
  void daemonThread();

  // Core signing logic
  bool generateAndBroadcastSignature(uint8_t elderfier_id, uint32_t blockHeight,
                                     const Crypto::Hash& merkleRoot);

  // Helper methods
  bool validateElderfierRegistration(uint8_t elderfier_id);
  Crypto::Signature signMerkleRoot(const Crypto::Hash& merkleRoot);
  bool broadcastSignature(uint8_t elderfier_id, uint32_t blockHeight,
                          const Crypto::Hash& merkleRoot,
                          const Crypto::Signature& signature);

  // Configuration
  ICore& m_core;
  IEldernodeIndexManager& m_eldernodeIndex;
  IP2pEndpoint* m_p2pEndpoint;
  mutable Logging::LoggerRef m_logger;

  // Thread management
  std::thread m_daemonThread;
  std::atomic<bool> m_running{false};
  std::atomic<bool> m_signingEnabled{true};

  // Synchronization
  mutable std::mutex m_mutex;

  // Persistent signing keys (used for all elderfier signatures from this node)
  Crypto::PublicKey m_signingPublicKey;
  Crypto::SecretKey m_signingSecretKey;
  bool m_hasSigningKeys = false;

  // Elderfier ID -> public key mapping (for validation lookups)
  std::map<uint8_t, Crypto::PublicKey> m_elderfierPubkeys;

  // Registered elderfiers to sign for
  std::vector<uint8_t> m_elderfierIds;

  // Pending block heights for signature generation
  std::queue<std::pair<uint32_t, Crypto::Hash>> m_pendingBlocks;

  // Statistics
  std::atomic<uint64_t> m_signaturesGenerated{0};
  std::atomic<uint64_t> m_signaturesFailed{0};
  std::atomic<uint32_t> m_lastSignatureHeight{0};
  std::atomic<uint64_t> m_lastSignatureTime{0};

  // Configuration constants
  static constexpr uint32_t SIGNATURE_GENERATION_DELAY_BLOCKS = 2;  // Generate sig after 2 blocks
  static constexpr uint64_t SIGNATURE_GENERATION_INTERVAL_MS = 1000;  // Check every 1 second
  static constexpr uint64_t BLOCK_TIME_ESTIMATE_MS = 120000;  // 2 minute estimated block time
};

}  // namespace CryptoNote
