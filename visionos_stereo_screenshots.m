@import CompositorServices;
@import Darwin;
@import ObjectiveC;
@import UniformTypeIdentifiers;
//#include <Metal/Metal.h>
//#include <simd/matrix_types.h>
//#include <CompositorServices/drawable.h>

#include "openxr-Bridging-Header.h"

#define DYLD_INTERPOSE(_replacement, _replacee)                                           \
  __attribute__((used)) static struct {                                                   \
    const void* replacement;                                                              \
    const void* replacee;                                                                 \
  } _interpose_##_replacee __attribute__((section("__DATA,__interpose,interposing"))) = { \
      (const void*)(unsigned long)&_replacement, (const void*)(unsigned long)&_replacee}

// 6.5cm translation on x
static simd_float4x4 gIdentityMat = {.columns = {
                                            {1, 0, 0, 0},
                                            {0, 1, 0, 0},
                                            {0, 0, 1, 0},
                                            {0, 0, 0, 1},
                                        }};
static simd_float4x4 gWorldMat = {.columns = {
                                            {1, 0, 0, 0},
                                            {0, 1, 0, 0},
                                            {0, 0, 1, 0},
                                            {0, 2.0, 0, 1},
                                        }};

static simd_float4x4 left_controller_pose;
static simd_float4x4 right_controller_pose;

static simd_float4 left_eye_pos;
static simd_float4 left_eye_quat;
static simd_float4 left_eye_zbasis;
static simd_float4 right_eye_pos;
static simd_float4 right_eye_quat;

// cp_drawable_get_view
struct cp_view {
  simd_float4x4 transform;     // 0x0
  simd_float4 tangents;        // 0x40
  uint32_t unknown_1[4];  // 0x50 (0x110 - 0x50)/4
  uint32_t unknown_2[4];  // 0x60 (0x110 - 0x50)/4
  float unknown_3[4];  // 0x70 (0x110 - 0x50)/4
  float unknown_4[3];  // 0x80 (0x110 - 0x50)/4
  float height_maybe;
  uint32_t unknown[0x20];  // 0x90 (0x110 - 0x90)/4
};
static_assert(sizeof(struct cp_view) == 0x110, "cp_view size is wrong");

// cp_view_texture_map_get_texture_index
struct cp_view_texture_map {
  size_t texture_index;  // 0x0
  size_t slice_index;    // 0x8
  MTLViewport viewport;  // 0x10
};

struct RSSimulatedHeadsetPose {
  simd_float4 position;
  simd_float4 rotation;
};

/*
typedef struct cp_time {
  uint64_t cp_mach_abs_time;
} cp_time;
*/

static id<MTLTexture> gOrigTextureList[3] = {nil,nil,nil};

// TODO(zhuowei): multiple screenshots in flight
static cp_drawable_t gHookedDrawable[3] = {nil,nil,nil};;

static id<MTLTexture> gHookedRightTexture[3] = {nil,nil,nil};
static id<MTLTexture> gHookedRightDepthTexture[3] = {nil,nil,nil};
static id<MTLTexture> gHookedLeftTexture[3] = {nil,nil,nil};
static id<MTLTexture> gHookedLeftDepthTexture[3] = {nil,nil,nil};

static id<MTLTexture> gHookedSimulatorPreviewTexture[3] = {nil,nil,nil};

#define NUM_VIEWS (2)

@interface RSSimulatedHeadset
- (void)getEyePose:(struct RSSimulatedHeadsetPose*)pose:(int)forEye;
- (void)setEyePose:(struct RSSimulatedHeadsetPose)pose:(int)forEye;
- (void)setHMDPose:(struct RSSimulatedHeadsetPose)pose;
@end

RSSimulatedHeadset* gHookedSimulatedHeadset = nil;
SEL gHookedSimulatedHeadset_sel;
static void (*real_RSSimulatedHeadset_setHMDPose)(RSSimulatedHeadset* self, SEL sel,
                                                  struct RSSimulatedHeadsetPose pose);

