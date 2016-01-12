//	CEndpoint.h		
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================

#ifndef CENDPOINT_H_INCLUDED
#define CENDPOINT_H_INCLUDED

#include <queue>
#include "GalacticaPackets.h"
#include "pdg/sys/mutex.h"
#include "DebugMacros.h"

// include after std includes to avoid problems with macro definition of min
#include "OpenPlay.h"


/* #ifdef DEBUG_MACROS_ENABLED
    #if TARGET_API_MAC_CARBON
        #include "UEnvironment.h"
        #define ASYNC_DEBUG_OUT(a, b)  if (UEnvironment::IsRunningOSX()) { DEBUG_OUT(a, b); }
        #define ASYNC_ASSERT(a)        if (UEnvironment::IsRunningOSX()) { ASSERT(a); }
    #elif TARGET_OS_MAC
        #define ASYNC_DEBUG_OUT(a, b)
        #define ASYNC_ASSERT(a)
    #else
        #define ASYNC_DEBUG_OUT(a, b)  DEBUG_OUT(a, b)
        #define ASYNC_ASSERT(a)        ASSERT(a)
    #endif
#else
*/
    #define ASYNC_DEBUG_OUT(a, b)
    #define ASYNC_ASSERT(a)
//#endif

// total number of connections of any kind we can have before refusing new connections
#define MAX_CONNECTIONS 128


class CEndpoint {
public:
			    CEndpoint();		// constructor
	virtual		~CEndpoint();	// stub destructor

    bool            IsThisEndpoint(PEndpointRef inEndpoint) {return (mEndpoint == inEndpoint);}
    bool            HasEndpointRef() {return mEndpoint != NULL;}
	void	        SetEndpointRef(PEndpointRef inEndpoint);
    PEndpointRef    GetEndpointRef() {return mEndpoint;}
    void            Receive();
    void            SendPacket(Packet* inPacket);
    Packet*         GetNextPacket();
    bool            HasPacket();
    bool            WaitingToSend();
    void            SendWaitingPackets();

    const char*     GetName() {return mName.c_str();}

protected: 
    std::string         mName;   
protected:
	PEndpointRef        mEndpoint;
	std::queue<Packet*> mPackets;
	std::queue<Packet*> mOutgoingPackets;
    Packet*             mInProgressPacket;
    SInt32              mBytesRemainingForPacket;
    UInt32              mOffsetInPacket;
    Packet              mPacketInfo;
    pdg::Mutex			mPacketQueueMutex;
    pdg::Mutex			mOutgoingQueueMutex;
    SInt32              mBytesRemainingForHeader;
    UInt32              mOffsetInHeader;
};

#endif // CENDPOINT_H_INCLUDED

