#include "MeshtasticNode.h"
#include "MeshtasticConnection.h"

#include <Common/Util.h>
#include <Logging/LoggerRef.h>
#include <System/EventLock.h>
#include <System/InterruptedException.h>
#include <System/Timer.h>

#include <google/protobuf/util/json_util.h>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

namespace CryptoNote {

namespace {
  const uint32_t BROADCAST_NUM = 0xFFFFFFFF;
  const int MAX_RECONNECT_DELAY_MS = 30000; // 30 seconds
  const int MIN_RECONNECT_DELAY_MS = 1000;  // 1 second
}

MeshtasticNode::MeshtasticNode(
    const std::string& mqttBroker,
    uint32_t channel,
    const std::string& channelName,
    uint32_t myNodeNum,
    const std::string& encryptionKey)
  : mqttBroker_(mqttBroker)
  , channel_(channel)
  , channelName_(channelName)
  , myNodeNum_(myNodeNum)
  , encryptionKey_(encryptionKey)
  , stopped_(false) {
  
  // Start the worker thread for MQTT communication
  startWorker();
}

MeshtasticNode::~MeshtasticNode() {
  stop();
}

std::unique_ptr<IP2pConnection> MeshtasticNode::receiveConnection() {
  std::unique_lock<std::mutex> lock(queueMutex_);
  
  while (newConnections_.empty() && !stopped_) {
    queueCV_.wait(lock);
  }
  
  if (stopped_) {
    throw std::runtime_error("Node stopped");
  }
  
  auto connection = std::move(newConnections_.front());
  newConnections_.pop();
  return connection;
}

void MeshtasticNode::stop() {
  stopped_ = true;
  
  // Wake up any waiting receiveConnection calls
  queueCV_.notify_all();
}

void MeshtasticNode::startWorker() {
  workerThread_ = std::thread([this]() {
    std::unique_ptr<MqttClient> mqttClient;
    System::Timer reconnectTimer(System::Dispatcher::get());
    int reconnectDelay = MIN_RECONNECT_DELAY_MS;
    
    while (!stopped_) {
      try {
        // Create and connect MQTT client
        mqttClient.reset(new PahoMqttClient(
            mqttBroker_, 
            "fuego_node_" + std::to_string(myNodeNum_),
            myNodeNum_));
        
        if (mqttClient->connect()) {
          // Reset reconnect delay after successful connection
          reconnectDelay = MIN_RECONNECT_DELAY_MS;
          
          // Subscribe to the channel topic
          if (mqttClient->subscribe(constructTopic())) {
            LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticNode");
            logger(INFO, BRIGHT_WHITE) << "Meshtastic node started on channel " 
                                      << channel_ << " (" << channelName_ << ")";
            
            // Main processing loop
            while (!stopped_) {
              // Process incoming messages
              if (!mqttClient->processIncoming()) {
                break; // Connection lost
              }
              
              // Sleep briefly to avoid busy-waiting
              System::Timer::sleep(std::chrono::milliseconds(100));
            }
          }
        }
      } catch (const std::exception& e) {
        LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticNode");
        logger(WARNING) << "Meshtastic node error: " << e.what();
      }
      
      // Clean up client
      mqttClient.reset();
      
      // Wait before attempting to reconnect
      if (!stopped_) {
        LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticNode");
        logger(WARNING) << "Meshtastic node disconnected. Reconnecting in " 
                        << reconnectDelay << " ms";
        
        try {
          System::Timer::sleep(std::chrono::milliseconds(reconnectDelay));
        } catch (const System::InterruptedException&) {
          break;
        }
        
        // Exponential backoff (with max)
        reconnectDelay = std::min(reconnectDelay * 2, MAX_RECONNECT_DELAY_MS);
      }
    }
  });
}

void MeshtasticNode::processIncomingPacket(const meshtastic::MeshPacket& packet) {
  // Only process decoded packets
  if (!packet.decoded()) {
    return;
  }
  
  // Handle node info for peer discovery
  if (packet.payload_variant_case() == meshtastic::MeshPacket::kNodeInfo) {
    const meshtastic::NodeInfo& nodeInfo = packet.node_info();
    uint32_t peerNodeNum = nodeInfo.num();
    
    // Skip our own node info
    if (peerNodeNum == myNodeNum_) {
      return;
    }
    
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticNode");
    logger(DEBUGGING) << "Discovered new peer on mesh: node " << peerNodeNum;
    
    // Create a connection to this peer
    auto connection = std::make_unique<MeshtasticConnection>(
        mqttBroker_,
        channel_,
        channelName_,
        myNodeNum_,
        peerNodeNum,
        encryptionKey_
    );
    
    // Add to new connections queue
    {
      std::lock_guard<std::mutex> lock(queueMutex_);
      newConnections_.push(std::move(connection));
    }
    
    // Notify any waiting receiveConnection calls
    queueCV_.notify_one();
  }
  
  // Handle direct messages (could be from known peers)
  else if (packet.payload_variant_case() == meshtastic::MeshPacket::kData) {
    const meshtastic::Data& data = packet.data();
    uint32_t fromNode = packet.from();
    
    // Only process USER_APP messages
    if (data.portnum() == 65535) {
      LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticNode");
      logger(TRACE) << "Received direct message from node " << fromNode;
      
      // If we don't have a connection for this peer yet, create one
      // (This could happen if we receive a message before node info)
      bool connectionExists = false;
      {
        std::lock_guard<std::mutex> lock(queueMutex_);
        // In a real implementation, we'd check existing connections
        // For simplicity, we assume connection doesn't exist
      }
      
      if (!connectionExists) {
        auto connection = std::make_unique<MeshtasticConnection>(
            mqttBroker_,
            channel_,
            channelName_,
            myNodeNum_,
            fromNode,
            encryptionKey_
        );
        
        // Add to new connections queue
        {
          std::lock_guard<std::mutex> lock(queueMutex_);
          newConnections_.push(std::move(connection));
        }
        
        // Notify any waiting receiveConnection calls
        queueCV_.notify_one();
      }
    }
  }
}

std::string MeshtasticNode::constructTopic() const {
  // Format: msh/<region>/<channel_name>/# 
  // Example: msh/US/channels/mychannel/#
  return "msh/US/" + channelName_ + "/#";
}

// Concrete implementation of MQTT client
MeshtasticNode::PahoMqttClient::PahoMqttClient(
    const std::string& broker,
    const std::string& clientId,
    uint32_t myNodeNum)
  : broker_(broker)
  , clientId_(clientId)
  , myNodeNum_(myNodeNum)
  , mqttClient_(nullptr) {
}

bool MeshtasticNode::PahoMqttClient::connect() {
  try {
    // Parse broker address (format: host:port)
    std::string host = "mqtt.meshtastic.org";
    uint16_t port = 1883;
    
    size_t pos = broker_.find(':');
    if (pos != std::string::npos) {
      host = broker_.substr(0, pos);
      port = static_cast<uint16_t>(std::stoi(broker_.substr(pos + 1)));
    }
    
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticNode");
    logger(DEBUGGING) << "Connecting to Meshtastic MQTT broker " << host << ":" << port;
    
    // In a real implementation, this would initialize the Paho client
    // For demonstration, we'll simulate a successful connection
    mqttClient_ = reinterpret_cast<void*>(0x1); // Non-null to indicate connected
    return true;
  } catch (const std::exception& e) {
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticNode");
    logger(ERROR) << "MQTT connection error: " << e.what();
    return false;
  }
}

bool MeshtasticNode::PahoMqttClient::disconnect() {
  if (mqttClient_) {
    // Actual disconnect logic would go here
    mqttClient_ = nullptr;
  }
  return true;
}

bool MeshtasticNode::PahoMqttClient::subscribe(const std::string& topic) {
  try {
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticNode");
    logger(DEBUGGING) << "Subscribing to " << topic;
    
    // In real implementation, subscribe to the topic
    return true;
  } catch (const std::exception& e) {
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticNode");
    logger(ERROR) << "MQTT subscribe error: " << e.what();
    return false;
  }
}

bool MeshtasticNode::PahoMqttClient::isConnected() const {
  return mqttClient_ != nullptr;
}

bool MeshtasticNode::PahoMqttClient::processIncoming() {
  try {
    if (!mqttClient_) {
      return false;
    }
    
    // In a real implementation, this would check for incoming messages
    // and call processIncomingPacket for each
    
    // Simulate receiving a NodeInfo packet periodically
    static int counter = 0;
    if ((counter++ % 10) == 0) {
      meshtastic::MeshPacket packet;
      packet.set_decoded(true);
      packet.set_from(0x12345678); // Simulate another node
      
      auto* nodeInfo = packet.mutable_node_info();
      nodeInfo->set_num(0x12345678);
      
      // Call the parent's processIncomingPacket
      auto* node = static_cast<MeshtasticNode*>(this);
      node->processIncomingPacket(packet);
    }
    
    return true;
  } catch (const std::exception& e) {
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticNode");
    logger(ERROR) << "MQTT processing error: " << e.what();
    return false;
  }
}

} // namespace CryptoNote