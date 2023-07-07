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
static simd_float4x4 gRightEyeMatrix = {.columns = {
                                            {1, 0, 0, 0},
                                            {0, 1, 0, 0},
                                            {0, 0, 1, 0},
                                            {0.065, 0, 0, 1},
                                        }};
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

struct ar_pose_internal {
  uint32_t unk_0[4]; // NSObject
  simd_float4x4 transform; // 0x10
  simd_float4x4 transform_2; // 0x50
};
static_assert(sizeof(struct ar_pose_internal) == 0x90, "OS_ar_pose size is wrong");
typedef struct ar_pose_internal* ar_pose_internal_t;

// cp_view_texture_map_get_texture_index
struct cp_view_texture_map {
  size_t texture_index;  // 0x0
  size_t slice_index;    // 0x8
  MTLViewport viewport;  // 0x10
};

static const int kTakeScreenshotStatusIdle = 0;
static const int kTakeScreenshotStatusScreenshotNextFrame = 1;
static const int kTakeScreenshotStatusScreenshotInProgress = 2;

// TODO(zhuowei): do I need locking for this?
static int gTakeScreenshotStatus = kTakeScreenshotStatusIdle;

// TODO(zhuowei): multiple screenshots in flight
static cp_drawable_t gHookedDrawable;

id<MTLTexture> gHookedExtraTexture = nil;
static id<MTLTexture> gHookedExtraDepthTexture = nil;
static id<MTLTexture> gHookedExtraScrapTexture = nil;
static id<MTLTexture> gHookedExtraScrapDepthTexture = nil;

id<MTLTexture> gHookedRealTexture = nil;

static void DumpScreenshot(void);

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

void RETransformComponentSetWorldMatrix4x4F(simd_float4x4 a);
static void hook_RETransformComponentSetWorldMatrix4x4F(simd_float4x4 a)
{
  //printf("asdf\n");
  gWorldMat = a;
  RETransformComponentSetWorldMatrix4x4F(a);
}

DYLD_INTERPOSE(hook_RETransformComponentSetWorldMatrix4x4F, RETransformComponentSetWorldMatrix4x4F);

