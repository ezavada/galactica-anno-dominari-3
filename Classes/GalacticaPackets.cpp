// GalacticaPackets.cpp
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// Initial version: 11/26/01, ERZ
//
//	This file contains packet functions

#include "GalacticaPackets.h"
#include "GalacticaConsts.h"
#include "pdg/sys/mutex.h"

static SInt64 sPacketCreateTotal;
static SInt64 sPacketReleaseTotal;
static SInt64 sPacketSentTotal;
static SInt64 sPacketReceivedTotal;
static SInt64 sPacketCreateCount[packet_NumPacketTypes + 1];
static SInt64 sPacketReleaseCount[packet_NumPacketTypes + 1];
static SInt64 sPacketSentCount[packet_NumPacketTypes + 1];
static SInt64 sPacketReceivedCount[packet_NumPacketTypes + 1];
static SInt64 sTotalBytesAllocated;
static SInt64 sTotalBytesFreed;
static SInt64 sTotalBytesSent;
static SInt64 sTotalBytesReceived;
static SInt64 sTotalInvalidPacketsReceived;

static pdg::Mutex  sPacketStatsMutex;
static pdg::Mutex  sPacketMemMutex;


static bool sPacketCountInited = false;

// following used internally only
enum {packet_BadPacketType = packet_NumPacketTypes};


const char*
Packet::GetName() throw() { 
    return GetPacketNameForType(packetType); 
}

bool
Packet::NeedsNulTermination() throw() {
    return PacketNeedsNulTermination(packetType);
}

bool 
Packet::ValidatePacket() throw() {
    return true;
}

bool 
PlayerToPlayerMessagePacket::ValidatePacket() throw() {
  #warning TODO: need better validation for player to player message packets
    if (std::strlen(messageText) > MAX_TEXT_MESSAGE_LEN) {
        return false;
    }
    return true;
}

bool 
GameInfoPacket::ValidatePacket() throw() {
    if (std::strlen(gameTitle) > MAX_GAME_NAME_LEN) {
        return false;
    }
    return true;
}

void 
GameInfoPacket::ByteSwapOutgoing() throw() {
    hostVersion         = Native_ToBigEndian32(hostVersion);
    currTurn            = Native_ToBigEndian32(currTurn);
    maxTimePerTurn      = Native_ToBigEndian32(maxTimePerTurn);
    secsRemainingInTurn = Native_ToBigEndian32(secsRemainingInTurn);
    compSkill           = Native_ToBigEndian16(compSkill);
    compAdvantage       = Native_ToBigEndian16(compAdvantage);
    sectorDensity       = Native_ToBigEndian16(sectorDensity);
    sectorSize          = Native_ToBigEndian16(sectorSize);
    gameID              = Native_ToBigEndian32(gameID);
}

void
GameInfoPacket::ByteSwapIncoming() throw() {
    hostVersion         = BigEndian32_ToNative(hostVersion);
    currTurn            = BigEndian32_ToNative(currTurn);
    maxTimePerTurn      = BigEndian32_ToNative(maxTimePerTurn);
    secsRemainingInTurn = BigEndian32_ToNative(secsRemainingInTurn);
    compSkill           = BigEndian16_ToNative(compSkill);
    compAdvantage       = BigEndian16_ToNative(compAdvantage);
    sectorDensity       = BigEndian16_ToNative(sectorDensity);
    sectorSize          = BigEndian16_ToNative(sectorSize);
    gameID              = BigEndian32_ToNative(gameID);
}

bool 
PlayerInfoPacket::ValidatePacket() throw() {
    if (std::strlen(info.name) > MAX_PLAYER_NAME_LEN) {
        return false;
    }
    return true;
}

bool 
LoginRequestPacket::ValidatePacket() throw() {
    if (std::strlen(copyrightStr) > strLen_Copyright) {
        return false;
    }
    if (std::strlen(password) > MAX_PASSWORD_LEN) {
        return false;
    }
    if (std::strlen(playerName) > MAX_PLAYER_NAME_LEN) {
        return false;
    }
    return true;
}

