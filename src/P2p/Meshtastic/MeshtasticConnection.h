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
 * @brief MeshtasticConnection implements IP2pConnection using Meshtastic mesh network
 * 
 * This class provides a P2P connection implementation that uses Meshtastic as the
 * underlying transport layer. It handles serialization/deserialization between
 * Fuego's P2P messages and Meshtastic packets, and manages the connection state.
 */
class MeshtasticConnection : public IP2pConnection {
public:
    /**
     * @brief Construct a new Meshtastic Connection object
     * 
     * @param mqttBroker Address of the MQTT broker (e.g., "mqtt.meshtastic.org:1883")
     * @param channel Channel number to use on the mesh network
     * @param channelName Name of the channel (for topic construction)
     * @param myNodeNum Our node number in the Meshtastic network
     * @param peerNodeNum Target node number to communicate with (BROADCAST_NUM for broadcast)
     * @param encryptionKey Encryption key for the channel (base64 encoded)
     */
    MeshtasticConnection(
        const std::string& mqttBroker,
        uint32_t channel,
        const std::string& channelName,
        uint32_t myNodeNum,
        uint32_t peerNodeNum,
        const std::string& encryptionKey);
    
    virtual ~MeshtasticConnection() override;

    /**
     * @brief Read a message from the connection (blocks until message is available)
     * 
     * @param message Output parameter for the received message
     */
    virtual void read(P2pMessage& message) override;
    
    /**
     * @brief Write a message to the connection
     * 
     * @param message Message to send
     */
    virtual void write(const P2pMessage& message) override;
    
    /**
     * @brief Ban the remote peer
     */
    virtual void ban() override;
    
    /**
     * @brief Stop the connection
     */
    virtual void stop() override;

private:
    // Connection parameters
    std::string mqttBroker_;
    uint32_t channel_;
    std::string channelName_;
    uint32_t myNodeNum_;
    uint32_t peerNodeNum_;
    std::string encryptionKey_;
    
    // Connection state
    bool stopped_;
    bool banned_;
    std::unique_ptr<class MeshtasticClient> client_; // Added client implementation
    std::mutex connectionMutex_; // For thread safety
    
    // Message queue for received packets
    std::queue<P2pMessage> messageQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    
    // MQTT client and worker thread
    std::unique_ptr<class MqttClient> mqttClient_;
    std::thread workerThread_;
    
    // Private methods
    void startWorker();
    void processIncomingPacket(const meshtastic::MeshPacket& packet);
    meshtastic::MeshPacket createMeshPacket(const P2pMessage& message);
    std::string constructTopic() const;
    
    /**
     * @brief MqttClient is an internal class that handles MQTT communication
     * 
     * This class abstracts the MQTT client implementation to avoid pulling in
     * external dependencies at the header level.
     */
    class MqttClient {
    public:
        virtual ~MqttClient() = default;
        virtual bool connect() = 0;
        virtual bool disconnect() = 0;
        virtual bool publish(const std::string& topic, const std::string& payload) = 0;
        virtual bool subscribe(const std::string& topic) = 0;
        virtual bool isConnected() const = 0;
    };
    
    /**
     * @brief PahoMqttClient is a concrete implementation using the Paho MQTT library
     * 
     * Note: In a real implementation, this would be conditionally compiled based
     * on whether Paho is available in the build environment.
     */
    class PahoMqttClient : public MqttClient {
    public:
        PahoMqttClient(
            const std::string& broker,
            const std::string& clientId,
            uint32_t myNodeNum);
        
        virtual bool connect() override;
        virtual bool disconnect() override;
        virtual bool publish(const std::string& topic, const std::string& payload) override;
        virtual bool subscribe(const std::string& topic) override;
        virtual bool isConnected() const override;
        
    private:
        std::string broker_;
        std::string clientId_;
        uint32_t myNodeNum_;
        void* mqttClient_; // Opaque pointer to Paho client
    };
};

} // namespace CryptoNote