void cp_drawable_set_pose(cp_drawable_t,simd_float4x4);
void cp_drawable_set_simd_pose(cp_drawable_t,simd_float4x4);

static float CalculateFovY(float fovX, float aspect) {
  // https://cs.android.com/android/platform/superproject/+/master:external/exoplayer/tree/library/ui/src/main/java/com/google/android/exoplayer2/ui/spherical/SphericalGLSurfaceView.java;l=328;drc=2c30c028bf6b958edf635b3e4e1079e64737250a
  float halfFovX = fovX / 2;
  float tanY = tanf(halfFovX) / aspect;
  float halfFovY = atanf(tanY);
  return halfFovY * 2;
}

static id<MTLTexture> MakeOurTextureBasedOnTheirTexture(id<MTLDevice> device,
                                                        id<MTLTexture> originalTexture) {
  MTLTextureDescriptor* descriptor =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:originalTexture.pixelFormat
                                                         width:originalTexture.width
                                                        height:originalTexture.height
                                                     mipmapped:false];
  descriptor.storageMode = originalTexture.storageMode;
  return [device newTextureWithDescriptor:descriptor];
}

#if 0
static ar_pose_t hook_cp_drawable_get_ar_pose(cp_drawable_t drawable) {
  ar_pose_t retval = cp_drawable_get_ar_pose(drawable);
  printf("%p\n", retval);
  return retval;
}

DYLD_INTERPOSE(hook_cp_drawable_get_ar_pose, cp_drawable_get_ar_pose);

static void hook_cp_drawable_set_simd_pose(cp_drawable_t drawable, simd_float4x4 pose) {
  cp_drawable_set_simd_pose(drawable, pose);
  printf("set_simd_pose %p\n", drawable);
}

DYLD_INTERPOSE(hook_cp_drawable_set_simd_pose, cp_drawable_set_simd_pose);
#endif

int _os_feature_enabled_impl(const char* a, const char* b);
static int hook__os_feature_enabled_impl(const char* a, const char* b)
{
  int ret = _os_feature_enabled_impl(a,b);
  if (!strcmp(a, "RealitySimulation") && !strcmp(b, "OverlapRenderAndSimulation")) {
    //ret = 1;
  }
  //printf("Feature: %s, %s -> %x\n", a, b, ret);
  return ret;
}
DYLD_INTERPOSE(hook__os_feature_enabled_impl, _os_feature_enabled_impl);

void RETransformComponentSetWorldMatrix4x4F(simd_float4x4 a);
static void hook_RETransformComponentSetWorldMatrix4x4F(simd_float4x4 a)
{
  gWorldMat = a;
  //RETransformComponentSetWorldMatrix4x4F(gIdentityMat);
  RETransformComponentSetWorldMatrix4x4F(a);
}
DYLD_INTERPOSE(hook_RETransformComponentSetWorldMatrix4x4F, RETransformComponentSetWorldMatrix4x4F);

#if 0
static void* hook_cp_frame_predict_timing(void* a)
{
  return cp_frame_predict_timing(a);
}

static struct cp_time hook_cp_frame_timing_get_presentation_time(void* a)
{
  // +0x68 double 0.008333 (120Hz)
  // +0x28 frame idx
  //printf("testing %f\n", *(double*)((intptr_t)a + 0x68));
  //printf("testing %d\n", *(int*)((intptr_t)a + 0x28));

  //struct cp_time val = {mach_absolute_time()};
  //return val;

  return cp_frame_timing_get_presentation_time(a);
}
DYLD_INTERPOSE(hook_cp_frame_timing_get_presentation_time, cp_frame_timing_get_presentation_time);

static struct cp_time hook_cp_frame_timing_get_rendering_deadline(void* a)
{
  //struct cp_time val = {mach_absolute_time()};
  //return val;

