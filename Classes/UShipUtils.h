// UShipUtils.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ

#ifndef USHIPUTILS_H_INCLUDED
#define USHIPUTILS_H_INCLUDED

#include <vector>
#include "CThingyList.h"
#include "GalacticaConsts.h"
#include "LStream.h"

class CShip;
class CStar;

typedef class CourseT {
	friend class CShip;
	friend class CFleet;
public:
	CourseT() {loop = patrol = hunt = false; curr = 0; prev = 0;}
	~CourseT() {}
	CourseT& operator=(CourseT &crs) {
					waypoints = crs.waypoints; loop = crs.loop; patrol = crs.patrol;
					hunt = crs.hunt; curr = crs.curr; prev = crs.prev; return *this;}
	void		WriteStream(LStream *inStream);
	void		ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);
	void		Duplicate(CourseT* outCourse) const;
protected:
	std::vector<Waypoint> waypoints;
	bool		loop;
	bool		patrol;
	bool		hunt;
	SInt16		curr;
	SInt16		prev;
} CourseT, *CoursePtr;

LStream& operator >> (LStream& inStream, CourseT& outCourse);
LStream& operator << (LStream& inStream, CourseT& inCourse);




#define DRAG_SLOP_PIXELS 4

typedef struct MoveFactorT {
	long toOurStar;
	long toEnemyStar;
	long toOtherStar;
	long toShip;
	long toFormFleet;
	CStar* ourStar;
	CStar* enemyStar;
	CStar* otherStar;
	CShip* ship;
} MoveFactorT;

MoveFactorT NewDecisionMatrix();
void	ConsiderFactor(int factor, int shiptype, MoveFactorT &ioDecisionMatrix);
void	ConsiderMultiplierFactor(int factor, int shiptype, long delta, MoveFactorT &ioDecisionMatrix);

void	SortOutReserves(LArray* inForces, LArray* ioReserves, LArray* ioFleets);
bool    AttackLoop(LArray* attackers, LArray* attackersReserves, LArray* defenders, LArray* defendersReserves);
void	RemoveDead(LArray *ioForces);
SInt16	RemoveAllies(LArray *ioForces, SInt16 inOwner);
bool	HasShipClass(LArray *inForces, EShipClass inClass);
long 	FinalizeLosses(LArray *ioFleets, SInt16 opponentPlayerNum);

bool	ThingyListHasShipClass(const CThingyList *inForces, EShipClass inClass, CShip* dontCountThisOne = 0);

float	GetBuildCostOfShipClass(EShipClass inClass, UInt16 inTechLevel);

Point 	GetDockedShipOffsetFromStarCenter(float inZoomScale);

#define NUM_MOVE_FACTORS			35
#define NUM_SHIP_TYPE_MATRICIES		3

extern MoveFactorT gMoveFactors[NUM_SHIP_TYPE_MATRICIES][NUM_MOVE_FACTORS];

void	GetSpeedString(SInt32 inSpeed, std::string &outStr);

#endif // USHIPUTILS_H_INCLUDED