static cp_drawable_t hook_cp_frame_query_drawable(cp_frame_t frame) {
  cp_drawable_t retval = cp_frame_query_drawable(frame);
  gHookedDrawable = nil;
  if (!gHookedExtraTexture) {
    // only make this once
    id<MTLDevice> metalDevice = MTLCreateSystemDefaultDevice();
    id<MTLTexture> originalTexture = cp_drawable_get_color_texture(retval, 0);
    id<MTLTexture> originalDepthTexture = cp_drawable_get_depth_texture(retval, 0);
    gHookedExtraTexture = MakeOurTextureBasedOnTheirTexture(metalDevice, originalTexture);
    gHookedExtraDepthTexture = MakeOurTextureBasedOnTheirTexture(metalDevice, originalDepthTexture);
    gHookedExtraScrapTexture = MakeOurTextureBasedOnTheirTexture(metalDevice, originalTexture);
    gHookedExtraScrapDepthTexture =
        MakeOurTextureBasedOnTheirTexture(metalDevice, originalDepthTexture);
  }
  //if (gTakeScreenshotStatus == kTakeScreenshotStatusScreenshotNextFrame) 
  {
    gTakeScreenshotStatus = kTakeScreenshotStatusScreenshotInProgress;
    gHookedDrawable = retval;
    gHookedRealTexture = cp_drawable_get_color_texture(retval, 0);
    //NSLog(@"visionos_stereo_screenshots starting screenshot!");
  }
  ar_pose_internal_t pose = (__bridge ar_pose_internal_t)cp_drawable_get_ar_pose(retval);
  cp_view_t leftView = cp_drawable_get_view(retval, 0);
  cp_view_t rightView = cp_drawable_get_view(retval, 1);
  memcpy(rightView, leftView, sizeof(*leftView));
  
  cp_view_get_view_texture_map(rightView)->texture_index = 1;

  openxr_headset_data xr_data;
  openxr_headset_get_data(&xr_data);

  // Apple
  // X is -left/+right
  // Y is +up/-down
  // Z is -forward/+back
  /*leftView->transform.columns[3][0] = xr_data.l_x;
  leftView->transform.columns[3][1] = xr_data.l_y;
  leftView->transform.columns[3][2] = xr_data.l_z * 2.0;
  leftView->transform.columns[3][3] = 0.0;

  rightView->transform.columns[3][0] = xr_data.r_x;
  rightView->transform.columns[3][1] = xr_data.r_y;
  rightView->transform.columns[3][2] = xr_data.r_z * 2.0;
  rightView->transform.columns[3][3] = 0.0;*/

  simd_float4x4 view_mat_l;
  simd_float4x4 view_mat_r;

  if (xr_data.view_l)
  {
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        //printf("%u,%u: %f\n", i,j, pose->transform.columns[i][j]);
        //pose->transform.columns[i][j] = xr_data.view_l[(i*4)+j];
        view_mat_l.columns[i][j] = xr_data.view_l[(i*4)+j];
        //leftView->transform.columns[i][j] = xr_data.view_l[(i*4)+j];
        /*if (j == 2) {
          leftView->transform.columns[i][j] *= -1.0;
        }*/
      }
    }
  }

  if (xr_data.view_r)
  {
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++)
      {
        view_mat_r.columns[i][j] = xr_data.view_r[(i*4)+j];
        /*if (j == 2) {
          rightView->transform.columns[i][j] *= -1.0;
        }*/
      }
    }
  }

  leftView->height_maybe = 0.0f;
  rightView->height_maybe = 0.0f;

  //gWorldMat = view_mat_l;
  //gWorldMat.columns[3][1] += 0.0;
  gRightEyeMatrix.columns[3][0] = view_mat_r.columns[3][0] - view_mat_l.columns[3][0];
  //gRightEyeMatrix.columns[3][1] = view_mat_r.columns[3][1] - view_mat_l.columns[3][1];
  //gRightEyeMatrix.columns[3][2] = view_mat_r.columns[3][2] - view_mat_l.columns[3][2];
  //leftView->transform.columns[3][1] += 2.0;
  //rightView->transform.columns[3][1] += 2.0;

  printf("%f\n", gWorldMat.columns[3][1]);

  if (gWorldMat.columns[3][1] < 0.0) {

  }

  //view_mat_l.columns[3][1] -= gWorldMat.columns[3][1];
  //view_mat_r.columns[3][1] -= gWorldMat.columns[3][1];
  
  for (int i = 0; i < 0x20; i += 4)
  {
    //printf("%x: %x %x %x %x\n", i, leftView->unknown[i+0], leftView->unknown[i+1], leftView->unknown[i+2], leftView->unknown[i+3]);
  }
  
  

  //float fovAngleHorizontal = 108 * M_PI / 180;
  //float fovAngleVertical = CalculateFovY(fovAngleHorizontal, 2732 / 2048.f);

  /*
  leftLens = (
        angleUp = 42,
        angleDown = 54,
        angleLeft = 54,
        angleRight = 40,
        info8Unk3p0 = -0.0246586,
        info8Unk3p1 = -0,
        info8Unk4p0 = 0.0234743,
        info8Unk4p1 = -0.0030586,
        info8Unk5p0 = 0,
        info8Unk5p1 = 0.0234743,
        info8Unk6p0 = 0.0432,
        info8Unk6p1 = 0.04608 ),
      rightLens = (
        angleUp = 42,
        angleDown = 54,
        angleLeft = 40,
        angleRight = 54,
  */

  /*float fovAngleLeft_l = -fovAngleHorizontal / 2;
  float fovAngleRight_l = fovAngleHorizontal / 2;
  float fovAngleTop_l = fovAngleVertical / 2;
  float fovAngleBottom_l = -fovAngleVertical / 2;

  float fovAngleLeft_r = -fovAngleHorizontal / 2;
  float fovAngleRight_r = fovAngleHorizontal / 2;
  float fovAngleTop_r = fovAngleVertical / 2;
  float fovAngleBottom_r = -fovAngleVertical / 2;*/

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

  leftView->tangents = tangents_l;
  rightView->tangents = tangents_r;

  if (xr_data.tangents_l)
  {
    simd_float4 tangents_l = simd_make_float4(tanf(-xr_data.tangents_l[0]), tanf(xr_data.tangents_l[1]),
                                              tanf(xr_data.tangents_l[2]), tanf(-xr_data.tangents_l[3]));
    leftView->tangents = tangents_l;
  }
  
  if (xr_data.tangents_r)
  {
    simd_float4 tangents_r = simd_make_float4(tanf(-xr_data.tangents_r[0]), tanf(xr_data.tangents_r[1]),
                                              tanf(xr_data.tangents_r[2]), tanf(-xr_data.tangents_r[3]));
    rightView->tangents = tangents_r;
  }

  leftView->transform = view_mat_l;
  rightView->transform = view_mat_r;
  //leftView->transform = gIdentityMat;
  //rightView->transform = gRightEyeMatrix;

  return retval;
}

