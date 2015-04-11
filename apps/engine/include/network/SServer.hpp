/**
 *  @file ServerSingleton.hpp
 *  @brief Include file defining the Server and related functions.
 *
 *  This software is licensed under the Draconic Free License version 1. Please refer
 *  to LICENSE.txt for more information.
 *
 *  @author Draconic Entertainment
 *  @copyright (c) 2014 Draconic Entertainment
 */

#ifndef _INCLUDE_KIARO_GAME_SERVERSINGLETON_HPP_
#define _INCLUDE_KIARO_GAME_SERVERSINGLETON_HPP_

#include <enet/enet.h>

#include "engine/common.hpp"

#include <game/entities/entities.hpp>

namespace Kiaro
{
    namespace Engine
    {
        class SGameWorld;
    }

    namespace Support
    {
        class CBitStream;
        class CMapDivision;
    } // End NameSpace Support

    namespace Game
    {
        class IGameMode;

        namespace Entities
        {
            class IEntity;
        }
    }

    namespace Network
    {
        class IMessage;
        class CClient;

        //! Server class that remote hosts connect to.
        class SServer
        {
            // Public Typedefs
            public:
                typedef std::set<CClient *>::iterator clientIterator;
                typedef std::set<CClient *>::const_iterator clientConstIterator;

            // Public Methods
            public:
                static SServer *getPointer(const Kiaro::Common::String &listenAddress = "0.0.0.0", const Kiaro::Common::U16 &listenPort = 11595, const Kiaro::Common::U32 &maximumClientCount = 32);
                static void destroy(void);

                /**
                 *  @brief Signals the server to stop running.
                 */
                void stop(void);

                void globalSend(Kiaro::Network::IMessage *packet, const bool &reliable);

                void setGamemode(Kiaro::Game::IGameMode *game);

                /**
                 *  @brief Returns the current running status of the server.
                 *  @return A boolean representing whether or not the server is running.
                 */
                bool isRunning(void);

                //! Performs an update time pulse on the server.
                void update(const Kiaro::Common::F32 &deltaTimeSeconds);

                //! Causes the server to handle all queued network events immediately.
                void dispatch(void);

                void networkUpdate(const Kiaro::Common::F32 &deltaTimeSeconds);

                /**
                 *  @brief Callback function that is called upon the server's underlaying
                 *  network subsystem accepting a remote host connection.
                 *  @param client A pointer to a Kiaro::Network::IncomingClientBase representing
                 *  the incoming connection.
                 */
                void onClientConnected(Kiaro::Network::CClient *client);

                /**
                 *  @brief Callback function that is called upon the server's underlaying
                 *  network subsystem disconnecting a remote host connection.
                 *  @param client A pointer to a Kiaro::Network::IncomingClientBase representing
                 *  the disconnected client.
                 */
                void onClientDisconnected(Kiaro::Network::CClient *client);

                /**
                 *  @brief Callback function that is called upon the server's underlaying
                 *  network subsystem receiving a packet.
                 *  @param incomingStream A reference to the Kiaro::Support::BitStream that can be
                 *  used to unpack the packet payload.
                 *  @param sender A pointer to a Kiaro::Network::IncomingClientBase representing
                 *  the sender of the packet.
                 */
                void onReceivePacket(Kiaro::Support::CBitStream &incomingStream, Kiaro::Network::CClient *sender);

                Kiaro::Network::CClient *getLastPacketSender(void);

                Kiaro::Common::U32 getClientCount(void);

               // Kiaro::Network::IncomingClientBase *GetLastPacketSender(void);
                clientIterator clientsBegin(void) { return mConnectedClientSet.begin(); }

                clientConstIterator clientsEnd(void) { return mConnectedClientSet.end(); }

            // Private Methods
            private:
                /**
                 *  @brief Constructor that initializes a server instance with the given information.
                 *  @param listen_address An array of c8 representing the IP address to listen on.
                 *  @param listen_port A Kiaro::Common::U16 representing the port number to listen on.
                 *  @param clients A Kiaro::Common::U32 representing the maximum number of clients to accept.
                 */
                SServer(const Kiaro::Common::String &listenAddress, const Common::U16 &listenPort, const Common::U32 &maximumClientCount);

                /**
                 *  @brief Standard virtual destructor.
                 *  @note The only reason this destructor is virtual is to prevent compiler warnings when deleting instances that
                 *  are children of this Server class.
                 */
                ~SServer(void);

            // Private Members
            private:
                Kiaro::Network::CClient *mLastPacketSender;

                Kiaro::Engine::SGameWorld *mEntityGroup;

                Kiaro::Game::IGameMode *mCurrentGamemode;

                bool mIsRunning;

                ENetHost *mInternalHost;

                //! The Port number that we're listening on.
                const Kiaro::Common::U16 mListenPort;
                //! The Address that we're listening on.
                const Kiaro::Common::String mListenAddress;

                Kiaro::Common::U32 mMaximumClientCount;

                std::set<CClient *> mConnectedClientSet;
        };
    } // End Namespace Network
} // End Namespace Kiaro
#endif // _INCLUDE_KIARO_GAME_SERVERSINGLETON_HPP_