  return cp_frame_timing_get_rendering_deadline(a);
}
DYLD_INTERPOSE(hook_cp_frame_timing_get_rendering_deadline, cp_frame_timing_get_rendering_deadline);

extern int cp_frame_timing_get_frame_repeat_count(void* a);
static int hook_cp_frame_timing_get_frame_repeat_count(void* a)
{
  return 0;
}
DYLD_INTERPOSE(hook_cp_frame_timing_get_frame_repeat_count, cp_frame_timing_get_frame_repeat_count);
#endif

#if 0
void RERenderManagerWaitForFramePacing(void* ctx);
void hook_RERenderManagerWaitForFramePacing(void* ctx)
{

}
DYLD_INTERPOSE(hook_RERenderManagerWaitForFramePacing, RERenderManagerWaitForFramePacing);
#endif

extern int cp_layer_get_state(void* a);
static int hook_cp_layer_get_state(void* a)
{
  return cp_layer_get_state(a);
}
DYLD_INTERPOSE(hook_cp_layer_get_state, cp_layer_get_state);

int num_buffers_collected()
{
  int ret = 0;
  for (int i = 0; i < 3; i++)
  {
    if (gOrigTextureList[i]) {
      ret++;
    }
  }
  return ret;
}

int which_buffer_is_this(id<MTLTexture> originalTexture)
{
  if (!originalTexture) return -1;

  for (int i = 0; i < 3; i++)
  {
    if (gOrigTextureList[i] == originalTexture) {
      return i;
    }
  }

  for (int i = 0; i < 3; i++)
  {
    if (gOrigTextureList[i] == nil) {
      gOrigTextureList[i] = originalTexture;
      return i;
    }
  }

  return -1;
}

static int last_which = -1;