void 
LoginRequestPacket::ByteSwapOutgoing() throw() {
    clientVers = Native_ToBigEndian32(clientVers);
}

void
LoginRequestPacket::ByteSwapIncoming() throw() {
    clientVers = BigEndian32_ToNative(clientVers);
}

void 
LoginResponsePacket::ByteSwapOutgoing() throw() {
    rejoinKey = Native_ToBigEndian32(rejoinKey);
}

void
LoginResponsePacket::ByteSwapIncoming() throw() {
    rejoinKey = BigEndian32_ToNative(rejoinKey);
}

bool 
ThingyDataPacket::ValidatePacket() throw() {
    return true;
}

void 
ThingyDataPacket::ByteSwapOutgoing() throw() {
    id   = Native_ToBigEndian32(id);
    type = Native_ToBigEndian32(type);
}

void
ThingyDataPacket::ByteSwapIncoming() throw() {
    id   = BigEndian32_ToNative(id);
    type = BigEndian32_ToNative(type);
}

void
TurnCompletePacket::ByteSwapOutgoing() throw() {
    turnNum = Native_ToBigEndian16(turnNum);
}

void
TurnCompletePacket::ByteSwapIncoming() throw() {
    turnNum = BigEndian16_ToNative(turnNum);
}

bool 
PrivatePlayerInfoPacket::ValidatePacket() throw() {
    return true;
}

void 
PrivatePlayerInfoPacket::ByteSwapOutgoing() throw() {
    info.lastTurnPosted     = Native_ToBigEndian32(info.lastTurnPosted);
    info.numPiecesRemaining = Native_ToBigEndian32(info.numPiecesRemaining);
    info.homeID             = Native_ToBigEndian32(info.homeID);
    info.techStarID         = Native_ToBigEndian32(info.techStarID);
    info.hiStarTech         = Native_ToBigEndian16(info.hiStarTech);
    info.hiShipTech         = Native_ToBigEndian16(info.hiShipTech);
}

void
PrivatePlayerInfoPacket::ByteSwapIncoming() throw() {
    info.lastTurnPosted     = BigEndian32_ToNative(info.lastTurnPosted);
    info.numPiecesRemaining = BigEndian32_ToNative(info.numPiecesRemaining);
    info.homeID             = BigEndian32_ToNative(info.homeID);
    info.techStarID         = BigEndian32_ToNative(info.techStarID);
    info.hiStarTech         = BigEndian16_ToNative(info.hiStarTech);
    info.hiShipTech         = BigEndian16_ToNative(info.hiShipTech);
}

void 
CreateThingyRequestPacket::ByteSwapOutgoing() throw() {
    tempID = Native_ToBigEndian32(tempID);
    type   = Native_ToBigEndian32(type);
    where.ByteSwapOutgoing();
}

void
CreateThingyRequestPacket::ByteSwapIncoming() throw() {
    tempID = BigEndian32_ToNative(tempID);
    type   = BigEndian32_ToNative(type);
    where.ByteSwapIncoming();
}

void 
CreateThingyResponsePacket::ByteSwapOutgoing() throw() {
    tempID = Native_ToBigEndian32(tempID);
    newID  = Native_ToBigEndian32(newID);
}

void
CreateThingyResponsePacket::ByteSwapIncoming() throw() {
    tempID = BigEndian32_ToNative(tempID);
    newID  = BigEndian32_ToNative(newID);
}

void 
ThingyCountPacket::ByteSwapOutgoing() throw() {
    thingyCount = Native_ToBigEndian32(thingyCount);
}

void
ThingyCountPacket::ByteSwapIncoming() throw() {
    thingyCount = BigEndian32_ToNative(thingyCount);
}

void 
ReassignThingyRequestPacket::ByteSwapOutgoing() throw() {
    id = Native_ToBigEndian32(id);
    type = Native_ToBigEndian32(type);
    oldContainer = Native_ToBigEndian32(oldContainer);
    newContainer = Native_ToBigEndian32(newContainer);
}

void
ReassignThingyRequestPacket::ByteSwapIncoming() throw() {
    id = BigEndian32_ToNative(id);
    type   = BigEndian32_ToNative(type);
    oldContainer = BigEndian32_ToNative(oldContainer);
    newContainer = BigEndian32_ToNative(newContainer);
}

