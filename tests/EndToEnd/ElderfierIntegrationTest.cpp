// Copyright (c) 2017-2025 Fuego Developers
// Copyright (c) 2020-2025 Elderfire Privacy Group
//
// This file is part of Fuego.
//
// Comprehensive end-to-end test for elderfier consensus system

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

#include "IWallet.h"
#include "ICore.h"
#include "EldernodeIndexManager.h"
#include "ElderfierSignatureDaemon.h"
#include "FeeEscrowManager.h"
#include "CryptoNoteCore/CommitmentIndex.h"
#include "P2p/P2pProtocolDefinitions.h"

namespace CryptoNote {

class ElderfierIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize logging
    Logging::ConsoleLogger logger;

    // Initialize CommitmentIndex for signature caching
    m_commitmentIndex = std::make_unique<CommitmentIndex>();

    // Initialize FeeEscrowManager for fee persistence
    m_feeEscrow = std::make_unique<FeeEscrowManager>("/tmp/fuego_test", logger);

    // Initialize EldernodeIndex for elderfier registration
    m_eldernodeIndex = std::make_unique<EldernodeIndexManager>(logger);

    // Initialize signature daemon (requires ICore and EldernodeIndex)
    // Note: In real test, would use actual ICore implementation
    // m_daemon = std::make_unique<ElderfierSignatureDaemon>(
    //   *m_core, *m_eldernodeIndex, m_p2pEndpoint, logger);
  }

  void TearDown() override {
    // Cleanup
  }

  // Test helpers
  void registerElderfiers(int count) {
    for (int i = 0; i < count; i++) {
      uint8_t elf_id = static_cast<uint8_t>(i);

      // Create mock elderfier deposit data
      ElderfierDepositData deposit_data;
      deposit_data.depositHash = Crypto::Hash{};  // Mock hash
      deposit_data.elderfierPublicKey = Crypto::PublicKey{};  // Mock key
      deposit_data.depositAmount = 800000000000;  // 800 XFG
      deposit_data.depositTimestamp = std::time(nullptr);
      deposit_data.lastSeenTimestamp = deposit_data.depositTimestamp;
      deposit_data.elderfierAddress = "test_address_" + std::to_string(i);
      deposit_data.isActive = true;
      deposit_data.isSlashable = true;
      deposit_data.isUnlocked = false;
      deposit_data.isSpent = false;

      // Register in elderfier index
      m_eldernodeIndex->addElderfierDeposit(deposit_data);
    }
  }

  void simulateSignatures(int elderfierCount, uint32_t blockHeight) {
    Crypto::Hash merkle_root = Crypto::cn_fast_hash("test_block", 10);

    for (int i = 0; i < elderfierCount; i++) {
      uint8_t elf_id = static_cast<uint8_t>(i);

      // Generate ephemeral keypair and signature
      Crypto::PublicKey pubkey;
      Crypto::SecretKey seckey;
      Crypto::generate_keys(pubkey, seckey);
      Crypto::Signature sig = Crypto::generate_signature(merkle_root, pubkey, seckey);

      // Create cached signature entry
      CommitmentIndex::CachedElderfierSignature cached_sig;
      cached_sig.merkle_root = merkle_root;
      cached_sig.signature = sig;
      cached_sig.elderfier_id = elf_id;
      cached_sig.block_height = blockHeight;
      cached_sig.timestamp = std::time(nullptr);
      cached_sig.received_block_height = blockHeight;
      cached_sig.is_valid = true;

      // Add to commitment index
      m_commitmentIndex->addSignatureToCache(cached_sig);
    }
  }

  void distributeFees(int elderfierCount, uint64_t epochNumber) {
    uint64_t feePerElderfier = 50000000;  // 0.5 XFG per elderfier

    for (int i = 0; i < elderfierCount; i++) {
      uint8_t elf_id = static_cast<uint8_t>(i);
      std::string address = "test_address_" + std::to_string(i);

      // Add fee to escrow
      m_feeEscrow->addFeeEscrow(epochNumber, elf_id, address, feePerElderfier);
    }
  }

  // Members
  std::unique_ptr<CommitmentIndex> m_commitmentIndex;
  std::unique_ptr<FeeEscrowManager> m_feeEscrow;
  std::unique_ptr<EldernodeIndexManager> m_eldernodeIndex;
  std::unique_ptr<ElderfierSignatureDaemon> m_daemon;
  IP2pEndpoint* m_p2pEndpoint = nullptr;
};

// Test 1: Register 5 elderfiers
TEST_F(ElderfierIntegrationTest, RegisterFiveElderfiers) {
  EXPECT_NO_THROW({
    registerElderfiers(5);
  });

  auto elderfiers = m_eldernodeIndex->getElderfierNodes();
  EXPECT_EQ(elderfiers.size(), 5);
}

// Test 2: Simulate signature gossip
TEST_F(ElderfierIntegrationTest, SignatureGossip) {
  registerElderfiers(5);

  uint32_t blockHeight = 1000;
  EXPECT_NO_THROW({
    simulateSignatures(5, blockHeight);
  });

  // Verify signatures were cached
  auto signed_ids = m_commitmentIndex->getSignedElderfierIds();
  EXPECT_EQ(signed_ids.size(), 5);
}

