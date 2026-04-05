#pragma once

#include <CryptoNote.h>
#include <P2p/P2pInterfaces.h>
#include "Meshtastic/Mesh.pb.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace CryptoNote {

/**
 * @brief MeshtasticNode implements IP2pNode using Meshtastic mesh network
 * 
 * This class represents a P2P node that uses the Meshtastic mesh network as its
 * transport layer. It handles peer discovery on the mesh network and creates
 * connections to discovered peers.
 * 
 * The node subscribes to the Meshtastic MQTT broker and processes incoming
 * packets to identify new peers and create connections to them.
 */
class MeshtasticNode : public IP2pNode {
public:
    /**
     * @brief Construct a new Meshtastic Node object
     * 
     * @param mqttBroker Address of the MQTT broker (e.g., "mqtt.meshtastic.org:1883")
     * @param channel Channel number to use on the mesh network
     * @param channelName Name of the channel (for topic construction)
     * @param myNodeNum Our node number in the Meshtastic network
     * @param encryptionKey Encryption key for the channel (base64 encoded)
     */
    MeshtasticNode(
        const std::string& mqttBroker,
        uint32_t channel,
        const std::string& channelName,
        uint32_t myNodeNum,
        const std::string& encryptionKey);
    
    virtual ~MeshtasticNode() override;
    
    /**
     * @brief Wait for and return a new incoming connection
     * 
     * Blocks until a new peer is discovered on the mesh network
     * 
     * @return std::unique_ptr<IP2pConnection> A new connection to a peer
     */
    virtual std::unique_ptr<IP2pConnection> receiveConnection() override;
    
    /**
     * @brief Stop the node and clean up resources
     */
    virtual void stop() override;
    
private:
    // Configuration parameters
    std::string mqttBroker_;
    uint32_t channel_;
    std::string channelName_;
    uint32_t myNodeNum_;
    std::string encryptionKey_;
    
    // Node state
    bool stopped_;
    
    // Queue for new incoming connections
    std::queue<std::unique_ptr<IP2pConnection>> newConnections_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    
    // Worker thread for MQTT communication
    std::thread workerThread_;
    
    // Private methods
    void startWorker();
    void processIncomingPacket(const meshtastic::MeshPacket& packet);
    
    /**
     * @brief MqttClient handles MQTT communication for the node
     * 
     * This class abstracts the MQTT client implementation to avoid pulling in
     * external dependencies at the header level.
     */
    class MqttClient {
    public:
        virtual ~MqttClient() = default;
        virtual bool connect() = 0;
        virtual bool disconnect() = 0;
        virtual bool subscribe(const std::string& topic) = 0;
        virtual bool isConnected() const = 0;
        
        /**
         * @brief Process incoming messages
         * 
         * This should be called regularly to handle incoming packets
         * 
         * @return bool true if successful, false if connection lost
         */
        virtual bool processIncoming() = 0;
    };
    
    /**
     * @brief PahoMqttClient implements MqttClient using the Paho MQTT library
     */
    class PahoMqttClient : public MqttClient {
    public:
        PahoMqttClient(
            const std::string& broker,
            const std::string& clientId,
            uint32_t myNodeNum);
        
        virtual bool connect() override;
        virtual bool disconnect() override;
        virtual bool subscribe(const std::string& topic) override;
        virtual bool isConnected() const override;
        virtual bool processIncoming() override;
        
    private:
        std::string broker_;
        std::string clientId_;
        uint32_t myNodeNum_;
        void* mqttClient_; // Opaque pointer to Paho client
    };
};

} // namespace CryptoNote