static cp_drawable_t hook_cp_frame_query_drawable(cp_frame_t frame) {
  int which_guess = (last_which + 1) % 3;
  // We do this first, because query_drawable might stall...?
  openxr_headset_data xr_data;
  memset(&xr_data, 0, sizeof(xr_data));
  if (num_buffers_collected() >= 3)
  {
    openxr_set_textures(&gHookedLeftTexture, &gHookedRightTexture, gHookedRightTexture[0].width, gHookedRightTexture[0].height);

    openxr_spawn_renderframe(which_guess);

    // this will wait on headset data, this pose MUST be synced with the frame send w/ xrEndFrame
    openxr_headset_get_data(&xr_data, which_guess);
  }

  cp_drawable_t retval = cp_frame_query_drawable(frame);
  int which = which_buffer_is_this(cp_drawable_get_color_texture(retval, 0));
  last_which = which;
  if (!gHookedRightTexture[which]) {
    // only make this once
    id<MTLDevice> metalDevice = MTLCreateSystemDefaultDevice();
    id<MTLTexture> originalTexture = cp_drawable_get_color_texture(retval, 0);
    id<MTLTexture> originalDepthTexture = cp_drawable_get_depth_texture(retval, 0);
    
    gHookedRightTexture[which] = MakeOurTextureBasedOnTheirTexture(metalDevice, originalTexture);
    gHookedRightDepthTexture[which] = MakeOurTextureBasedOnTheirTexture(metalDevice, originalDepthTexture);
    gHookedLeftTexture[which] = MakeOurTextureBasedOnTheirTexture(metalDevice, originalTexture);
    gHookedLeftDepthTexture[which] = MakeOurTextureBasedOnTheirTexture(metalDevice, originalDepthTexture);
  }

  gHookedDrawable[which] = retval;
  gHookedSimulatorPreviewTexture[which] = cp_drawable_get_color_texture(retval, 0);

  cp_view_t simView = cp_drawable_get_view(retval, 0);
  cp_view_t rightView = cp_drawable_get_view(retval, 1);
  memcpy(rightView, simView, sizeof(*simView));

  cp_view_get_view_texture_map(rightView)->texture_index = 1;
  cp_view_get_view_texture_map(simView)->texture_index = 2; // HACK: move the sim view to the left eye texture index

  // Apple
  // X is -left/+right
  // Y is +up/-down
  // Z is -forward/+back
  left_eye_pos[0] = xr_data.l_x;
  left_eye_pos[1] = xr_data.l_y;
  left_eye_pos[2] = xr_data.l_z;
  left_eye_pos[3] = 0.0;

  left_eye_quat[0] = xr_data.l_qx;
  left_eye_quat[1] = xr_data.l_qy;
  left_eye_quat[2] = xr_data.l_qz;
  left_eye_quat[3] = xr_data.l_qw;

  right_eye_pos[0] = xr_data.r_x;
  right_eye_pos[1] = xr_data.r_y;
  right_eye_pos[2] = xr_data.r_z;
  right_eye_pos[3] = 0.0;

  right_eye_quat[0] = xr_data.r_qx;
  right_eye_quat[1] = xr_data.r_qy;
  right_eye_quat[2] = xr_data.r_qz;
  right_eye_quat[3] = xr_data.r_qw;

  simd_float4x4 view_mat_l;
  simd_float4x4 view_mat_r;

  if (xr_data.view_l)
  {
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        view_mat_l.columns[i][j] = xr_data.view_l[(i*4)+j];
      }
    }
  }

  if (xr_data.view_r)
  {
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        view_mat_r.columns[i][j] = xr_data.view_r_rel[(i*4)+j];
      }
    }
  }

  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      left_controller_pose.columns[i][j] = xr_data.l_controller[(i*4)+j];
      right_controller_pose.columns[i][j] = xr_data.r_controller[(i*4)+j];
    }
  }

  simView->height_maybe = 0.0f;
  rightView->height_maybe = 0.0f;


  float fovAngleLeft_l   = -42 * M_PI / 180;
  float fovAngleRight_l  =  40 * M_PI / 180;
  float fovAngleTop_l    =  54 * M_PI / 180;
  float fovAngleBottom_l = -54 * M_PI / 180;

  float fovAngleLeft_r   = -40 * M_PI / 180;
  float fovAngleRight_r  =  42 * M_PI / 180;
  float fovAngleTop_r    =  54 * M_PI / 180;
  float fovAngleBottom_r = -54 * M_PI / 180;

  // https://git.sr.ht/~sircmpwn/wxrc/tree/master/item/src/xrutil.c#L313
  simd_float4 tangents_l = simd_make_float4(tanf(-fovAngleLeft_l), tanf(fovAngleRight_l),
                                          tanf(fovAngleTop_l), tanf(-fovAngleBottom_l));
  simd_float4 tangents_r = simd_make_float4(tanf(-fovAngleLeft_r), tanf(fovAngleRight_r),
                                          tanf(fovAngleTop_r), tanf(-fovAngleBottom_r));

  simView->tangents = tangents_l;
  rightView->tangents = tangents_r;

  if (xr_data.tangents_l)
  {
    simd_float4 tangents_l = simd_make_float4(tanf(-xr_data.tangents_l[0]), tanf(xr_data.tangents_l[1]),
                                              tanf(xr_data.tangents_l[2]), tanf(-xr_data.tangents_l[3]));
    simView->tangents = tangents_l;
  }
  
  if (xr_data.tangents_r)
  {
    simd_float4 tangents_r = simd_make_float4(tanf(-xr_data.tangents_r[0]), tanf(xr_data.tangents_r[1]),
                                              tanf(xr_data.tangents_r[2]), tanf(-xr_data.tangents_r[3]));
    rightView->tangents = tangents_r;
  }

  //simView->transform = view_mat_l;
  rightView->transform = view_mat_r;
  simView->transform = gIdentityMat;

  left_eye_zbasis[0] = view_mat_l.columns[2][0];
  left_eye_zbasis[1] = view_mat_l.columns[2][1];
  left_eye_zbasis[2] = view_mat_l.columns[2][2];
  left_eye_zbasis[3] = view_mat_l.columns[2][3];

  if (gHookedSimulatedHeadset) {
    struct RSSimulatedHeadsetPose pose;
    pose.position = left_eye_pos;
    pose.rotation = left_eye_quat;

    real_RSSimulatedHeadset_setHMDPose(gHookedSimulatedHeadset, gHookedSimulatedHeadset_sel, pose);
  }

  return retval;
}

