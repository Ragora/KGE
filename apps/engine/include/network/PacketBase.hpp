/**
 *  @file PacketBase.hpp
 *  @brief Include file defining the Packet class.
 *
 *  This software is licensed under the GNU Lesser General Public License version 3.
 *  Please refer to gpl.txt and lgpl.txt for more information.
 *
 *  @author Draconic Entertainment
 *  @version 0.1.0
 *  @date 12/23/2013
 *  @copyright (c) 2013 Draconic Entertainment
 */

#ifndef _INCLUDE_KIARO_NETWORK_PACKET_H_
#define _INCLUDE_KIARO_NETWORK_PACKET_H_

#include "engine/Common.hpp"

#include <support/BitStream.hpp>
#include <engine/SerializableObjectBase.hpp>

namespace Kiaro
{
    namespace Network
    {
        class IncomingClientBase;

        //! Class representing a packet that has or is ready to traverse across the network to a remote host.
        class PacketBase : public Kiaro::Engine::SerializableObjectBase
        {
            // Public Methods
            public:
                /**
                 *  @brief Constructor that accepts a received netpacket from the underlaying networking subsystem.
                 *  @param received A packet from the internal networking subsystem to construct the class from.
                 */
                PacketBase(const Kiaro::Common::U32 &packetType = 0, Kiaro::Support::BitStream *received = NULL, IncomingClientBase *sender = NULL) : mID(0),
                mType(packetType), mSender(sender)
                {
                    //if (received)
                    //    UnpackData(*received);
                }

                virtual void packData(Kiaro::Support::BitStream &out)
                {
                    static Kiaro::Common::U32 sLastPacketID = 0;

                    out.writeU32(mType);
                    out.writeU32(sLastPacketID);
                    mID = sLastPacketID;

                    sLastPacketID++;
                }

                virtual void unpackData(Kiaro::Support::BitStream &in)
                {
                    mID = in.readU32();
                    mType = in.readU32();
                }

                Kiaro::Common::U32 getType(void) { return mType; }

                Kiaro::Common::U32 getID(void) { return mID; }

                virtual Kiaro::Common::U32 getMinimumPacketPayloadLength(void)
                {
                    return sizeof(Kiaro::Common::U32) * 2;
                }

            // Public Members
            public:
                static const Kiaro::Common::U8 sAcceptedStage = 0;

                //! A pointer to a Client instance that sent the data. If this is NULL, then the origin was a Server.
                Kiaro::Network::IncomingClientBase *mSender;

            // Private Members
            private:
                Kiaro::Common::U32 mType;
                Kiaro::Common::U32 mID;
        };
    } // End Namespace Network
} // End Namespace Kiaro
#endif // _INCLUDE_KIARO_NETWORK_PACKET_H_