void 
ReassignThingyResponsePacket::ByteSwapOutgoing() throw() {
    id = Native_ToBigEndian32(id);
}

void
ReassignThingyResponsePacket::ByteSwapIncoming() throw() {
    id = BigEndian32_ToNative(id);
}

void 
ClientSettingsPacket::ByteSwapOutgoing() throw() {
    defaultGrowth = Native_ToBigEndian32(defaultGrowth);
    defaultTech = Native_ToBigEndian32(defaultTech);
    defaultShips = Native_ToBigEndian32(defaultShips);
}

void
ClientSettingsPacket::ByteSwapIncoming() throw() {
    defaultGrowth = BigEndian32_ToNative(defaultGrowth);
    defaultTech = BigEndian32_ToNative(defaultTech);
    defaultShips = BigEndian32_ToNative(defaultShips);
}


static void InitPacketCounts();

void 
InitPacketCounts() {
    pdg::AutoMutex mutex(&sPacketStatsMutex);
    sPacketCountInited = true;
    sPacketCreateTotal = 0;
    sPacketReleaseTotal = 0;
    sPacketSentTotal = 0;
    sPacketReceivedTotal = 0;
    for (int i = 0; i<=packet_NumPacketTypes; i++) {
        sPacketCreateCount[i] = 0;
        sPacketReleaseCount[i] = 0;
        sPacketSentCount[i] = 0;
        sPacketReceivedCount[i] = 0;
    }
    sTotalBytesAllocated = 0;
    sTotalBytesFreed = 0;
    sTotalBytesSent = 0;
    sTotalBytesReceived = 0;
    sTotalInvalidPacketsReceived = 0;
}

// packets created with CreatePacket are malloc'd, and must be freed with free()
Packet* 
CreatePacket(PacketTypeT inType, PacketLenT inLen) throw() {  
    if (!sPacketCountInited) {
        InitPacketCounts();
    }
    Packet* resultPacket = NULL;
    switch (inType) {
        case packet_Ignore:
            inLen = sizeof(Packet);
            break;
        case packet_GameInfoRequest:
            inLen = sizeof(GameInfoRequestPacket);
            break;
        case packet_PlayerInfoRequest:
            inLen = sizeof(PlayerInfoRequestPacket);
            break;
        case packet_HostDisconnect:
            inLen = sizeof(HostDisconnectPacket);
            break;
        case packet_LoginResponse:
            inLen = sizeof(LoginResponsePacket);
            break;
        case packet_TurnComplete:
            inLen = sizeof(TurnCompletePacket);
            break;
        case packet_PrivatePlayerInfo:
            inLen = sizeof(PrivatePlayerInfoPacket);
            break;
        case packet_CreateThingyRequest:
            inLen = sizeof(CreateThingyRequestPacket);
            break;
        case packet_CreateThingyResponse:
            inLen = sizeof(CreateThingyResponsePacket);
            break;
        case packet_ThingyCount:
            inLen = sizeof(ThingyCountPacket);
            break;
        case packet_Ping:
            inLen = sizeof(PingPacket);
            break;
        case packet_ClientSettings:
        	inLen = sizeof(ClientSettingsPacket);
        	break;
        case packet_ReassignThingyRequest:
            inLen = sizeof(ReassignThingyRequestPacket);
            break;
        case packet_ReassignThingyResponse:
            inLen = sizeof(ReassignThingyResponsePacket);
            break;
        case packet_AdminEndTurnNow:
            inLen = sizeof(AdminEndTurnNowPacket);
            break;
        case packet_AdminPauseGame:
            inLen = sizeof(AdminPauseGamePacket);
            break;
        case packet_AdminResumeGame:
            inLen = sizeof(AdminResumeGamePacket);
            break;
        case packet_PlayerToPlayerMessage:
        case packet_LoginRequest:
        case packet_PlayerInfo:
        case packet_GameInfo:
        case packet_ThingyData:
        case packet_AdminSetPlayerInfo:
        case packet_AdminSetGameInfo:
            // take length as given
           #warning TODO: compare to MaxSize for the packet type
            break;
        default:
            inLen = 0; // do nothing
            break;
    };
    if (inLen == 0) {
        DEBUG_OUT("Failed to CreatePacket, type = "<<(int)inType<<", size = "<<inLen, DEBUG_ERROR | DEBUG_PACKETS);
        pdg::AutoMutex mutex(&sPacketStatsMutex);
        ++sPacketCreateCount[packet_BadPacketType];
    } else {
        {
            pdg::AutoMutex mutex(&sPacketMemMutex);
            resultPacket = (Packet*)std::malloc(inLen);
        }
        if (resultPacket) {
            pdg::AutoMutex mutex(&sPacketStatsMutex);
            sTotalBytesAllocated += inLen;
            resultPacket->packetType = inType;
            resultPacket->packetLen = inLen;
//            DEBUG_OUT("Created Packet " << resultPacket->GetName() <<", size = "<<inLen, DEBUG_TRIVIA | DEBUG_PACKETS);
            ++sPacketCreateCount[inType];
            ++sPacketCreateTotal;
          #ifdef NUMBER_ALL_PACKETS
            resultPacket->packetNum = sPacketCreateTotal;
          #endif
        } else {
            DEBUG_OUT("MemAlloc failed for CreatePacket, type = "<<inType<<", size = "<<inLen, DEBUG_ERROR | DEBUG_PACKETS);
        }
    }
    return resultPacket;
}

