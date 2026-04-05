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
  const uint32_t USER_APP_PORT = 65535; // USER_APP port number from Mesh.proto
  const int MAX_RECONNECT_DELAY_MS = 30000; // 30 seconds
  const int MIN_RECONNECT_DELAY_MS = 1000;  // 1 second
}

MeshtasticConnection::MeshtasticConnection(
    const std::string& mqttBroker,
    uint32_t channel,
    const std::string& channelName,
    uint32_t myNodeNum,
    uint32_t peerNodeNum,
    const std::string& encryptionKey)
  : mqttBroker_(mqttBroker)
  , channel_(channel)
  , channelName_(channelName)
  , myNodeNum_(myNodeNum)
  , peerNodeNum_(peerNodeNum)
  , encryptionKey_(encryptionKey)
  , stopped_(false)
  , banned_(false) {
  
  // Start the worker thread for MQTT communication
  startWorker();
}

MeshtasticConnection::~MeshtasticConnection() {
  stop();
}

void MeshtasticConnection::read(P2pMessage& message) {
  std::unique_lock<std::mutex> lock(queueMutex_);
  
  while (messageQueue_.empty() && !stopped_ && !banned_) {
    queueCV_.wait(lock);
  }
  
  if (stopped_ || banned_) {
    throw std::runtime_error("Connection closed");
  }
  
  message = std::move(messageQueue_.front());
  messageQueue_.pop();
}

void MeshtasticConnection::write(const P2pMessage& message) {
  if (banned_) {
    throw std::runtime_error("Connection banned");
  }
  
  if (stopped_) {
    throw std::runtime_error("Connection stopped");
  }
  
  try {
    // Convert P2P message to Meshtastic packet
    meshtastic::MeshPacket meshPacket = createMeshPacket(message);
    
    // Serialize the packet
    std::string serialized;
    if (!meshPacket.SerializeToString(&serialized)) {
      throw std::runtime_error("Failed to serialize mesh packet");
    }
    
    // Publish to MQTT topic
    std::string topic = constructTopic();
    bool isConnected = false;
    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        if (client_) {
            isConnected = client_->isConnected();
        }
    }
    if (isConnected) {
      if (!mqttClient_->publish(topic, serialized)) {
        throw std::runtime_error("Failed to publish to MQTT");
      }
    }
  } catch (const std::exception& e) {
    // If writing fails, stop the connection
    stop();
    throw;
  }
}

void MeshtasticConnection::ban() {
  banned_ = true;
  stop();
}

