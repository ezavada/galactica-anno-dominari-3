//	CEndpoint.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#include "CEndpoint.h"

//#include <MacTypes.h>

CEndpoint::CEndpoint() {
    mEndpoint = NULL;
    mInProgressPacket = NULL;
    mOffsetInHeader = 0;
}

CEndpoint::~CEndpoint() {
}

void
CEndpoint::SetEndpointRef(PEndpointRef inEndpoint) {
   mInProgressPacket = NULL;
   if (mEndpoint != NULL) {
      {
         // release any incoming packets that may have been left behind
         pdg::AutoMutex mutex(&mPacketQueueMutex);
         while (!mPackets.empty()) {
            Packet* p = mPackets.front();
            mPackets.pop();
            ReleasePacket(p);
         }
      }
      {
         // release any outgoing packets that may have been left behind
         pdg::AutoMutex mutex(&mOutgoingQueueMutex);
         while (!mOutgoingPackets.empty()) {
            Packet* p = mOutgoingPackets.front();
            mOutgoingPackets.pop();
            ReleasePacket(p);
         }
      }
   }
   mEndpoint = inEndpoint; 
}

void
CEndpoint::Receive() {
    if (mEndpoint == NULL) {
        ASYNC_DEBUG_OUT("Endpoint Receive failed, not connected", DEBUG_ERROR | DEBUG_PACKETS);
        return;
    }
	NMErr err = kNMNoError;
	UInt32 dataLen;
    void* dataPtr;
	NMFlags flags;
	
    // repeat until we get an error code (no data left is returned as an error)
	while (err == kNMNoError) {
        // --------------------------------------------
        // Header reassembly and deframing section
        // --------------------------------------------
        if (!mInProgressPacket) {  // there is no partially received packet, so we are starting at the header
            if (mOffsetInHeader > 0) {
                // there is a partially received header
                dataPtr = (void*)( (char*)&mPacketInfo + mOffsetInHeader );
                ASYNC_DEBUG_OUT("Continuing receipt of partial packet header, dataPtr = " << dataPtr
                                << ", offsetInHeader = " << mOffsetInHeader << ", byteRemainingForHeader ="
                                << mBytesRemainingForHeader, DEBUG_TRIVIA | DEBUG_PACKETS);
            } else {
                // no partially received header, fetch the whole thing
                mBytesRemainingForHeader = sizeof(mPacketInfo);
        	    dataPtr = &mPacketInfo;
        	}
            dataLen = mBytesRemainingForHeader;
           	err = ProtocolReceive(mEndpoint, dataPtr, &dataLen, &flags);
//           		ASYNC_DEBUG_OUT("Client ProtocolReceive returned "<<err
//           		            << ": mPacketInfo.packetType = "<<(int)mPacketInfo.packetType
//           		            << ", mPacketInfo.packetLen = "<<mPacketInfo.packetLen
//           		            << ", dataLen = "<<dataLen <<", flags = "<<(int)flags, DEBUG_TRIVIA | DEBUG_PACKETS);
           	if (err != kNMNoDataErr) {  // don't report when we got no data

                if ((long)dataLen != mBytesRemainingForHeader) {
                    ASYNC_DEBUG_OUT("CEndpoint::Receive only returned "<<dataLen<<" bytes, not the "
                                << mBytesRemainingForHeader << " bytes required for the header", 
                                DEBUG_TRIVIA | DEBUG_PACKETS);
                    err = kNMNoDataErr;  // don't try to get more data for this packet now
                }
                mBytesRemainingForHeader -= dataLen;
                mOffsetInHeader += dataLen;
                ASYNC_ASSERT(mBytesRemainingForHeader >= 0);  // just making sure, Open Play shouldn't do this
            } else if (err != kNMNoDataErr) {  // don't report when we got no data
               ASYNC_DEBUG_OUT("CEndpoint::Receive ProtocolReceive for packet header returned "<<err, 
                               DEBUG_ERROR | DEBUG_PACKETS);
               return;
            }
            // Check for packet header complete
            if (mBytesRemainingForHeader == 0) {
                // we need to get the length byte swapped into native format to create the packet
                // but we don't want to byte swap the entire packet yet because we don't have all 
                // of it. PacketType is a one byte value and doesn't need to be swapped.
                PacketLenT packetLen = BigEndian16_ToNative(mPacketInfo.packetLen);
        	    mInProgressPacket = CreatePacket(mPacketInfo.packetType, packetLen);
//        				ASYNC_DEBUG_OUT("Client CreatePacket returned "<<(void*)mInProgressPacket
//        				            << ": mInProgressPacket->packetType = "<<(int)mInProgressPacket->packetType
//        				            << ", mInProgressPacket->packetLen = "<<mInProgressPacket->packetLen, 
//                                    DEBUG_TRIVIA | DEBUG_PACKETS);
        	    if (!mInProgressPacket) {
        	        ASYNC_DEBUG_OUT("CEndpoint::Receive failed to allocate incoming packet data, len = "
        	                    <<mPacketInfo.packetLen, DEBUG_ERROR | DEBUG_PACKETS);
        	        return;
        	    }
                // CreatePacket populated packetType, packetNum and packetLen based on what we
                // passed in. So replace them with the Big Endian values that came from the network
                // We will byte swap once we have the entire packet
              #ifdef NUMBER_ALL_PACKETS
                mInProgressPacket->packetNum = mPacketInfo.packetNum;
              #endif
                mInProgressPacket->packetLen = Native_ToBigEndian16(packetLen);
        	    mBytesRemainingForPacket = (SInt32)packetLen - sizeof(mPacketInfo);
        	    mOffsetInPacket = PACKET_DATA_OFFSET;
                mOffsetInHeader = 0;  // reset for header of next packet
           	}
        }
        // --------------------------------------------
        // Packet body reassembly section
        // --------------------------------------------
    	if (err == kNMNoError) {
//        				ASYNC_DEBUG_OUT("Client bytes remaining for packet = " << mBytesRemainingForPacket
//    				                << ", mInProgressPacket->packetLen = "<<mInProgressPacket->packetLen,
//                                    DEBUG_TRIVIA | DEBUG_PACKETS);
    	    if (mBytesRemainingForPacket > 0) {  // handle packets that are just the header
    		    dataLen = mBytesRemainingForPacket;
    	        void* dataPtr = (void*)( (char*)mInProgressPacket + mOffsetInPacket );
//            				    ASYNC_DEBUG_OUT("Client datPtr = " << dataPtr
//        				                << ": (void*)( (char*)mInProgressPacket + mOffsetInPacket ) ["
//        				                <<(void*)mInProgressPacket << "] + ["
//        				                <<mOffsetInPacket<<"]", DEBUG_TRIVIA | DEBUG_PACKETS);
    		    err = ProtocolReceive(mEndpoint, dataPtr, &dataLen, &flags);
//            				ASYNC_DEBUG_OUT("Client ProtocolReceive returned "<<err
//            				            << ", dataLen = "<<dataLen <<", flags = "<<(int)flags,
//            				            DEBUG_TRIVIA | DEBUG_PACKETS);
                if (err == kNMNoError) {
                    if ((long)dataLen != mBytesRemainingForPacket) {
                        ASYNC_DEBUG_OUT("Endpoint ProtocolReceive() only returned "<<dataLen<<" bytes, not the "
                                    << mBytesRemainingForPacket << " bytes required", 
                                    DEBUG_DETAIL | DEBUG_PACKETS);
                    }
                    mBytesRemainingForPacket -= dataLen;
                    mOffsetInPacket += dataLen;
                    ASYNC_ASSERT(mBytesRemainingForPacket >= 0);  // just making sure. OpenPlay should never do this
                } else if (err != kNMNoDataErr) {
           	        ASYNC_DEBUG_OUT("Endpoint ProtocolReceive for packet body returned "<<err, 
           	                        DEBUG_ERROR | DEBUG_PACKETS);
                    return;
                }
     		}
     		if (mBytesRemainingForPacket == 0) {
     		    ByteSwapIncomingPacket(mInProgressPacket);
     		    // we got all the data for the packet, and now its in native endian format
#ifdef NUMBER_ALL_PACKETS
               ASYNC_DEBUG_OUT("Endpoint enqueuing "<<mInProgressPacket->GetName()
                			     << " ["<<mInProgressPacket->packetNum<<"]" << ", length = "
                			     << mInProgressPacket->packetLen, DEBUG_TRIVIA | DEBUG_PACKETS);
#else
               ASYNC_DEBUG_OUT("Endpoint enqueuing "<<mInProgressPacket->GetName() << ", length = "
                               << mInProgressPacket->packetLen, DEBUG_TRIVIA | DEBUG_PACKETS);
#endif // NUMBER_ALL_PACKETS
                // queue packet until someone calls GetNextPacket()
                if (mInProgressPacket->NeedsNulTermination()) {
                    // check to make sure that it is Nul terminated
                    ASYNC_ASSERT(((char*)(mInProgressPacket))[mInProgressPacket->packetLen] == 0);
                    // add the termination
                    ((char*)(mInProgressPacket))[mInProgressPacket->packetLen] = 0;
                }
                RecordPacketReceived(mInProgressPacket);
                bool packetIsGood = ValidatePacket(mInProgressPacket);
                if (packetIsGood) {
                    pdg::AutoMutex mutex(&mPacketQueueMutex);
                    mPackets.push(mInProgressPacket);
                }
        	    mInProgressPacket = NULL;
    	    }
    	}
	}
}