void
ByteSwapOutgoingPacket(Packet* inPacket) {
  #if PLATFORM_LITTLE_ENDIAN
    if (inPacket == NULL) {
        return;
    }
    inPacket->packetLen = Native_ToBigEndian16(inPacket->packetLen);
   #ifdef NUMBER_ALL_PACKETS
    inPacket->packetNum = Native_ToBigEndian32(inPacket->packetNum);
   #endif // NUMBER_ALL_PACKETS
    switch (inPacket->packetType) {
        case packet_GameInfo:
            static_cast<GameInfoPacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_LoginRequest:
            static_cast<LoginRequestPacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_LoginResponse:
            static_cast<LoginResponsePacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_ThingyData:
            static_cast<ThingyDataPacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_TurnComplete:
            static_cast<TurnCompletePacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_PrivatePlayerInfo:
            static_cast<PrivatePlayerInfoPacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_CreateThingyRequest:
            static_cast<CreateThingyRequestPacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_CreateThingyResponse:
            static_cast<CreateThingyResponsePacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_ThingyCount:
            static_cast<ThingyCountPacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_ClientSettings:
        	static_cast<ClientSettingsPacket*>(inPacket)->ByteSwapOutgoing();
        	break;
        case packet_AdminSetGameInfo:
            static_cast<AdminSetGameInfoPacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_ReassignThingyRequest:
            static_cast<ReassignThingyRequestPacket*>(inPacket)->ByteSwapOutgoing();
            break;
        case packet_ReassignThingyResponse:
            static_cast<ReassignThingyResponsePacket*>(inPacket)->ByteSwapOutgoing();
            break;
        default:
            break;
    }
  #else
    inPacket; // reference so no warning
  #endif // PLATFORM_LITTLE_ENDIAN
}

