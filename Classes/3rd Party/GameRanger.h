/*	GameRanger Libary	Copyright � 1999-2001 by Scott Kevill*/#ifndef __GAMERANGER_H__#define __GAMERANGER_H__#if defined(__MACH__) && defined(__APPLE__)	#include <Carbon/Carbon.h>#else	#ifndef __NETSPROCKET__	#include <NetSprocket.h>	#endif	#ifndef __ICONS__//	#include <Icons.h>	#endif#endif#ifdef __cplusplusextern "C" {#endifbool			GRCheckFileForCmd();bool			GRIsWaitingCmd();void			GRGetWaitingCmd();bool			GRIsCmd();bool			GRIsHostCmd();bool			GRIsJoinCmd();char*			GRGetHostGameName();UInt16			GRGetHostMaxPlayers();UInt32			GRGetJoinAddress();char*			GRGetJoinAddressStr();char*			GRGetPlayerName();UInt16			GRGetPortNumber();//char			*GRGetPlayerIcon();//IconSuiteRef	GRGetPlayerIconSuite();bool			GRHasProperty(SInt32 key);char*			GRGetPropertyStr(SInt32 key);SInt32			GRGetPropertyVal(SInt32 key);void			GRReset();void			GRHostReady();void			GRGameBegin();/*	Score reporting	*/void			GRStatMyPlayerID(UInt32 playerID);void			GRStatPlayerScore(UInt32 playerID, SInt32 playerScore);void			GRStatMyTeamID(UInt32 teamID);void			GRStatTeamScore(UInt32 teamID, SInt32 teamScore);/*	Old score API	*/void			GRStatScore(SInt32 score, UInt32 playerID);void			GRStatOtherScore(SInt32 score, UInt32 playerID);void			GRGameAbort();void			GRGameEnd();void			GRHostClosed();bool			GRIsGameRangerInstalled();OSErr			GROpenGameRanger();#if !defined(OPENPLAY_LIMITED) && !(defined(__MACH__) && defined(__APPLE__))boolGRNSpDoModalHostDialog			(NSpProtocolListReference  ioProtocolList,								 Str31 					ioGameName,								 Str31 					ioPlayerName,								 Str31 					ioPassword,								 NSpEventProcPtr 		inEventProcPtr);NSpAddressReferenceGRNSpDoModalJoinDialog			(ConstStr31Param 		inGameType,								 ConstStr255Param 		inEntityListLabel,								 Str31 					ioName,								 Str31 					ioPassword,								 NSpEventProcPtr 		inEventProcPtr);voidGRNSpReleaseAddressReference	(NSpAddressReference	inAddr);#endif/*	Same as GRCheckFileForCmd() but does not remove the command	*/bool	GRPeekFileForCmd(void);#ifdef __cplusplus}#endif#endif // __GAMERANGER_H__