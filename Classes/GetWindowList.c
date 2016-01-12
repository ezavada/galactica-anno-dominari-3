#include <MacWindows.h>
#include <LowMem.h>

EXTERN_API( WindowRef )
GetWindowList(void)
{
	return LMGetWindowList();
}
