//#include "GalacticaConsts.h"
#include "GalacticaTutorial.h"

class TutorialBuilderApp : public LDocApplication {
public:
						TutorialBuilderApp();		// constructor registers all PPobs
	
	virtual void		OpenDocument(FSSpec *inMacFSSpec);
	void				SavePageToResFile(LWindow *inPageWind, short inRefNum);
	short 				FindUnusedResourceID(ResType inType, short startWith = 128);
protected:
	bool		mPrevPageHadButton;
};

