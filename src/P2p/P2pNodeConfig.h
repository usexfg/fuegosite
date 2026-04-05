// Copyright (c) 2017-2022 Fuego Developers
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
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

#include <chrono>
#include "NetNodeConfig.h"

namespace CryptoNote {

class P2pNodeConfig : public NetNodeConfig {
public:
  P2pNodeConfig();

  // getters
  std::chrono::nanoseconds getTimedSyncInterval() const;
  std::chrono::nanoseconds getHandshakeTimeout() const;
  std::chrono::nanoseconds getConnectInterval() const;
  std::chrono::nanoseconds getConnectTimeout() const;
  size_t getExpectedOutgoingConnectionsCount() const;
  size_t getWhiteListConnectionsPercent() const;
  boost::uuids::uuid getNetworkId() const;
  size_t getPeerListConnectRange() const;
  size_t getPeerListGetTryCount() const;
  bool getMeshtasticEnabled() const;
  std::string getMeshtasticMqttBroker() const;
  uint32_t getMeshtasticChannel() const;
  std::string getMeshtasticChannelName() const;
  std::string getMeshtasticEncryptionKey() const;

  // setters
  void setTimedSyncInterval(std::chrono::nanoseconds interval);
  void setHandshakeTimeout(std::chrono::nanoseconds timeout);
  void setConnectInterval(std::chrono::nanoseconds interval);
  void setConnectTimeout(std::chrono::nanoseconds timeout);
  void setExpectedOutgoingConnectionsCount(size_t count);
  void setWhiteListConnectionsPercent(size_t percent);
  void setNetworkId(const boost::uuids::uuid& id);
  void setPeerListConnectRange(size_t range);
  void setPeerListGetTryCount(size_t count);
  void setMeshtasticEnabled(bool enabled);
  void setMeshtasticMqttBroker(const std::string& broker);
  void setMeshtasticChannel(uint32_t channel);
  void setMeshtasticChannelName(const std::string& name);
  void setMeshtasticEncryptionKey(const std::string& key);

private:
  std::chrono::nanoseconds timedSyncInterval;
  std::chrono::nanoseconds handshakeTimeout;
  std::chrono::nanoseconds connectInterval;
  std::chrono::nanoseconds connectTimeout;
  boost::uuids::uuid networkId;
  size_t expectedOutgoingConnectionsCount;
  size_t whiteListConnectionsPercent;
  size_t peerListConnectRange;
  size_t peerListGetTryCount;
  bool meshtasticEnabled;
  std::string meshtasticMqttBroker;
  uint32_t meshtasticChannel;
  std::string meshtasticChannelName;
  std::string meshtasticEncryptionKey;
};

}