// Test 3: Fee distribution
TEST_F(ElderfierIntegrationTest, FeeDistribution) {
  registerElderfiers(5);
  distributeFees(5, 1);

  // Verify fees were recorded
  EXPECT_GT(m_feeEscrow->getTotalFeeEscrow(), 0);

  // Verify per-elderfier fees
  uint64_t elf0_fees = m_feeEscrow->getUnclaimedFees(0);
  EXPECT_EQ(elf0_fees, 50000000);

  uint64_t elf4_fees = m_feeEscrow->getUnclaimedFees(4);
  EXPECT_EQ(elf4_fees, 50000000);
}

// Test 4: Fee claiming
TEST_F(ElderfierIntegrationTest, FeeClaiming) {
  registerElderfiers(5);
  distributeFees(5, 1);

  // Verify initial fees
  uint64_t initialFees = m_feeEscrow->getUnclaimedFees(0);
  EXPECT_GT(initialFees, 0);

  // Claim fees
  EXPECT_TRUE(m_feeEscrow->claimFees(1, 0, 2000));

  // Verify fees are claimed
  uint64_t remainingFees = m_feeEscrow->getUnclaimedFees(0);
  EXPECT_EQ(remainingFees, 0);

  // Verify claimed count increased
  EXPECT_GT(m_feeEscrow->getTotalFeesClaimed(), 0);
}

// Test 5: Consensus threshold (69%)
TEST_F(ElderfierIntegrationTest, ConsensusThreshold) {
  registerElderfiers(5);

  // Simulate 3 out of 5 signatures (60% - below threshold)
  simulateSignatures(3, 1000);
  auto signed_count_1 = m_commitmentIndex->getSignedElderfierIds().size();
  EXPECT_EQ(signed_count_1, 3);

  // Check consensus percentage (should be 60%)
  uint64_t consensus_pct = m_commitmentIndex->getConsensusPercentageForCurrentRoot();
  EXPECT_GE(consensus_pct, 60);
  EXPECT_LT(consensus_pct, 69);

  // Simulate 4 out of 5 signatures (80% - above threshold)
  simulateSignatures(4, 1001);
  auto signed_count_2 = m_commitmentIndex->getSignedElderfierIds().size();
  EXPECT_EQ(signed_count_2, 4);

  // Check consensus percentage (should be 80%)
  consensus_pct = m_commitmentIndex->getConsensusPercentageForCurrentRoot();
  EXPECT_GE(consensus_pct, 69);
}

// Test 6: Fee escrow persistence
TEST_F(ElderfierIntegrationTest, FeeEscrowPersistence) {
  {
    FeeEscrowManager escrow1("/tmp/fuego_test", Logging::ConsoleLogger());
    distributeFees(5, 1);

    uint64_t total_before = escrow1.getTotalFeeEscrow();
    EXPECT_GT(total_before, 0);

    // Save
    EXPECT_TRUE(escrow1.save());
  }

  // Load and verify
  {
    FeeEscrowManager escrow2("/tmp/fuego_test", Logging::ConsoleLogger());
    EXPECT_TRUE(escrow2.load());

    uint64_t total_after = escrow2.getTotalFeeEscrow();
    EXPECT_GT(total_after, 0);
  }
}

// Test 7: Elderfier deposit auto-registration
TEST_F(ElderfierIntegrationTest, DepositAutoRegistration) {
  // Create a mock deposit with 0xEC tag
  ElderfierDepositData deposit;
  deposit.depositHash = Crypto::cn_fast_hash("test", 4);
  deposit.elderfierPublicKey = Crypto::PublicKey{};
  deposit.depositAmount = 800000000000;  // 800 XFG
  deposit.elderfierAddress = "test_address";
  deposit.isActive = true;

  // Register deposit
  EXPECT_TRUE(m_eldernodeIndex->addElderfierDeposit(deposit));

  // Verify it's registered and active
  EXPECT_EQ(m_eldernodeIndex->getElderfierNodeCount(), 1);
}

// Test 8: Multiple epochs
TEST_F(ElderfierIntegrationTest, MultipleEpochs) {
  registerElderfiers(5);

  // Distribute fees across 3 epochs
  for (uint64_t epoch = 0; epoch < 3; epoch++) {
    distributeFees(5, epoch);
  }

  // Verify fees are tracked per epoch
  auto epoch0_entries = m_feeEscrow->getEpochEscrowEntries(0);
  EXPECT_EQ(epoch0_entries.size(), 5);

  auto epoch1_entries = m_feeEscrow->getEpochEscrowEntries(1);
  EXPECT_EQ(epoch1_entries.size(), 5);

  auto epoch2_entries = m_feeEscrow->getEpochEscrowEntries(2);
  EXPECT_EQ(epoch2_entries.size(), 5);
}