void MeshtasticConnection::stop() {
  stopped_ = true;
  
  // Signal the worker thread to stop
  {
      std::lock_guard<std::mutex> lock(connectionMutex_);
      if (client_) {
    mqttClient_->disconnect();
  }
  
  // Wake up any waiting read operations
  queueCV_.notify_all();
  
  // Join the worker thread if it's running
  if (workerThread_.joinable()) {
    workerThread_.join();
  }
}

void MeshtasticConnection::startWorker() {
  workerThread_ = std::thread([this]() {
    System::Timer reconnectTimer(System::Dispatcher::get());
    int reconnectDelay = MIN_RECONNECT_DELAY_MS;
    
    while (!stopped_) {
      try {
        // Create and connect MQTT client
        {
            std::lock_guard<std::mutex> lock(connectionMutex_);
            client_.reset(new PahoMqttClient(mqttBroker_, "fuego_" + std::to_string(myNodeNum_), myNodeNum_));
        }
        
        bool connected = false;
        {
            std::lock_guard<std::mutex> lock(connectionMutex_);
            if (client_) {
                connected = client_->connect();
            }
        }
        if (connected) {
          // Reset reconnect delay after successful connection
          reconnectDelay = MIN_RECONNECT_DELAY_MS;
          
          // Subscribe to the channel topic
          bool subscribed = false;
          {
              std::lock_guard<std::mutex> lock(connectionMutex_);
              if (client_) {
                  subscribed = client_->subscribe(constructTopic());
              }
          }
          if (subscribed) {
            LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticConnection");
            logger(INFO, BRIGHT_WHITE) << "Connected to Meshtastic mesh via " << mqttBroker_
                                      << " on channel " << channel_ << " (" << channelName_ << ")";
            
            // Message processing loop
            while (!stopped_) {
              // This would normally be handled by MQTT callbacks
              // In a real implementation, we'd use async callbacks
              // For simplicity, we'll just sleep here
              System::Timer::sleep(std::chrono::milliseconds(100));
            }
          }
        }
      } catch (const std::exception& e) {
        LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticConnection");
        logger(WARNING) << "Meshtastic connection error: " << e.what();
      }
      
      // Clean up client
      {
          std::lock_guard<std::mutex> lock(connectionMutex_);
          client_.reset();
      }
      
      // Wait before attempting to reconnect
      if (!stopped_) {
        logger(WARNING) << "Attempting to reconnect to Meshtastic in " 
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

void MeshtasticConnection::processIncomingPacket(const meshtastic::MeshPacket& packet) {
  // Only process decoded packets with USER_APP payload
  if (!packet.decoded() || packet.payload_variant_case() != meshtastic::MeshPacket::kData) {
    return;
  }
  
  const meshtastic::Data& data = packet.data();
  if (data.portnum() != USER_APP_PORT) {
    return;
  }
  
  // Convert to P2P message
  P2pMessage p2pMessage;
  p2pMessage.type = data.request_id(); // Using request_id as message type
  p2pMessage.data.assign(data.payload().begin(), data.payload().end());
  
  // Add to message queue
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    messageQueue_.push(std::move(p2pMessage));
  }
  
  // Notify any waiting readers
  queueCV_.notify_one();
}

meshtastic::MeshPacket MeshtasticConnection::createMeshPacket(const P2pMessage& message) {
  meshtastic::MeshPacket packet;
  
  // Basic packet configuration
  packet.set_id(Common::random_hash().data);
  packet.set_from(myNodeNum_);
  packet.set_to(peerNodeNum_);
  packet.set_channel(channel_);
  packet.set_priority(meshtastic::MEDIUM);
  packet.set_want_ack(false);
  packet.set_hop_limit(3);   // Reasonable TTL for mesh
  packet.set_hop_start(3);
  
  // Set data payload
  meshtastic::Data* data = packet.mutable_data();
  data->set_portnum(USER_APP_PORT);
  data->set_payload(message.data.data(), message.data.size());
  data->set_request_id(message.type); // Using as message ID/type
  
  return packet;
}

std::string MeshtasticConnection::constructTopic() const {
  // Format: msh/<region>/<channel_name>/<portnum>/#
  // Example: msh/US/channels/mychannel/65535/#
  return "msh/US/" + channelName_ + "/" + std::to_string(USER_APP_PORT) + "/#";
}

// Concrete implementation of MQTT client using Boost ASIO for simplicity
// In a real implementation, this would use a proper MQTT library like Paho
MeshtasticConnection::PahoMqttClient::PahoMqttClient(
    const std::string& broker,
    const std::string& clientId,
    uint32_t myNodeNum)
  : broker_(broker)
  , clientId_(clientId)
  , myNodeNum_(myNodeNum)
  , mqttClient_(nullptr) {
}

bool MeshtasticConnection::PahoMqttClient::connect() {
  try {
    // Parse broker address (format: host:port)
    std::string host = "mqtt.meshtastic.org";
    uint16_t port = 1883;
    
    size_t pos = broker_.find(':');
    if (pos != std::string::npos) {
      host = broker_.substr(0, pos);
      port = static_cast<uint16_t>(std::stoi(broker_.substr(pos + 1)));
    }
    
    // In a real implementation, we'd set up the MQTT client here
    // For demonstration, we'll just simulate a successful connection
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticConnection");
    logger(DEBUGGING) << "Connecting to MQTT broker " << host << ":" << port
                     << " with client ID " << clientId_;
    
    // Simulate successful connection
    return true;
  } catch (const std::exception& e) {
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticConnection");
    logger(ERROR) << "MQTT connection error: " << e.what();
    return false;
  }
}

bool MeshtasticConnection::PahoMqttClient::disconnect() {
  // Clean up MQTT client resources
  if (mqttClient_) {
    // Actual disconnect logic would go here
    mqttClient_ = nullptr;
  }
  return true;
}

bool MeshtasticConnection::PahoMqttClient::publish(const std::string& topic, const std::string& payload) {
  try {
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticConnection");
    logger(TRACE) << "Publishing to " << topic << " (" << payload.size() << " bytes)";
    
    // In real implementation, this would publish to the MQTT broker
    return true;
  } catch (const std::exception& e) {
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticConnection");
    logger(ERROR) << "MQTT publish error: " << e.what();
    return false;
  }
}

bool MeshtasticConnection::PahoMqttClient::subscribe(const std::string& topic) {
  try {
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticConnection");
    logger(DEBUGGING) << "Subscribing to " << topic;
    
    // In real implementation, this would subscribe to the MQTT topic
    return true;
  } catch (const std::exception& e) {
    LoggerRef logger(System::Dispatcher::get().getLogger(), "MeshtasticConnection");
    logger(ERROR) << "MQTT subscribe error: " << e.what();
    return false;
  }
}

bool MeshtasticConnection::PahoMqttClient::isConnected() const {
  // In real implementation, check actual connection state
  return mqttClient_ != nullptr;
}

} // namespace CryptoNote