void
ByteSwapIncomingPacket(Packet* inPacket) {
  #if PLATFORM_LITTLE_ENDIAN
    if (inPacket == NULL) {
        return;
    }
    inPacket->packetLen = BigEndian16_ToNative(inPacket->packetLen);
   #ifdef NUMBER_ALL_PACKETS
    inPacket->packetNum = BigEndian32_ToNative(inPacket->packetNum);
   #endif // NUMBER_ALL_PACKETS
    switch (inPacket->packetType) {
        case packet_GameInfo:
            static_cast<GameInfoPacket*>(inPacket)->ByteSwapIncoming();
            break;
        case packet_LoginRequest:
            static_cast<LoginRequestPacket*>(inPacket)->ByteSwapIncoming();
            break;
        case packet_LoginResponse:
            static_cast<LoginResponsePacket*>(inPacket)->ByteSwapIncoming();
            break;
        case packet_ThingyData:
            static_cast<ThingyDataPacket*>(inPacket)->ByteSwapIncoming();
            break;
        case packet_TurnComplete:
            static_cast<TurnCompletePacket*>(inPacket)->ByteSwapIncoming();
            break;
        case packet_PrivatePlayerInfo:
            static_cast<PrivatePlayerInfoPacket*>(inPacket)->ByteSwapIncoming();
            break;
        case packet_CreateThingyRequest:
            static_cast<CreateThingyRequestPacket*>(inPacket)->ByteSwapIncoming();
            break;
        case packet_CreateThingyResponse:
            static_cast<CreateThingyResponsePacket*>(inPacket)->ByteSwapIncoming();
            break;
        case packet_ThingyCount:
            static_cast<ThingyCountPacket*>(inPacket)->ByteSwapIncoming();
            break;
        case packet_ClientSettings:
        	static_cast<ClientSettingsPacket*>(inPacket)->ByteSwapIncoming();
        	break;
        case packet_ReassignThingyRequest:
            static_cast<ReassignThingyRequestPacket*>(inPacket)->ByteSwapIncoming();
            break;
        case packet_ReassignThingyResponse:
            static_cast<ReassignThingyResponsePacket*>(inPacket)->ByteSwapIncoming();
            break;
        default:
            break;
    }
  #else
    inPacket; // reference so no warning
  #endif // PLATFORM_LITTLE_ENDIAN
}

void
ReleasePacket(Packet* inPacket) throw() {
    if (!inPacket) {
        return;
    }
//    DEBUG_OUT("Freeing Packet " << inPacket->GetName() <<", size = "<<inPacket->packetLen, DEBUG_TRIVIA | DEBUG_PACKETS);
    {
        pdg::AutoMutex mutex(&sPacketStatsMutex);
        if (inPacket->packetType < packet_NumPacketTypes) {
            ++sPacketReleaseCount[inPacket->packetType];
            ++sPacketReleaseTotal;
        } else {
            ++sPacketReleaseCount[packet_BadPacketType];
        }
        sTotalBytesFreed += inPacket->packetLen;
    }
    pdg::AutoMutex mutex(&sPacketMemMutex);
    std::free(inPacket);
}

Packet*
ClonePacket(const Packet* inPacket) throw() {
    Packet* resultPacket = CreatePacket(inPacket->packetType, inPacket->packetLen);
    if (resultPacket) { // create could return NULL
        pdg::AutoMutex mutex(&sPacketMemMutex);
        std::memcpy((void*)resultPacket,(void*)inPacket, inPacket->packetLen);
    }
    return resultPacket; 
}

const char*  
GetPacketNameForType(PacketTypeT inType) throw() {
    switch (inType) {
        case packet_Ignore:
            return "IgnorePacket";
        case packet_PlayerToPlayerMessage:
            return "PlayerToPlayerMessagePacket";
        case packet_GameInfoRequest:
            return "GameInfoRequestPacket";
        case packet_GameInfo:
            return "GameInfoPacket";
        case packet_PlayerInfo:
            return "PlayerInfoPacket";
        case packet_HostDisconnect:
            return "HostDisconnectPacket";
        case packet_LoginRequest:
            return "LoginRequestPacket";
        case packet_LoginResponse:
            return "LoginResponsePacket";
        case packet_ThingyData:
            return "ThingyDataPacket";
        case packet_TurnComplete:
            return "TurnCompletePacket";
        case packet_PrivatePlayerInfo:
            return "PrivatePlayerInfoPacket";
        case packet_CreateThingyRequest:
            return "CreateThingyRequestPacket";
        case packet_CreateThingyResponse:
            return "CreateThingyResponsePacket";
        case packet_ThingyCount:
            return "ThingyCountPacket";
        case packet_PlayerInfoRequest:
            return "PlayerInfoRequestPacket";
        case packet_Ping:
            return "PingPacket";
        case packet_ReassignThingyRequest:
            return "ReassignThingyRequestPacket";
        case packet_ReassignThingyResponse:
            return "ReassignThingyResponsePacket";
        case packet_ClientSettings:
            return "ClientSettingsPacket";
        case packet_AdminSetPlayerInfo:
            return "AdminSetPlayerInfoPacket";
        case packet_AdminSetGameInfo:
            return "AdminSetGameInfoPacket";
        case packet_AdminEndTurnNow:
            return "AdminEndTurnNowPacket";
        case packet_AdminPauseGame:
            return "AdminPauseGamePacket";
        case packet_AdminResumeGame:
            return "AdminResumeGamePacket";
        default:
            {
                static char s[256];
                std::sprintf(s, "Unknown Packet Type %d", inType);
                return s;
            }
    }
}