DYLD_INTERPOSE(hook_cp_frame_query_drawable, cp_frame_query_drawable);

static void hook_cp_drawable_encode_present(cp_drawable_t drawable,
                                            id<MTLCommandBuffer> command_buffer) {
  if (gHookedDrawable == drawable) {
    [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
      //DumpScreenshot();
    }];
  }

  //NSLog(@"visionos_stereo_screenshot: present");
  openxr_set_textures(gHookedRealTexture, gHookedExtraTexture, gHookedExtraTexture.width, gHookedExtraTexture.height);
  openxr_loop();

  return cp_drawable_encode_present(drawable, command_buffer);
}

DYLD_INTERPOSE(hook_cp_drawable_encode_present, cp_drawable_encode_present);

static size_t hook_cp_drawable_get_view_count(cp_drawable_t drawable) { return 2; }

DYLD_INTERPOSE(hook_cp_drawable_get_view_count, cp_drawable_get_view_count);

static size_t hook_cp_drawable_get_texture_count(cp_drawable_t drawable) { return 2; }

DYLD_INTERPOSE(hook_cp_drawable_get_texture_count, cp_drawable_get_texture_count);

static id<MTLTexture> hook_cp_drawable_get_color_texture(cp_drawable_t drawable, size_t index) {
  if (index == 1) {
    return drawable == gHookedDrawable ? gHookedExtraTexture : gHookedExtraScrapTexture;
  }
  return cp_drawable_get_color_texture(drawable, 0);
}

DYLD_INTERPOSE(hook_cp_drawable_get_color_texture, cp_drawable_get_color_texture);

static id<MTLTexture> hook_cp_drawable_get_depth_texture(cp_drawable_t drawable, size_t index) {
  if (index == 1) {
    return drawable == gHookedDrawable ? gHookedExtraDepthTexture : gHookedExtraScrapDepthTexture;
  }
  return cp_drawable_get_depth_texture(drawable, 0);
}

DYLD_INTERPOSE(hook_cp_drawable_get_depth_texture, cp_drawable_get_depth_texture);

size_t cp_layer_properties_get_view_count(cp_layer_renderer_properties_t properties);

static size_t hook_cp_layer_properties_get_view_count(cp_layer_renderer_properties_t properties) {
  return 2;
}

DYLD_INTERPOSE(hook_cp_layer_properties_get_view_count, cp_layer_properties_get_view_count);

cp_layer_renderer_layout cp_layer_configuration_get_layout_private(
    cp_layer_renderer_configuration_t configuration);

static cp_layer_renderer_layout hook_cp_layer_configuration_get_layout_private(
    cp_layer_renderer_configuration_t configuration) {
  return cp_layer_renderer_layout_dedicated;
}

DYLD_INTERPOSE(hook_cp_layer_configuration_get_layout_private,
               cp_layer_configuration_get_layout_private);