DYLD_INTERPOSE(hook_cp_frame_query_drawable, cp_frame_query_drawable);

#if 0
extern void* RERenderFrameSettingsAddGpuWaitEvent(void* a, void* b, void* c);
void* hook_RERenderFrameSettingsAddGpuWaitEvent(void* a, void* b, void* c)
{
  return NULL;
}
DYLD_INTERPOSE(hook_RERenderFrameSettingsAddGpuWaitEvent, RERenderFrameSettingsAddGpuWaitEvent);
#endif

extern void cp_drawable_present(cp_drawable_t drawable);

static void hook_cp_drawable_encode_present(cp_drawable_t drawable,
                                            id<MTLCommandBuffer> command_buffer) {
  int which = which_buffer_is_this(cp_drawable_get_color_texture(drawable, 0));
  //printf("%u\n", which);
  if (gHookedDrawable[which] == drawable && num_buffers_collected() >= 3) {
    [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
      openxr_complete_renderframe(which);
    }];
  }

  id<MTLBlitCommandEncoder> blit = [command_buffer blitCommandEncoder];
  [blit copyFromTexture:gHookedLeftTexture[which] sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(gHookedLeftTexture[which].width, gHookedLeftTexture[which].height, 1) toTexture:gHookedSimulatorPreviewTexture[which] destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
  [blit generateMipmapsForTexture:gHookedSimulatorPreviewTexture[which]];
  [blit endEncoding];

  cp_drawable_encode_present(drawable, command_buffer);
  //cp_drawable_present(drawable);
}

DYLD_INTERPOSE(hook_cp_drawable_encode_present, cp_drawable_encode_present);

#if 0
int RCPHIDEventGetSelectionRay(void* ctx, struct RSSimulatedHeadsetPose* pose);
int hook_RCPHIDEventGetSelectionRay(void* ctx, struct RSSimulatedHeadsetPose* pose) {
  int ret = RCPHIDEventGetSelectionRay(ctx, pose);

#if 0
  pose->position = right_controller_pose.columns[3];
  pose->rotation[0] = -right_controller_pose.columns[2][0];
  pose->rotation[1] = -right_controller_pose.columns[2][1];
  pose->rotation[2] = -right_controller_pose.columns[2][2];

//#ifdef EYE_CURSOR
  pose->position = left_eye_pos;
  //pose->rotation = left_eye_quat;
  pose->rotation[0] = -left_eye_zbasis[0];
  pose->rotation[1] = -left_eye_zbasis[1];
  pose->rotation[2] = -left_eye_zbasis[2];
//#endif
#endif

  //printf("selection ray %u, %f %f %f %f, %f %f %f %f\n", ret, pose->position[0], pose->position[1], pose->position[2], pose->position[3], pose->rotation[0], pose->rotation[1], pose->rotation[2], pose->rotation[3]);
  return ret;
}
DYLD_INTERPOSE(hook_RCPHIDEventGetSelectionRay, RCPHIDEventGetSelectionRay);
#endif

#if 0
void RFAnchorPtrGetIndexTipTransform(void*);
void hook_RFAnchorPtrGetIndexTipTransform(void* a) {
  RFAnchorPtrGetIndexTipTransform(a);
}
DYLD_INTERPOSE(hook_RFAnchorPtrGetIndexTipTransform, RFAnchorPtrGetIndexTipTransform);
#endif

#if 0
extern int __extractTargetTimes(void*, void* ,void*);
int hook__extractTargetTimes(void* a, void* b, void* c)
{
  return __extractTargetTimes(a,b,c);
}
#endif
//DYLD_INTERPOSE(hook__extractTargetTimes, _extractTargetTimes);