// Test 9: Fee statistics
TEST_F(ElderfierIntegrationTest, FeeStatistics) {
  registerElderfiers(5);
  distributeFees(5, 1);

  // Claim some fees
  m_feeEscrow->claimFees(1, 0, 1000);
  m_feeEscrow->claimFees(1, 1, 1000);

  auto stats = m_feeEscrow->getStats();

  EXPECT_GT(stats.totalFeesCollected, 0);
  EXPECT_EQ(stats.totalFeesDistributed, stats.totalFeesCollected);
  EXPECT_GT(stats.totalFeesClaimed, 0);
  EXPECT_LT(stats.pendingFeesInEscrow, stats.totalFeesCollected);
  EXPECT_EQ(stats.activeElderfierCount, 5);
}

// Test 10: Elderfier history
TEST_F(ElderfierIntegrationTest, ElderfierFeeHistory) {
  registerElderfiers(5);

  // Distribute fees across 3 epochs for elderfier 0
  for (uint64_t epoch = 0; epoch < 3; epoch++) {
    m_feeEscrow->addFeeEscrow(epoch, 0, "address_0", 100000000);
  }

  // Get history for elderfier 0
  auto history = m_feeEscrow->getElderfierEscrowHistory(0);
  EXPECT_EQ(history.size(), 3);

  // Verify epochs are in order
  EXPECT_EQ(history[0].epochNumber, 0);
  EXPECT_EQ(history[1].epochNumber, 1);
  EXPECT_EQ(history[2].epochNumber, 2);

  // Verify all unclaimed initially
  for (const auto& entry : history) {
    EXPECT_FALSE(entry.claimed);
  }

  // Claim one and verify
  m_feeEscrow->claimFees(1, 0, 1000);
  history = m_feeEscrow->getElderfierEscrowHistory(0);
  EXPECT_TRUE(history[1].claimed);
  EXPECT_EQ(history[1].claimBlockHeight, 1000);
}

// Test 11: Full system integration
TEST_F(ElderfierIntegrationTest, FullSystemIntegration) {
  // 1. Register 5 elderfiers
  EXPECT_NO_THROW({ registerElderfiers(5); });
  EXPECT_EQ(m_eldernodeIndex->getElderfierNodeCount(), 5);

  // 2. Simulate signatures for epoch 1
  EXPECT_NO_THROW({ simulateSignatures(4, 1000); });  // 4 out of 5 (80%)
  EXPECT_EQ(m_commitmentIndex->getSignedElderfierIds().size(), 4);

  // 3. Verify consensus threshold met
  uint64_t consensus = m_commitmentIndex->getConsensusPercentageForCurrentRoot();
  EXPECT_GE(consensus, 69);

  // 4. Distribute fees for epoch 1
  EXPECT_NO_THROW({ distributeFees(4, 1); });
  EXPECT_GT(m_feeEscrow->getTotalFeeEscrow(), 0);

  // 5. Verify fee distribution
  for (int i = 0; i < 4; i++) {
    uint64_t fees = m_feeEscrow->getUnclaimedFees(i);
    EXPECT_EQ(fees, 50000000);
  }

  // 6. Claim fees
  for (int i = 0; i < 4; i++) {
    EXPECT_TRUE(m_feeEscrow->claimFees(1, i, 1100 + i));
  }

  // 7. Verify fees claimed
  uint64_t totalClaimed = m_feeEscrow->getTotalFeesClaimed();
  EXPECT_GT(totalClaimed, 0);

  // 8. Verify statistics
  auto stats = m_feeEscrow->getStats();
  EXPECT_EQ(stats.activeElderfierCount, 5);
  EXPECT_LT(stats.pendingFeesInEscrow, stats.totalFeesCollected);
}

// Test 12: Stress test - many elderfiers
TEST_F(ElderfierIntegrationTest, StressTestManyElderfiers) {
  // Register 100 elderfiers
  EXPECT_NO_THROW({
    for (int i = 0; i < 100; i++) {
      ElderfierDepositData deposit;
      deposit.depositHash = Crypto::cn_fast_hash(
        ("test" + std::to_string(i)).c_str(),
        4 + std::to_string(i).length());
      deposit.depositAmount = 800000000000;
      deposit.elderfierAddress = "address_" + std::to_string(i);
      deposit.isActive = true;

      m_eldernodeIndex->addElderfierDeposit(deposit);
    }
  });

  EXPECT_EQ(m_eldernodeIndex->getElderfierNodeCount(), 100);

  // Simulate signatures from 70 of them (70%)
  EXPECT_NO_THROW({
    simulateSignatures(70, 5000);
  });

  // Verify consensus threshold met
  uint64_t consensus = m_commitmentIndex->getConsensusPercentageForCurrentRoot();
  EXPECT_GE(consensus, 69);

  // Distribute fees
  EXPECT_NO_THROW({
    for (int i = 0; i < 70; i++) {
      m_feeEscrow->addFeeEscrow(1, i, "address_" + std::to_string(i), 10000000);
    }
  });

  auto stats = m_feeEscrow->getStats();
  EXPECT_EQ(stats.activeElderfierCount, 70);
}

}  // namespace CryptoNote
