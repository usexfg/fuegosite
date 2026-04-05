#pragma once

#include <CryptoNote.h>
#include <P2p/P2pInterfaces.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace CryptoNote {

/**
 * @brief MultiTransportNode implements IP2pNode with multiple transport fallback
 * 
 * This class provides a P2P node implementation that manages multiple transport
 * layers (typically TCP/IP and Meshtastic) with graceful fallback between them.
 * 
 * The node prioritizes the primary transport (usually TCP) when available, and
 * automatically falls back to the secondary transport (Meshtastic) when the
 * primary transport fails or becomes unavailable.
 * 
 * Key features:
 * - Automatic transport selection based on connectivity
 * - Seamless fallback between transport layers
 * - Unified connection interface for higher-level P2P logic
 * - Configurable transport priorities
 */
class MultiTransportNode : public IP2pNode {
public:
    /**
     * @brief Construct a new Multi Transport Node object
     * 
     * @param primaryNode Primary transport node (typically TCP-based)
     * @param secondaryNode Secondary transport node (typically Meshtastic)
     * @param fallbackEnabled Whether to enable fallback to secondary transport
     */
    MultiTransportNode(
        std::unique_ptr<IP2pNode> primaryNode,
        std::unique_ptr<IP2pNode> secondaryNode,
        bool fallbackEnabled = true);
    
    virtual ~MultiTransportNode() override;
    
    /**
     * @brief Wait for and return a new incoming connection
     * 
     * Blocks until a connection is available from any active transport
     * 
     * @return std::unique_ptr<IP2pConnection> A new connection to a peer
     */
    virtual std::unique_ptr<IP2pConnection> receiveConnection() override;
    
    /**
     * @brief Stop the node and clean up resources
     */
    virtual void stop() override;
    
    /**
     * @brief Check if the primary transport is currently active
     * 
     * @return true Primary transport is active and connected
     * @return false Primary transport is down, using fallback
     */
    bool isPrimaryActive() const;
    
    /**
     * @brief Get the current active transport name
     * 
     * @return std::string "tcp" or "meshtastic"
     */
    std::string getActiveTransport() const;

private:
    // Transport nodes
    std::unique_ptr<IP2pNode> primaryNode_;
    std::unique_ptr<IP2pNode> secondaryNode_;
    bool fallbackEnabled_;
    
    // Node state
    bool stopped_;
    bool primaryActive_;
    std::string activeTransport_;
    
    // Connection queue for incoming connections
    std::queue<std::unique_ptr<IP2pConnection>> connectionQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    
    // Worker threads for each transport
    std::thread primaryWorker_;
    std::thread secondaryWorker_;
    
    // Configuration parameters
    static constexpr int TRANSPORT_CHECK_INTERVAL_MS = 5000; // 5 seconds
    static constexpr int PRIMARY_FAILURE_THRESHOLD = 3;      // Number of failed checks
    
    // Private methods
    void startWorkerThreads();
    void transportMonitorThread();
    void connectionWorker(std::unique_ptr<IP2pNode> node, bool isPrimary);
    bool checkPrimaryTransport();
};

} // namespace CryptoNote