#if 0
void __WAITING_FOR_ENCODING_START_TIME_FOR_BEST_HEAD_POSE__();
void hook___WAITING_FOR_ENCODING_START_TIME_FOR_BEST_HEAD_POSE__()
{

}
//DYLD_INTERPOSE(hook___WAITING_FOR_ENCODING_START_TIME_FOR_BEST_HEAD_POSE__, __WAITING_FOR_ENCODING_START_TIME_FOR_BEST_HEAD_POSE__);
#endif

static size_t hook_cp_drawable_get_view_count(cp_drawable_t drawable) { return NUM_VIEWS; }
DYLD_INTERPOSE(hook_cp_drawable_get_view_count, cp_drawable_get_view_count);

static size_t hook_cp_drawable_get_texture_count(cp_drawable_t drawable) { return NUM_VIEWS+1; }
DYLD_INTERPOSE(hook_cp_drawable_get_texture_count, cp_drawable_get_texture_count);

static id<MTLTexture> hook_cp_drawable_get_color_texture(cp_drawable_t drawable, size_t index) {
  int which = which_buffer_is_this(cp_drawable_get_color_texture(drawable, 0));
  if (index == 1) {
    return gHookedRightTexture[which];
  }
  else if (index == 2) {
    return gHookedLeftTexture[which];
  }
  return cp_drawable_get_color_texture(drawable, 0);
}

DYLD_INTERPOSE(hook_cp_drawable_get_color_texture, cp_drawable_get_color_texture);

static id<MTLTexture> hook_cp_drawable_get_depth_texture(cp_drawable_t drawable, size_t index) {
  int which = which_buffer_is_this(cp_drawable_get_color_texture(drawable, 0));
  if (index == 1) {
    return gHookedRightDepthTexture[which];
  }
  else if (index == 2) {
    return gHookedLeftDepthTexture[which];
  }
  return cp_drawable_get_depth_texture(drawable, 0);
}

DYLD_INTERPOSE(hook_cp_drawable_get_depth_texture, cp_drawable_get_depth_texture);

size_t cp_layer_properties_get_view_count(cp_layer_renderer_properties_t properties);

static size_t hook_cp_layer_properties_get_view_count(cp_layer_renderer_properties_t properties) {
  return NUM_VIEWS;
}

DYLD_INTERPOSE(hook_cp_layer_properties_get_view_count, cp_layer_properties_get_view_count);

cp_layer_renderer_layout cp_layer_configuration_get_layout_private(
    cp_layer_renderer_configuration_t configuration);

/*static cp_layer_renderer_layout hook_cp_layer_configuration_get_layout_private(
    cp_layer_renderer_configuration_t configuration) {
  return cp_layer_renderer_layout_dedicated;
}

DYLD_INTERPOSE(hook_cp_layer_configuration_get_layout_private,
               cp_layer_configuration_get_layout_private);*/

#if 0
static void hook_sleep(unsigned int a) {
  //sleep(a);
}
DYLD_INTERPOSE(hook_sleep, sleep);
#endif

static void hook_mach_wait_until(void* a)
{

}
DYLD_INTERPOSE(hook_mach_wait_until, mach_wait_until);

#if 0
void RERenderFrameSettingsSetTotalTime(void* re, float time);
static void hook_RERenderFrameSettingsSetTotalTime(void* re, float time) {
  //printf("_RERenderFrameSettingsSetTotalTime %f\n", time);
  RERenderFrameSettingsSetTotalTime(re, time - 0.03125 + 0.008);
}

DYLD_INTERPOSE(hook_RERenderFrameSettingsSetTotalTime, RERenderFrameSettingsSetTotalTime);
#endif

//RSXRRenderLoop::currentFrequency

