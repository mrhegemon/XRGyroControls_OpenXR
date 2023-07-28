#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef struct openxr_headset_data
{
  float l_x;
  float l_y;
  float l_z;

  float l_qx;
  float l_qy;
  float l_qz;
  float l_qw;

  float r_x;
  float r_y;
  float r_z;

  float r_qx;
  float r_qy;
  float r_qz;
  float r_qw;

  float* view_l;
  float* view_r;
  float* proj_l;
  float* proj_r;
  float* tangents_l;
  float* tangents_r;

  float l_controller[16];
  float r_controller[16];

  float view_r_rel[16];
} openxr_headset_data;

//#define EYE_CURSOR

#ifdef __OBJC__
@protocol MTLTexture;
typedef id<MTLTexture> MTLTexture_id;
#else
typedef void* MTLTexture_id;
#endif

int openxr_main();
int openxr_full_loop(int which);
void openxr_spawn_renderframe(int which);
void openxr_complete_renderframe(int which);
int openxr_cleanup();
int openxr_done();
void openxr_headset_get_data(openxr_headset_data* out, int which);
int openxr_set_textures(MTLTexture_id* paTex_l, MTLTexture_id* paTex_r, uint32_t w, uint32_t h);

#ifdef __cplusplus
}
#endif