bool
PacketNeedsNulTermination(PacketTypeT inType) throw() {
    switch (inType) {
        case packet_LoginRequest:
        case packet_PlayerToPlayerMessage:
        case packet_GameInfo:
        case packet_PlayerInfo:
            return true;
            break;
        default:
            return false;
            break;
    }
}

// Use this to validate a packet. Calls particular packet methods to validate.
// If packet invalid, sets type to packet_Ignore and returns false
bool
ValidatePacket(Packet* inPacket) throw() {
    bool result = inPacket->ValidatePacket();
    if (result) {
        switch (inPacket->packetType) {
           case packet_PlayerToPlayerMessage:
                result = static_cast<PlayerToPlayerMessagePacket*>(inPacket)->ValidatePacket();
                break;
            case packet_GameInfoRequest:
                result = static_cast<GameInfoRequestPacket*>(inPacket)->ValidatePacket();
                break;
            case packet_GameInfo:
                result = static_cast<GameInfoPacket*>(inPacket)->ValidatePacket();
                break;
            case packet_PlayerInfo:
                result = static_cast<PlayerInfoPacket*>(inPacket)->ValidatePacket();
                break;
            case packet_LoginRequest:
                result = static_cast<LoginRequestPacket*>(inPacket)->ValidatePacket();
                break;
            case packet_ThingyData:
                result = static_cast<ThingyDataPacket*>(inPacket)->ValidatePacket();
                break;
            case packet_PrivatePlayerInfo:
                result = static_cast<PrivatePlayerInfoPacket*>(inPacket)->ValidatePacket();
                break;
// following disabled because they don't have a ValidatePacket call
//            case packet_HostDisconnect:
//                result = static_cast<HostDisconnectPacket*>(inPacket)->ValidatePacket();
//                break;
//            case packet_LoginResponse:
//                result = static_cast<LoginResponsePacket*>(inPacket)->ValidatePacket();
//                break;
//            case packet_TurnComplete:
//                result = static_cast<TurnCompletePacket*>(inPacket)->ValidatePacket();
//                break;
//            case packet_CreateThingyRequest:
//                result = static_cast<CreateThingyRequestPacket*>(inPacket)->ValidatePacket();
//                break;
//            case packet_CreateThingyResponse:
//                result = static_cast<CreateThingyResponsePacket*>(inPacket)->ValidatePacket();
//                break;
//            case packet_ThingyCount:
//                result = static_cast<ThingyCountPacket*>(inPacket)->ValidatePacket();
//                break;
//            case packet_PlayerInfoRequest:
//                result = static_cast<PlayerInfoRequestPacket*>(inPacket)->ValidatePacket();
//                break;
            default:
                break;
        }
    }
    if (!result) {
        pdg::AutoMutex mutex(&sPacketStatsMutex);
        // this packet is invalid
        ++sTotalInvalidPacketsReceived;
        sTotalBytesReceived += inPacket->packetLen;
        inPacket->packetType = packet_Ignore;
    }
    return result;
}

void
RecordPacketSent(Packet* inPacket) throw() {
    if (!inPacket) return;
    if (inPacket->packetType < packet_NumPacketTypes) {
        ++sPacketSentCount[inPacket->packetType];
        ++sPacketSentTotal;
    } else {
        ++sPacketSentCount[packet_BadPacketType];
    }
    sTotalBytesSent += inPacket->packetLen;
}