#if 0
static double (*real_RSUserSettings_worstAllowedFrameTime)(void* self, double val);
static double hook_RSUserSettings_worstAllowedFrameTime(void* self, double val) {
  double ret = real_RSUserSettings_worstAllowedFrameTime(self, val);
  //printf("worstAllowedFrameTime %f %f\n", val, ret);
  return ret;
}
#endif

#if 0
static int (*real_RSXRRenderLoop_currentFrequency)(void* self);
static int hook_RSXRRenderLoop_currentFrequency(void* self) {
  int ret = real_RSXRRenderLoop_currentFrequency(self);
  //printf("frequency at %d\n", ret);
  //printf("%f\n", *(double *)(self + 0x188));
  //*(int *)(self + 0x140) = 2; //overcomitted
  //*(int *)(self + 0x144) = 3;
  //*(int *)(self + 0x148) = 2;
  //*(int *)(self + 0x13C) = 3;
  //*(double *)(self + 0x188) = 0.08; //max frametime
  //*(int*)((intptr_t)self + 0x1EC) = 120;

  *(double *)(self + 0x150) = -0.000050;
  *(double *)(self + 0x158) = -0.000050;
  *(double *)(self + 0x160) = 0.000000;

  //*(double *)(self + 0x1A0) = 0.000000050;

  /*
  *(double *)(self + 0x150) = 0.000050;
  *(double *)(self + 0x158) = 0.000050;
  *(double *)(self + 0x160) = 2.000000;
  */

  //printf("test %f %f %f\n", *(double *)(self + 0x150), *(double *)(self + 0x158), *(double *)(self + 0x160));
  return 120;
}
#endif

extern int os_workgroup_interval_start(os_workgroup_interval_t wg, uint64_t start, uint64_t deadline, os_workgroup_interval_data_t data);
static int hook_os_workgroup_interval_start(os_workgroup_interval_t wg, uint64_t start, uint64_t deadline, os_workgroup_interval_data_t data)
{
  return os_workgroup_interval_start(wg, start, deadline, data);
}
DYLD_INTERPOSE(hook_os_workgroup_interval_start, os_workgroup_interval_start);

extern int os_signpost_enabled(void* a);
static int hook_os_signpost_enabled(void* a)
{
  return 1;
}
DYLD_INTERPOSE(hook_os_signpost_enabled, os_signpost_enabled);

static void (*real_RSSimulatedHeadset_getEyePose)(RSSimulatedHeadset* self, SEL sel,
                                                  struct RSSimulatedHeadsetPose* pose, int forEye);
static void hook_RSSimulatedHeadset_getEyePose(RSSimulatedHeadset* self, SEL sel,
                                               struct RSSimulatedHeadsetPose* pose, int forEye) {
  real_RSSimulatedHeadset_getEyePose(self, sel, pose, forEye);
  
  /*if (forEye == 0)
  {
    pose->position = left_eye_pos;
    pose->rotation = left_eye_quat;
  }
  else if (forEye == 1)
  {
    pose->position = right_eye_pos;
    pose->rotation = right_eye_quat;
  }*/
  /*else if (forEye == 2)
  {
    pose->position = left_eye_pos;
    pose->rotation = left_eye_quat;
  }*/
  
  //printf("get eye pos %u, %f %f %f %f\n", forEye, pose->position[0], pose->position[1], pose->position[2], pose->position[3]);
}

static void (*real_RSSimulatedHeadset_setEyePose)(RSSimulatedHeadset* self, SEL sel,
                                                  struct RSSimulatedHeadsetPose pose, int forEye);
static void hook_RSSimulatedHeadset_setEyePose(RSSimulatedHeadset* self, SEL sel,
                                               struct RSSimulatedHeadsetPose pose, int forEye) {
  if (forEye == 0)
  {
    pose.position = left_eye_pos;
    pose.rotation = left_eye_quat;
  }
  else if (forEye == 1)
  {
    pose.position = right_eye_pos;
    pose.rotation = right_eye_quat;
  }
  /*else if (forEye == 2)
  {
    pose.position = left_eye_pos;
    pose.rotation = left_eye_quat;
  }*/

  real_RSSimulatedHeadset_setEyePose(self, sel, pose, forEye);
  
  //printf("set eye pos %u, %f %f %f %f\n", forEye, pose.position[0], pose.position[1], pose.position[2], pose.position[3]);
}


