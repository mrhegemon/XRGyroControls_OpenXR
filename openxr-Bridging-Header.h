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
} openxr_headset_data;

#ifdef __OBJC__
@protocol MTLTexture;
typedef id<MTLTexture> MTLTexture_id;
#else
typedef void* MTLTexture_id;
#endif

int openxr_main();
int openxr_pre_loop();
int openxr_loop();
int openxr_full_loop();
void openxr_spawn_renderframe();
void openxr_complete_renderframe();
int openxr_cleanup();
int openxr_done();
void openxr_headset_get_data(openxr_headset_data* out);
int openxr_set_textures(MTLTexture_id tex_l, MTLTexture_id tex_r, uint32_t w, uint32_t h);

#ifdef __cplusplus
}
#endif