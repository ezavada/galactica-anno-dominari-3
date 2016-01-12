// CShip.h
// ===========================================================================
// Copyright (c) 1996-2002, Sacred Tree Software
// All rights reserved
// ===========================================================================
// 3/9/96 ERZ

// AGalacticThingy

#ifndef CSHIP_H_INCLUDED
#define CSHIP_H_INCLUDED

#include "ANamedThingy.h"
#include "UShipUtils.h"

// const for adding waypoint to end of list
#define kEndOfWaypointList 0x7fffffff

class CShip : public ANamedThingy {
public:
			CShip(LView *inSuperView, GalacticaShared* inGame, long inThingyType = thingyType_Ship);
//			CShip(LView *inSuperView, GalacticaShared* inGame, LStream *inStream, long inThingyType = thingyType_Ship);
			virtual ~CShip();
			
	void InitShip();

// ---- database/streaming methods ----
	virtual void	ReadStream(LStream *inStream, UInt32 streamVersion = version_SavedTurnFile);
	virtual void	WriteStream(LStream *inStream, SInt8 forPlayerNum = kNoSuchPlayer);
	virtual void	UpdateHostFromStream(LStream *inStream);	// doesn't alter anything controlled by host
#ifndef GALACTICA_SERVER
	virtual void	UpdateClientFromStream(LStream *inStream);	// doesn't alter user interface items
#endif // GALACTICA_SERVER

// ---- container methods ----
	virtual bool	IsOfContentType(EContentTypes inType) const;
	virtual bool	SupportsContentsOfType(EContentTypes inType) const;
	virtual void*	GetContentsListForType(EContentTypes inType) const;
	virtual SInt32	GetContentsCountForType(EContentTypes inType) const;

// ---- object info methods ----
	virtual ResIDT  GetViewLayrResIDOffset();
	virtual long	GetDangerLevel() const;
	virtual void	NotChanged() {mChanged = false; mCourseChanged = false;}
	virtual long	GetAttackStrength() const;
	virtual long	GetDefenseStrength() const;
	long		    GetDamagePercent() {if (mPower) return mDamage / mPower; else return 0;}
	bool            WillCallIn() const {return mWantsAttention | mUserCallIn;}
	void            SetCallIn(bool inCall) {mWantsAttention = inCall; mUserCallIn = inCall;}
	void            SetAutoCallIn() {mWantsAttention = true;}
	void            SetSpeed(UInt32 inSpeed) {mSpeed = inSpeed;}
	UInt32			GetSpeed() const { return mSpeed; }
	void            SetPower(UInt32 inPower) {mPower = inPower;}
	UInt32			GetPower() const {return mPower;}
	void            SetShipClass(EShipClass inClass) {mClass = inClass;}
	EShipClass		GetShipClass() const {return (EShipClass)mClass;}

// ---- movement/course methods ----
	virtual void	ToggleScrap();
	virtual void    SetScrap(bool inScrap);
	CourseT*        GetCourse() {return &mCourse;}
	CourseT*        GetCourseMem() {return &mCourseMem;}
	void            SetCourse(CourseT* inCourse);
	void            SetCourseMem(CourseT* inCourse);
	int             GetDestWaypointNum() const {return mCourse.curr;}
	void            SetDestWaypointNum(int inNum) {mCourse.curr = inNum;}
	int             GetFromWaypointNum() const {return mCourse.prev;}
	void            SetFromWaypointNum(int inNum) {mCourse.prev = inNum;}
	bool            CourseLoops() const {return mCourse.loop;}
	void            SetLooping(bool inLoop) {mCourse.loop = inLoop;}
	bool            IsOnPatrol() const {return mCourse.patrol;}
	virtual void	SetPatrol(bool inPatrol) {mCourse.patrol = inPatrol;}
	bool            IsHunting() const {return mCourse.hunt;}
	void            SetHunting(bool inHunt) {mCourse.hunt = inHunt;}
	bool            CoursePlotted() const {return (mCourse.waypoints.size() != 0);}
	bool            CourseInMem() const {return (mCourseMem.waypoints.size() != 0);}
	bool            IsDocked() const {return (mCurrPos.GetType() == wp_Star);}
	int             GetWaypointCount() const {return mCourse.waypoints.size();}
	void		    AddWaypoint(int inWaypointNum, Waypoint inWaypoint, bool inRefresh);
	void			SetWaypoint(int inWaypointNum, Waypoint inWaypoint, bool inRefresh);
	void			DeleteWaypoint(int inWaypointNum, bool inRefresh);
	Waypoint		GetWaypoint(int inWayPointNum) const;
	Waypoint        GetDepartedFrom() const;
	Waypoint		GetDestination() const;
	const std::vector<Waypoint>&   GetWaypointsList() const {return static_cast<const std::vector<Waypoint>&>(mCourse.waypoints);}
	void			ClearCourse(bool inRecalcMove, bool inRefresh);
	void			SwapCourseWithCourseMem();
	void			WaypointsChanged(bool inRefresh) {CalcMoveDelta(); ThingMoved(inRefresh);} //call if you change the course wp list
	void			UserChangedCourse() {mCourseChanged = true;} // call for change by user
	void			SetSelectedWP(long inNum) {mSelectedWPNum = inNum;}
	int				GetSelectedWP() const {return mSelectedWPNum;}
	bool            HasMoved() const {return mHasMoved;}   // have we called EndOfTurn() yet turn this turn processing cycle?
	void            SetHasMoved(bool inHasMoved) {mHasMoved = inHasMoved;}

// ---- host only turn processing methods ----
	virtual void	DoComputerMove(long inTurnNum);   // only call from processor
    virtual void    PrepareForEndOfTurn(long inTurnNum);   // only call from processor
	virtual void 	EndOfTurn(long inTurnNum);   // only call from processor
	virtual void	FinishEndOfTurn(long inTurnNum);	// recalc ship's move so it draws correctly
	virtual long	GetOneAttackAgainst(AGalacticThingy* inDefender);
	virtual long	TakeDamage(long inHowMuch);
	virtual long	RepairDamage(long inHowMuch);
	virtual void	Scrapped();		// take action upon being scrapped
	virtual bool	Departing(Waypoint &inFrom);
	virtual bool	Arriving(Waypoint &inTo);
	virtual void	Die();
	void			CalcMoveDelta();

// ---- miscellaneous methods ----
	virtual bool	RefersTo(AGalacticThingy *inThingy);
	virtual void	RemoveReferencesTo(AGalacticThingy *inThingy);
	virtual void    ChangeIdReferences(PaneIDT oldID, PaneIDT newID);

// =========== GUI ONLY ============
#ifndef GALACTICA_SERVER

// ---- mouse handling methods ----
	virtual void 	DoubleClickSelf();
	virtual Boolean	DragSelf(SMouseDownEvent &inMouseDown);

// ---- selection methods ----
//	virtual void 	Select();

// ---- drawing methods ----
	virtual bool    ShouldDraw() const;
	virtual float	GetBrightness(short inDisplayMode);
	virtual Point	GetDrawOffsetFromCenter() const;
	virtual Rect	GetCenterRelativeFrame(bool full = false) const;
	virtual void 	DrawSelf(void);
#endif // GALACTICA_SERVER

protected:
	UInt32		mSpeed;
	UInt32		mRemDist;
	UInt32		mPower;
	UInt8		mClass;
	Point3D		mMoveDelta;
	CourseT		mCourse;
	CourseT		mCourseMem;
	bool		mCourseChanged;	// *** this only indicates changes by the USER. ***
	bool		mUserCallIn;	// *** indicates whether user has set callin button
	int			mSelectedWPNum;	// temp use only, for which item is hilited in a list
	bool        mHasMoved;
private:
	void	    ReadShipStream(LStream &inStream);
	void        SetWaypoint(int inWaypointNum, Waypoint inWaypoint);  // low level, no bounds checking

#ifndef GALACTICA_SERVER
	virtual void	MoveToNamePos(const Rect &frame);	// only called from ANamedThingy::DrawSelf()	
#endif // GALACTICA_SERVER
};


#endif // CSHIP_H_INCLUDED