void
CEndpoint::SendPacket(Packet* inPacket) {
    if (mEndpoint == NULL) {
        ASYNC_DEBUG_OUT("Endpoint SendPacket "<<inPacket->GetName() << " failed, not connected",
                        DEBUG_ERROR | DEBUG_PACKETS);
        return;
    }
    if (inPacket->packetLen >= MAX_PACKET_LEN) {
        ASYNC_DEBUG_OUT("Endpoint SendPacket "<<inPacket->GetName() << " failed, too large, size = " 
                    << inPacket->packetLen, DEBUG_ERROR | DEBUG_PACKETS);
        return;
    }
    if (inPacket->packetLen < sizeof(Packet) ) {
        ASYNC_DEBUG_OUT("Endpoint SendPacket "<<inPacket->GetName() << " failed, size not set, size = " 
                    << inPacket->packetLen, DEBUG_ERROR | DEBUG_PACKETS);
    }
    // always try to send any waiting packets
    SendWaitingPackets();
    // if, after trying to send waiting packets, there are still some in the queue, then queue
    // the packet to be sent
    if (WaitingToSend()) {
#ifdef NUMBER_ALL_PACKETS
        ASYNC_DEBUG_OUT("Packets already waiting, queuing outgoing packet "<<inPacket->GetName()
                        << " ["<<inPacket->packetNum<<"]", DEBUG_TRIVIA | DEBUG_PACKETS);
#else
        ASYNC_DEBUG_OUT("Packets already waiting, queuing outgoing packet "<<inPacket->GetName(),
                        DEBUG_TRIVIA | DEBUG_PACKETS);
#endif // NUMBER_ALL_PACKETS
        Packet* p = ClonePacket(inPacket);  // packet will be freed by caller, so we must make a copy
        pdg::AutoMutex mutex(&mOutgoingQueueMutex);
        mOutgoingPackets.push(p);        
    } else {
        long packetLen = inPacket->packetLen;
        ByteSwapOutgoingPacket(inPacket);   // swap to network byte order
        long sentBytes = ProtocolSend(mEndpoint, (void*)inPacket, packetLen, 0);
        ByteSwapIncomingPacket(inPacket);   // restore the data now that it's been sent
        if (sentBytes < 0) {
            // an error was returned, no bytes sent
            NMErr err = sentBytes;
            if (err == kNMFlowErr) {
#ifdef NUMBER_ALL_PACKETS
                ASYNC_DEBUG_OUT("Flow error, queuing outgoing packet "<<inPacket->GetName()
                                << " ["<<inPacket->packetNum<<"]", DEBUG_TRIVIA | DEBUG_PACKETS);
#else
                ASYNC_DEBUG_OUT("Flow error, queuing outgoing packet "<<inPacket->GetName(),
                                DEBUG_TRIVIA | DEBUG_PACKETS);
#endif // NUMBER_ALL_PACKETS
                Packet *p = ClonePacket(inPacket);  // packet will be freed by caller, so we must make a copy
                pdg::AutoMutex mutex(&mOutgoingQueueMutex);
                mOutgoingPackets.push(p);
            } else {
                ASYNC_DEBUG_OUT("Endpoint SendPacket "<<inPacket->GetName() << " Failed! Error "
                            << err, DEBUG_ERROR | DEBUG_PACKETS);
            }
        } else if (sentBytes != inPacket->packetLen) {
            // wrong number of bytes sent
            ASYNC_DEBUG_OUT("Endpoint SendPacket "<<inPacket->GetName() << " failed to send entire contents, size = "
                        <<inPacket->packetLen<<", sent = " << sentBytes, DEBUG_ERROR | DEBUG_PACKETS);
        } else {
            // packet was sent normally
            RecordPacketSent(inPacket);
#ifdef NUMBER_ALL_PACKETS
            ASYNC_DEBUG_OUT("Sent "<<inPacket->GetName() << " ["<<inPacket->packetNum<<"]",
                            DEBUG_DETAIL | DEBUG_PACKETS);
#else
            ASYNC_DEBUG_OUT("Sent "<<inPacket->GetName(), DEBUG_DETAIL | DEBUG_PACKETS);
#endif // NUMBER_ALL_PACKETS
        }
    }
}