static void hook_RSSimulatedHeadset_setHMDPose(RSSimulatedHeadset* self, SEL sel,
                                               struct RSSimulatedHeadsetPose pose) {

  pose.position = left_eye_pos;
  pose.rotation = left_eye_quat;

  gHookedSimulatedHeadset = self;
  gHookedSimulatedHeadset_sel = sel;

  real_RSSimulatedHeadset_setHMDPose(self, sel, pose);
  
  //printf("set eye pos %u, %f %f %f %f\n", forEye, pose.position[0], pose.position[1], pose.position[2], pose.position[3]);
}

__attribute__((constructor)) static void SetupSignalHandler() {
  NSLog(@"visionos_stereo_screenshots starting!");
  static dispatch_queue_t signal_queue;
  static dispatch_source_t signal_source;
  signal_queue = dispatch_queue_create("com.worthdoingbadly.stereoscreenshots.signalqueue",
                                       DISPATCH_QUEUE_SERIAL);
  signal_source =
      dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGUSR1, /*mask=*/0, signal_queue);
  dispatch_source_set_event_handler(signal_source, ^{
    // TODO reuse this?
  });
  signal(SIGUSR1, SIG_IGN);
  dispatch_activate(signal_source);

#if 0
  {
    Class cls = NSClassFromString(@"RSUserSettings");
    Method method = class_getInstanceMethod(cls, @selector(worstAllowedFrameTime));
    real_RSUserSettings_worstAllowedFrameTime = (void*)method_getImplementation(method);
    method_setImplementation(method, (IMP)hook_RSUserSettings_worstAllowedFrameTime);
  }
#endif

  {
    Class cls = NSClassFromString(@"RSSimulatedHeadset");
    Method method = class_getInstanceMethod(cls, @selector(_getEyePose:forEye:));
    real_RSSimulatedHeadset_getEyePose = (void*)method_getImplementation(method);
    method_setImplementation(method, (IMP)hook_RSSimulatedHeadset_getEyePose);
  }

  {
    Class cls = NSClassFromString(@"RSSimulatedHeadset");
    Method method = class_getInstanceMethod(cls, @selector(setEyePose:forEye:));
    real_RSSimulatedHeadset_setEyePose = (void*)method_getImplementation(method);
    method_setImplementation(method, (IMP)hook_RSSimulatedHeadset_setEyePose);
  }

  {
    Class cls = NSClassFromString(@"RSSimulatedHeadset");
    Method method = class_getInstanceMethod(cls, @selector(setHMDPose:));
    real_RSSimulatedHeadset_setHMDPose = (void*)method_getImplementation(method);
    method_setImplementation(method, (IMP)hook_RSSimulatedHeadset_setHMDPose);
  }

#if 0
  {
    Class cls = NSClassFromString(@"RSXRRenderLoop");
    Method method = class_getInstanceMethod(cls, @selector(currentFrequency));
    real_RSXRRenderLoop_currentFrequency = (void*)method_getImplementation(method);
    method_setImplementation(method, (IMP)hook_RSXRRenderLoop_currentFrequency);
  }
#endif

  //openxr_main();
}

int redirect_nslog(const char *prefix, const char *buffer, int size)
{
    NSLog(@"%s (%d bytes): %.*s", prefix, size, size, buffer);
    return size;
}

int stderr_redirect_nslog(void *inFD, const char *buffer, int size)
{
    return redirect_nslog("stderr", buffer, size);
}

int stdout_redirect_nslog(void *inFD, const char *buffer, int size)
{
    return redirect_nslog("stdout", buffer, size);
}