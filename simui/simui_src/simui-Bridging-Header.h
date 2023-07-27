#ifdef __cplusplus
extern "C"
{
#endif

#include "simui_types.h"

void ObjCBridge_Startup();
void ObjCBridge_Shutdown();
sharedmem_data* ObjCBridge_Loop();
void ObjCBridge_HomeButtonPress();
void ObjCBridge_HomeButtonPressUp();

//#ifdef EYE_CURSOR

int my_LMGetKbdType();

#ifdef __cplusplus
}
#endif