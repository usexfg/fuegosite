#include "MultiTransportNode.h"

#include <Common/Util.h>
#include <Logging/LoggerRef.h>
#include <System/EventLock.h>
#include <System/InterruptedException.h>
#include <System/Timer.h>

#include <chrono>
#include <thread>

namespace CryptoNote {

namespace {
  const char* TRANSPORT_TCP = "tcp";
  const char* TRANSPORT_MESH = "meshtastic";
}

MultiTransportNode::MultiTransportNode(
    std::unique_ptr<IP2pNode> primaryNode,
    std::unique_ptr<IP2pNode> secondaryNode,
    bool fallbackEnabled)
  : primaryNode_(std::move(primaryNode))
  , secondaryNode_(std::move(secondaryNode))
  , fallbackEnabled_(fallbackEnabled)
  , stopped_(false)
  , primaryActive_(true)
  , activeTransport_(TRANSPORT_TCP) {

  if (!primaryNode_) {
    throw std::invalid_argument("Primary node cannot be null");
  }

  // Start worker threads
  startWorkerThreads();
}

MultiTransportNode::~MultiTransportNode() {
  stop();
  
  // Join worker threads
  if (primaryWorker_.joinable()) {
    primaryWorker_.join();
  }
  if (secondaryWorker_.joinable()) {
    secondaryWorker_.join();
  }
}

std::unique_ptr<IP2pConnection> MultiTransportNode::receiveConnection() {
  std::unique_lock<std::mutex> lock(queueMutex_);
  
  while (connectionQueue_.empty() && !stopped_) {
    queueCV_.wait(lock);
  }
  
  if (stopped_) {
    throw std::runtime_error("Node stopped");
  }
  
  auto connection = std::move(connectionQueue_.front());
  connectionQueue_.pop();
  return connection;
}

void MultiTransportNode::stop() {
  stopped_ = true;
  
  // Notify all waiting threads
  queueCV_.notify_all();
}

bool MultiTransportNode::isPrimaryActive() const {
  std::lock_guard<std::mutex> lock(queueMutex_);
  return primaryActive_;
}

std::string MultiTransportNode::getActiveTransport() const {
  std::lock_guard<std::mutex> lock(queueMutex_);
  return activeTransport_;
}

void MultiTransportNode::startWorkerThreads() {
  // Start connection worker for primary transport
  primaryWorker_ = std::thread([this]() {
    connectionWorker(std::move(primaryNode_), true);
  });
  
  // Start connection worker for secondary transport (if available)
  if (secondaryNode_) {
    secondaryWorker_ = std::thread([this]() {
      connectionWorker(std::move(secondaryNode_), false);
    });
  }
  
  // Start transport monitoring thread
  std::thread monitorThread([this]() {
    transportMonitorThread();
  });
  monitorThread.detach(); // Let it run independently
}

void MultiTransportNode::transportMonitorThread() {
  LoggerRef logger(System::Dispatcher::get().getLogger(), "MultiTransport");
  logger(INFO, BRIGHT_WHITE) << "Transport monitoring started";
  
  auto lastTransportCheck = std::chrono::steady_clock::now();
  int failureCount = 0;
  
  while (!stopped_) {
    try {
      // Wait for the check interval
      System::Timer::sleep(std::chrono::milliseconds(TRANSPORT_CHECK_INTERVAL_MS));
      
      if (stopped_) break;
      
      bool primaryActive = checkPrimaryTransport();
      
      {
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        if (primaryActive) {
          // Primary is working - reset failure count
          if (!primaryActive_) {
            logger(INFO, BRIGHT_GREEN) << "Primary transport recovered";
            failureCount = 0;
          }
          primaryActive_ = true;
          activeTransport_ = TRANSPORT_TCP;
        } 
        else if (fallbackEnabled_ && secondaryNode_) {
          // Primary is down - check if we should switch
          failureCount++;
          
          if (failureCount >= PRIMARY_FAILURE_THRESHOLD && primaryActive_) {
            logger(WARNING, BRIGHT_YELLOW) << "Primary transport failed, falling back to Meshtastic";
            primaryActive_ = false;
            activeTransport_ = TRANSPORT_MESH;
          }
        }
      }
      
      lastTransportCheck = std::chrono::steady_clock::now();
    } catch (const std::exception& e) {
      LoggerRef logger(System::Dispatcher::get().getLogger(), "MultiTransport");
      logger(ERROR) << "Transport monitor error: " << e.what();
      System::Timer::sleep(std::chrono::milliseconds(TRANSPORT_CHECK_INTERVAL_MS));
    }
  }
}

void MultiTransportNode::connectionWorker(std::unique_ptr<IP2pNode> node, bool isPrimary) {
  LoggerRef logger(System::Dispatcher::get().getLogger(), "MultiTransport");
  
  try {
    while (!stopped_) {
      try {
        auto connection = node->receiveConnection();
        if (connection) {
          {
            std::lock_guard<std::mutex> lock(queueMutex_);
            connectionQueue_.push(std::move(connection));
          }
          queueCV_.notify_one();
          
          // Update transport status if this is the primary
          if (isPrimary) {
            std::lock_guard<std::mutex> lock(queueMutex_);
            primaryActive_ = true;
          }
        }
      } catch (const std::exception& e) {
        logger(DEBUGGING) << (isPrimary ? "Primary" : "Secondary") 
                          << " transport connection error: " << e.what();
        
        // Brief pause before retrying
        System::Timer::sleep(std::chrono::milliseconds(100));
      }
    }
  } catch (const System::InterruptedException&) {
    // Normal shutdown
  } catch (const std::exception& e) {
    logger(ERROR) << (isPrimary ? "Primary" : "Secondary") 
                  << " transport worker error: " << e.what();
  }
}

bool MultiTransportNode::checkPrimaryTransport() {
  // In a real implementation, this would check for active connections
  // or attempt a test connection with a short timeout
  
  // For demonstration, we'll assume primary is active if we've received
  // connections recently (this would be tracked in a real implementation)
  
  // In a production implementation, we'd check:
  // 1. Number of active connections
  // 2. Recent successful communication
  // 3. Network interface status
  
  // For this example, always return true if we're not stopped
  return !stopped_;
}

} // namespace CryptoNote