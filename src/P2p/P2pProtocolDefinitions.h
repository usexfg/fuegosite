// Copyright (c) 2017-2026 Fuego Developers
// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2018-2019 The TurtleCoin developers
// Copyright (c) 2016-2020 The Karbo developers
// Copyright (c) 2018-2021 Conceal Network & Conceal Devs
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

#include "P2pProtocolTypes.h"

#include "../crypto/crypto.h"
#include "../CryptoNoteConfig.h"
#include "../CryptoNoteCore/CryptoNoteStatInfo.h"

// new serialization
#include "Serialization/ISerializer.h"
#include "Serialization/SerializationOverloads.h"
#include "CryptoNoteCore/CryptoNoteSerialization.h"

// For EMPTY_STRUCT definition (needed for P2P messages)
#include "Rpc/CoreRpcServerCommandsDefinitions.h"

namespace CryptoNote
{
  // EMPTY_STRUCT is defined in CoreRpcServerCommandsDefinitions.h
  // Forward declare here to avoid circular includes
  struct EMPTY_STRUCT;

  inline bool serialize(uuid& v, Common::StringView name, ISerializer& s) {
    return s.binary(&v, sizeof(v), name);
  }

  struct network_config
  {
    void serialize(ISerializer& s) {
      KV_MEMBER(connections_count)
      KV_MEMBER(handshake_interval)
      KV_MEMBER(packet_max_size)
      KV_MEMBER(config_id)
    }

    uint32_t connections_count;
    uint32_t connection_timeout;
    uint32_t ping_connection_timeout;
    uint32_t handshake_interval;
    uint32_t packet_max_size;
    uint32_t config_id;
    uint32_t send_peerlist_sz;
  };

  struct basic_node_data
  {
    uuid network_id;
    uint8_t version;
    uint64_t local_time;
    uint32_t my_port;
    PeerIdType peer_id;

    void serialize(ISerializer& s) {
      KV_MEMBER(network_id)
      if (s.type() == ISerializer::INPUT) {
        version = 0;
      }
      KV_MEMBER(version)
      KV_MEMBER(peer_id)
      KV_MEMBER(local_time)
      KV_MEMBER(my_port)
    }
  };

  struct CORE_SYNC_DATA
  {
    uint32_t current_height;
    Crypto::Hash top_id;

    void serialize(ISerializer& s) {
      KV_MEMBER(current_height)
      KV_MEMBER(top_id)
    }
  };

#define P2P_COMMANDS_POOL_BASE 1000

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_HANDSHAKE
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 1 };

    struct request
    {
      basic_node_data node_data;
      CORE_SYNC_DATA payload_data;

      void serialize(ISerializer& s) {
        KV_MEMBER(node_data)
        KV_MEMBER(payload_data)
      }

    };

    struct response
    {
      basic_node_data node_data;
      CORE_SYNC_DATA payload_data;
      std::list<PeerlistEntry> local_peerlist;

      void serialize(ISerializer& s) {
        KV_MEMBER(node_data)
        KV_MEMBER(payload_data)
        serializeAsBinary(local_peerlist, "local_peerlist", s);
      }
    };
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_TIMED_SYNC
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 2 };

    struct request
    {
      CORE_SYNC_DATA payload_data;

      void serialize(ISerializer& s) {
        KV_MEMBER(payload_data)
      }

    };

    struct response
    {
      uint64_t local_time;
      CORE_SYNC_DATA payload_data;
      std::list<PeerlistEntry> local_peerlist;

      void serialize(ISerializer& s) {
        KV_MEMBER(local_time)
        KV_MEMBER(payload_data)
        serializeAsBinary(local_peerlist, "local_peerlist", s);
      }
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  struct COMMAND_PING
  {
    /*
      Used to make "callback" connection, to be sure that opponent node
      have accessible connection point. Only other nodes can add peer to peerlist,
      and ONLY in case when peer has accepted connection and answered to ping.
    */
    enum { ID = P2P_COMMANDS_POOL_BASE + 3 };

#define PING_OK_RESPONSE_STATUS_TEXT "OK"

    struct request
    {
      /*actually we don't need to send any real data*/
      void serialize(ISerializer& s) {}
    };

    struct response
    {
      std::string status;
      PeerIdType peer_id;

      void serialize(ISerializer& s) {
        KV_MEMBER(status)
        KV_MEMBER(peer_id)
      }
    };
  };


#ifdef ALLOW_DEBUG_COMMANDS
  //These commands are considered as insecure, and made in debug purposes for a limited lifetime.
  //Anyone who feel unsafe with this commands can disable the ALLOW_GET_STAT_COMMAND macro.

  struct proof_of_trust
  {
    PeerIdType peer_id;
    uint64_t    time;
    Crypto::Signature sign;

    void serialize(ISerializer& s) {
      KV_MEMBER(peer_id)
      KV_MEMBER(time)
      KV_MEMBER(sign)
    }
  };

  inline Crypto::Hash get_proof_of_trust_hash(const proof_of_trust& pot) {
    std::string s;
    s.append(reinterpret_cast<const char*>(&pot.peer_id), sizeof(pot.peer_id));
    s.append(reinterpret_cast<const char*>(&pot.time), sizeof(pot.time));
    return Crypto::cn_fast_hash(s.data(), s.size());
  }

  struct COMMAND_REQUEST_STAT_INFO
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 4 };

    struct request
    {
      proof_of_trust tr;

      void serialize(ISerializer& s) {
        KV_MEMBER(tr)
      }
    };

    struct response
    {
      std::string version;
      std::string os_version;
      uint64_t connections_count;
      uint64_t incoming_connections_count;
      core_stat_info payload_info;

      void serialize(ISerializer& s) {
        KV_MEMBER(version)
        KV_MEMBER(os_version)
        KV_MEMBER(connections_count)
        KV_MEMBER(incoming_connections_count)
        KV_MEMBER(payload_info)
      }
    };
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_NETWORK_STATE
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 5 };

    struct request
    {
      proof_of_trust tr;

      void serialize(ISerializer& s) {
        KV_MEMBER(tr)
      }
    };

    struct response
    {
      std::list<PeerlistEntry> local_peerlist_white;
      std::list<PeerlistEntry> local_peerlist_gray;
      std::list<connection_entry> connections_list;
      PeerIdType my_id;
      uint64_t local_time;

      void serialize(ISerializer& s) {
        serializeAsBinary(local_peerlist_white, "local_peerlist_white", s);
        serializeAsBinary(local_peerlist_gray, "local_peerlist_gray", s);
        serializeAsBinary(connections_list, "connections_list", s);
        KV_MEMBER(my_id)
        KV_MEMBER(local_time)
      }
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct COMMAND_REQUEST_PEER_ID
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 6 };

    struct request
    {
      void serialize(ISerializer& s) {}
    };

    struct response
    {
      PeerIdType my_id;

      void serialize(ISerializer& s) {
        KV_MEMBER(my_id)
      }
    };
  };