void
RecordPacketReceived(Packet* inPacket) throw() {
    if (!inPacket) return;
    pdg::AutoMutex mutex(&sPacketStatsMutex);
    if (inPacket->packetType < packet_NumPacketTypes) {
        ++sPacketReceivedCount[inPacket->packetType];
        ++sPacketReceivedTotal;
    } else {
        ++sPacketReceivedCount[packet_BadPacketType];
    }
    sTotalBytesReceived += inPacket->packetLen;
}

void
ReportPacketStatistics() throw() {
    try {
        DEBUG_OUT("Packet Statistics Report", DEBUG_IMPORTANT | DEBUG_PACKETS);
        DEBUG_OUT("=========================================================", DEBUG_IMPORTANT | DEBUG_PACKETS);
        for (int i = 0; i<packet_NumPacketTypes; i++) {
            long packetSentPercent = (sPacketSentTotal != 0) ? (sPacketSentCount[i]*100)/sPacketSentTotal : 0;
            long packetReceivedPercent = (sPacketReceivedTotal != 0) ? (sPacketReceivedCount[i]*100)/sPacketReceivedTotal : 0;
            long packetCreatedPercent = (sPacketCreateTotal != 0) ? (sPacketCreateCount[i]*100)/sPacketCreateTotal : 0;
            long packetReleasedPercent = (sPacketReleaseTotal != 0) ? (sPacketReleaseCount[i]*100)/sPacketReleaseTotal : 0;
            DEBUG_OUT(GetPacketNameForType(i) << ": " 
                        << sPacketSentCount[i] << " (" << packetSentPercent << "%) sent, "
                        << sPacketReceivedCount[i]  << " (" << packetReceivedPercent << "%) received, "
                        << sPacketCreateCount[i]  << " (" << packetCreatedPercent << "%) created, "
                        << sPacketReleaseCount[i]  << " (" << packetReleasedPercent << "%) released", DEBUG_IMPORTANT | DEBUG_PACKETS);
            if (sPacketCreateCount[i] != sPacketReleaseCount[i]) {
                DEBUG_OUT("LEAKED " << (sPacketCreateCount[i] - sPacketReleaseCount[i]) << " "
                            << GetPacketNameForType(i) << "(s)!", DEBUG_ERROR | DEBUG_PACKETS);
            }
        }
        long invalidReceivedPercent = (sPacketReceivedTotal != 0) ? (sTotalInvalidPacketsReceived*100)/sPacketReceivedTotal : 0;
        DEBUG_OUT("Invalid Packets: " << sTotalInvalidPacketsReceived << " (" << invalidReceivedPercent << "%) received", 
                    DEBUG_IMPORTANT | DEBUG_PACKETS);
        DEBUG_OUT("Bad Packets: " << sPacketSentCount[packet_BadPacketType] << " sent, "
                    << sPacketReceivedCount[packet_BadPacketType] << " received, "
                    << sPacketCreateCount[packet_BadPacketType] << " created, "
                    << sPacketReleaseCount[packet_BadPacketType] << " released", DEBUG_IMPORTANT | DEBUG_PACKETS);
        if (sPacketCreateCount[packet_BadPacketType] != sPacketReleaseCount[packet_BadPacketType]) {
            DEBUG_OUT("LEAKED " 
                    << (sPacketCreateCount[packet_BadPacketType] - sPacketReleaseCount[packet_BadPacketType])
                    << " Bad Packet(s)!", DEBUG_ERROR | DEBUG_PACKETS);
        }
        DEBUG_OUT("Byte Totals: " << sTotalBytesSent << " sent, " << sTotalBytesReceived <<" received, "
                    << sTotalBytesAllocated << " allocated, " << sTotalBytesFreed << " freed",
                    DEBUG_IMPORTANT | DEBUG_PACKETS);
        if (sTotalBytesAllocated != sTotalBytesFreed) {
            DEBUG_OUT("LEAKED " 
                    << (sTotalBytesAllocated - sTotalBytesFreed)
                    << " bytes!", DEBUG_ERROR | DEBUG_PACKETS);
        }
        DEBUG_OUT("=========================================================", DEBUG_IMPORTANT | DEBUG_PACKETS);
    }
    catch(...) {
    }
}
