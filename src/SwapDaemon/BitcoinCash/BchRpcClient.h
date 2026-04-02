// Copyright (c) 2017-2026 Fuego Developers
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
#include <string>
#include <cstdint>
#include <vector>

namespace XfgSwap {

struct BchUtxo {
  std::string txid;
  uint32_t vout;
  uint64_t satoshis;
  std::string scriptPubKey;
  uint32_t confirmations;
};

struct BchTxInfo {
  std::string txid;
  uint32_t confirmations;
  uint64_t blockHeight;
  bool inMempool;
};

class BchRpcClient {
public:
  BchRpcClient(const std::string& host, uint16_t port,
               const std::string& rpcUser, const std::string& rpcPassword);

  // Basic queries
  bool getBlockCount(uint64_t& height);
  bool getTransaction(const std::string& txid, BchTxInfo& info);
  bool getBalance(const std::string& address, uint64_t& satoshis);

  // UTXO queries
  bool listUnspent(const std::string& address, std::vector<BchUtxo>& utxos);

  // Raw transaction
  bool sendRawTransaction(const std::string& rawTxHex, std::string& txid);
  bool decodeRawTransaction(const std::string& rawTxHex, std::string& jsonResult);

  // Address utilities
  bool validateAddress(const std::string& address, bool& isValid);

  // Import address for watching (no private key)
  bool importAddress(const std::string& address, const std::string& label, bool rescan);

private:
  std::string rpcCall(const std::string& method, const std::string& params);
  std::string httpPost(const std::string& body);
  std::string base64Encode(const std::string& input);

  std::string m_host;
  uint16_t m_port;
  std::string m_rpcUser;
  std::string m_rpcPassword;
  std::string m_authHeader;  // "Basic base64(user:pass)"
};

} // namespace XfgSwap