#endif

  /************************************************************************/
  /* ELDERFIER SIGNATURE GOSSIP - P2P Message for Merkle Root Signatures */
  /************************************************************************/
  struct COMMAND_ELDERFIER_SIGNATURE
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 10 };

    struct request
    {
      Crypto::Hash merkle_root;              // Current merkle root hash
      Crypto::Signature signature;           // Ed25519 signature of merkle root
      uint8_t elderfier_id;                  // Elderfier ID (0-255) - privacy preserving
      uint64_t block_height;                // Block height when signed
      uint64_t timestamp;                   // Signature generation timestamp
      uint32_t version;                     // Message version (1)

      // Post-quantum hybrid extension (backward compatible - old nodes ignore)
      uint8_t sig_algorithm = 0;            // 0=Ed25519, 1=ML-DSA-65
      std::vector<uint8_t> pq_signature;    // Empty for Ed25519, 3293 bytes for ML-DSA-65
      std::vector<uint8_t> pq_pubkey;       // Empty for Ed25519, 1952 bytes for ML-DSA-65

      void serialize(ISerializer& s) {
        KV_MEMBER(merkle_root)
        KV_MEMBER(signature)
        KV_MEMBER(elderfier_id)
        KV_MEMBER(block_height)
        KV_MEMBER(timestamp)
        KV_MEMBER(version)
        KV_MEMBER(sig_algorithm)
        KV_MEMBER(pq_signature)
        KV_MEMBER(pq_pubkey)
      }
    };

    // NOTIFY pattern - no response needed (async broadcast)
    typedef EMPTY_STRUCT response;
  };

  /************************************************************************/
  /* ELDERFIER UNSTAKING REVIEW - P2P Gossip for Unstaking Notifications  */
  /************************************************************************/

  // Sent automatically when an EFier initiates unstaking.
  // Contains brief stats so active EFs can decide if full review is warranted.
  // No action required = unstake proceeds after review window.
  struct NOTIFY_ELDERFIER_UNSTAKING_REVIEW
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 11 };

    struct request
    {
      uint8_t elderfier_id;           // EFiD initiating unstaking
      std::string ceremony_alias;     // @ALIAS for human-readable display
      uint32_t unstaking_start_block;
      uint32_t review_window_blocks;  // How long peers have to respond

      // Brief activity stats (populated from epoch reports)
      uint64_t total_epochs_active;   // Total epochs this EFier has been registered
      uint64_t epochs_participated;   // Epochs where they actually signed
      uint64_t epochs_missed;         // Epochs where they did not sign
      uint32_t consecutive_missed;    // Current consecutive miss streak
      uint32_t double_sign_count;     // Total double-sign events ever recorded
      uint64_t total_fees_earned;     // Lifetime fees earned (atomic units)

      uint64_t broadcast_height;      // Block height when this was broadcast
      uint32_t version = 1;

      void serialize(ISerializer& s) {
        KV_MEMBER(elderfier_id)
        KV_MEMBER(ceremony_alias)
        KV_MEMBER(unstaking_start_block)
        KV_MEMBER(review_window_blocks)
        KV_MEMBER(total_epochs_active)
        KV_MEMBER(epochs_participated)
        KV_MEMBER(epochs_missed)
        KV_MEMBER(consecutive_missed)
        KV_MEMBER(double_sign_count)
        KV_MEMBER(total_fees_earned)
        KV_MEMBER(broadcast_height)
        KV_MEMBER(version)
      }
    };
    typedef EMPTY_STRUCT response;
  };

  // Sent by an active EF requesting a full review of an unstaking EFier.
  // Requires a documented reason from the permitted list — cannot be sent without evidence.
  // Same, no action = no problem, unstake continues.
  struct NOTIFY_ELDERFIER_FULL_REVIEW_REQUEST
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 12 };

    // Permitted reason codes (enforced at wallet layer before broadcast)
    // "double_sign"   — CommitmentIndex has recorded double-sign events for this EFiD
    // "missed_epochs" — epoch report shows consecutive_missed >= REVIEW_MISS_THRESHOLD
    // "invalid_sig"   — peer observed invalid signatures from this EFiD
    // "duty_abuse"    — EFier signed despite being in UNSTAKING state
    struct request
    {
      uint8_t target_efid;            // EFiD being reviewed
      uint8_t requester_efid;         // EFiD requesting the review
      std::string reason;             // One of permitted reason codes above
      std::string evidence_summary;   // Human-readable summary (max 256 chars)

      // Requesting EFier's signature (Ed25519) over: target_efid || reason || broadcast_height
      // Prove request isnt forged & requester is EFier
      Crypto::Signature requester_sig;
      Crypto::PublicKey requester_pubkey;

      uint64_t broadcast_height;
      uint32_t version = 1;

      void serialize(ISerializer& s) {
        KV_MEMBER(target_efid)
        KV_MEMBER(requester_efid)
        KV_MEMBER(reason)
        KV_MEMBER(evidence_summary)
        KV_MEMBER(requester_sig)
        KV_MEMBER(requester_pubkey)
        KV_MEMBER(broadcast_height)
        KV_MEMBER(version)
      }
    };
    typedef EMPTY_STRUCT response;
  };

  /************************************************************************/
  /* SWAP OFFER GOSSIP - P2P relay for atomic swap offers                */
  /************************************************************************/
  struct COMMAND_SWAP_OFFER
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 13 };

    struct request
    {
      std::string offerId;                // Unique offer identifier (hex)
      uint64_t    xfgAmount;              // XFG atomic units to swap
      uint64_t    rateNum;                // Rate: XFG per 1 CTR, scaled by 1e7
      uint8_t     pair;                   // 0=XMR, 1=ETH, 2=BCH
      Crypto::PublicKey makerPubKey;      // Maker's wallet pubkey
      Crypto::Signature signature;        // Signs offerId with maker's key
      uint64_t    timestamp;
      uint32_t    ttlBlocks;              // Offer expires after this many blocks
      uint32_t    postedHeight;           // Block height when posted

      void serialize(ISerializer& s) {
        KV_MEMBER(offerId)
        KV_MEMBER(xfgAmount)
        KV_MEMBER(rateNum)
        KV_MEMBER(pair)
        KV_MEMBER(makerPubKey)
        KV_MEMBER(signature)
        KV_MEMBER(timestamp)
        KV_MEMBER(ttlBlocks)
        KV_MEMBER(postedHeight)
      }
    };
    typedef EMPTY_STRUCT response;
  };

  /************************************************************************/
  /* SWAP CANCEL - P2P relay for cancelling a swap offer                  */
  /************************************************************************/
  struct COMMAND_SWAP_CANCEL
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 14 };

    struct request
    {
      std::string offerId;
      Crypto::PublicKey makerPubKey;
      Crypto::Signature signature;        // Signs "cancel:<offerId>" with maker's key

      void serialize(ISerializer& s) {
        KV_MEMBER(offerId)
        KV_MEMBER(makerPubKey)
        KV_MEMBER(signature)
      }
    };
    typedef EMPTY_STRUCT response;
  };

  /************************************************************************/
  /* SWAP TRADE COMPLETED - broadcast completed swap for TWAP             */
  /************************************************************************/
  struct COMMAND_SWAP_TRADE
  {
    enum { ID = P2P_COMMANDS_POOL_BASE + 15 };

    struct request
    {
      uint8_t     pair;
      uint64_t    xfgAmount;
      uint64_t    ctrAmount;
      uint64_t    rateScaled;             // Rate * 1e7 (integer representation)
      uint32_t    blockHeight;
      uint64_t    timestamp;

      void serialize(ISerializer& s) {
        KV_MEMBER(pair)
        KV_MEMBER(xfgAmount)
        KV_MEMBER(ctrAmount)
        KV_MEMBER(rateScaled)
        KV_MEMBER(blockHeight)
        KV_MEMBER(timestamp)
      }
    };
    typedef EMPTY_STRUCT response;
  };

}
