// UStarUtils.h
// 4/15/99 ERZ

// for every star, decide
//  danger level -- ask for help or give help
//	expansion factor

class StarAI {
	
typedef float StarMoveFactorT;

StarMoveFactorT* NewStarDecisionMatrix();
void	ConsiderFactor(int factor, );
void	ConsiderMultiplierFactor(int factor, int shiptype, long delta, MoveFactorT &ioDecisionMatrix);
