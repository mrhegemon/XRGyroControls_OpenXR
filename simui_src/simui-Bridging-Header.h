#ifdef __cplusplus
extern "C"
{
#endif

#include "simui_types.h"

void ObjCBridge_Startup();
void ObjCBridge_Shutdown();
sharedmem_data* ObjCBridge_Loop();

//#ifdef EYE_CURSOR

#ifdef __cplusplus
}
#endif