static void DumpScreenshot() {
  NSLog(@"visionos_stereo_screenshot: DumpScreenshot");
  gTakeScreenshotStatus = kTakeScreenshotStatusIdle;

  size_t textureDataSize = gHookedExtraTexture.width * gHookedExtraTexture.height * 4;
  NSMutableData* outputData = [NSMutableData dataWithLength:textureDataSize];
  [gHookedRealTexture
           getBytes:outputData.mutableBytes
        bytesPerRow:gHookedRealTexture.width * 4
      bytesPerImage:textureDataSize
         fromRegion:MTLRegionMake2D(0, 0, gHookedRealTexture.width, gHookedRealTexture.height)
        mipmapLevel:0
              slice:0];
  CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  CGDataProviderRef provider = CGDataProviderCreateWithCFData((__bridge CFDataRef)outputData);
  CGImageRef cgImage = CGImageCreate(
      gHookedRealTexture.width, gHookedRealTexture.height, /*bitsPerComponent=*/8,
      /*bitsPerPixel=*/32, /*bytesPerRow=*/gHookedRealTexture.width * 4, colorSpace,
      kCGImageByteOrder32Little | kCGImageAlphaPremultipliedFirst, provider, /*decode=*/nil,
      /*shouldInterpolate=*/false, /*intent=*/kCGRenderingIntentDefault);

  NSMutableData* outputData2 = [NSMutableData dataWithLength:textureDataSize];
  [gHookedExtraTexture
           getBytes:outputData2.mutableBytes
        bytesPerRow:gHookedExtraTexture.width * 4
      bytesPerImage:textureDataSize
         fromRegion:MTLRegionMake2D(0, 0, gHookedExtraTexture.width, gHookedExtraTexture.height)
        mipmapLevel:0
              slice:0];
  CGDataProviderRef provider2 = CGDataProviderCreateWithCFData((__bridge CFDataRef)outputData2);
  CGImageRef cgImage2 = CGImageCreate(
      gHookedExtraTexture.width, gHookedExtraTexture.height, /*bitsPerComponent=*/8,
      /*bitsPerPixel=*/32, /*bytesPerRow=*/gHookedExtraTexture.width * 4, colorSpace,
      kCGImageByteOrder32Little | kCGImageAlphaPremultipliedFirst, provider2, /*decode=*/nil,
      /*shouldInterpolate=*/false, /*intent=*/kCGRenderingIntentDefault);

  CGContextRef cgContext = CGBitmapContextCreate(
      nil, gHookedExtraTexture.width * 2, gHookedExtraTexture.height, /*bitsPerComponent=*/8,
      /*bytesPerRow=*/0, colorSpace, kCGImageByteOrder32Little | kCGImageAlphaPremultipliedFirst);
  CGContextDrawImage(
      cgContext, CGRectMake(0, 0, gHookedRealTexture.width, gHookedRealTexture.height), cgImage);
  CGContextDrawImage(cgContext,
                     CGRectMake(gHookedRealTexture.width, 0, gHookedExtraTexture.width,
                                gHookedExtraTexture.height),
                     cgImage2);
  CGImageRef outputImage = CGBitmapContextCreateImage(cgContext);

  NSString* filePath =
      [NSString stringWithFormat:@"/tmp/visionos_stereo_screenshot_%ld.png", time(nil)];
  ;

  CGImageDestinationRef destination =
      CGImageDestinationCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:filePath],
                                      (__bridge CFStringRef)UTTypePNG.identifier, 1, nil);
  CGImageDestinationAddImage(destination, outputImage, nil);
  bool success = CGImageDestinationFinalize(destination);

  if (success) {
    NSLog(@"visionos_stereo_screenshots wrote screenshot to %@", filePath);
  } else {
    NSLog(@"visionos_stereo_screenshots failed to write screenshot to %@", filePath);
  }

  CFRelease(destination);
  CFRelease(outputImage);
  CFRelease(cgContext);
  CFRelease(cgImage);
  CFRelease(cgImage2);
  CFRelease(colorSpace);
  CFRelease(provider);
  CFRelease(provider2);
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
    if (gTakeScreenshotStatus == kTakeScreenshotStatusIdle) {
      gTakeScreenshotStatus = kTakeScreenshotStatusScreenshotNextFrame;
      NSLog(@"visionos_stereo_screenshots preparing to take screenshot!");
    }
  });
  signal(SIGUSR1, SIG_IGN);
  dispatch_activate(signal_source);

  //openxr_main();
}

void __attribute__ ((constructor)) premain(void) {
#ifdef __x86_64__
    register void* r15 asm("r15");  //dyld4::gDyld
    register void* (*typed_dlopen)( void*, char const*, int) asm("rax");
    register void* (*typed_dlsym)(void*, char const*, void*) asm("rcx");
    __asm volatile(".intel_syntax noprefix;"
                   "mov rax,[r15];"
                   "mov rcx,[rax + 0x88];"
                   "mov rax,[rax + 0x70];"
                   : "=r"(typed_dlopen), "=r"(r15), "=r"(typed_dlsym)); //dyld4::gDyld
    void* handle = typed_dlopen(r15, "libc.dylib", RTLD_NOW);
    int (*myPrintf)(const char * __restrict, ...) = typed_dlsym(r15, handle, "printf");

    myPrintf("Hello world");
#endif
#if TARGET_CPU_ARM64
    register void* x8 asm("x8");  //dyld4::gDyld
    register void* (*typed_dlopen)( void*, char const*, int) asm("x0");
    register void* (*typed_dlsym)(void*, char const*, void*) asm("x1");

    __asm volatile("ldr x0,[x8] \t\n"
                   "ldr x1,[x0, 0x88]\t\n"
                   "ldr x0,[x0, 0x70]\t\n"
                   : "=r"(typed_dlopen), "=r"(x8), "=r"(typed_dlsym)); //dyld4::gDyld

    void* handle = typed_dlopen(x8, "libc.dylib", RTLD_NOW);
    int (*myPrintf)(const char * __restrict, ...) = typed_dlsym(x8, handle, "printf");
    void* searched_dlopen = typed_dlsym(x8, handle, "dlopen");

    myPrintf("Hello world");

    NSLog(@"stdout typed_dlopen %p dlopen %p searched_dlopen %p handle %p", typed_dlopen, dlopen, searched_dlopen, handle);
#endif
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