Packet*
CEndpoint::GetNextPacket() {
    Packet* p = NULL;
    pdg::AutoMutex mutex(&mPacketQueueMutex);
    if (!mPackets.empty()) {
        p = mPackets.front();
        mPackets.pop();
    }
    return p;
}

bool
CEndpoint::HasPacket() {
    pdg::AutoMutex mutex(&mPacketQueueMutex);
    return !mPackets.empty();
}


bool
CEndpoint::WaitingToSend() {
    pdg::AutoMutex mutex(&mPacketQueueMutex);
    return !mOutgoingPackets.empty();
}

void
CEndpoint::SendWaitingPackets() {
    pdg::AutoMutex mutex(&mPacketQueueMutex);
    while (!mOutgoingPackets.empty()) {
        Packet* p = mOutgoingPackets.front();
        long packetLen = p->packetLen;
        ByteSwapOutgoingPacket(p);   // swap to network byte order
        long sentBytes = ProtocolSend(mEndpoint, (void*)p, packetLen, 0);
        ByteSwapIncomingPacket(p);   // restore the data now that it's been sent
        if (sentBytes < 0) {
            NMErr err = sentBytes;
            if (err == kNMFlowErr) {
                ASYNC_DEBUG_OUT("Flow error, leaving packet "<<p->GetName() << " in queue", 
                            DEBUG_TRIVIA | DEBUG_PACKETS);
                return;  // don't try to send any more
            } else {
                ASYNC_DEBUG_OUT("Endpoint SendWaitingPackets "<<p->GetName() << " Failed! Error "
                            << err, DEBUG_ERROR | DEBUG_PACKETS);
                return; // don't try to send any more
            }
        } else if (sentBytes != p->packetLen) {
            // wrong number of bytes sent
            ASYNC_DEBUG_OUT("Endpoint SendWaitingPacket "<<p->GetName() << " failed to send entire contents, size = "
                        <<p->packetLen<<", sent = " << sentBytes, DEBUG_ERROR | DEBUG_PACKETS);
            return; // don't try to send any more
        } else {
            // packet was sent normally
            mOutgoingPackets.pop();
            RecordPacketSent(p);
#ifdef NUMBER_ALL_PACKETS
            ASYNC_DEBUG_OUT("Sent "<<p->GetName() << " ["<<p->packetNum<<"]", DEBUG_DETAIL | DEBUG_PACKETS);
#else
            ASYNC_DEBUG_OUT("Sent "<<p->GetName(), DEBUG_DETAIL | DEBUG_PACKETS);
#endif // NUMBER_ALL_PACKETS
            ReleasePacket(p);
        }
    }
}
