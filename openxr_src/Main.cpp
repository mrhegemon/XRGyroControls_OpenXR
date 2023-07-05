#include "../openxr-Bridging-Header.h"
#include "Context.h"
#include "Headset.h"
#include "Renderer.h"
#include <unistd.h>
#include <stdlib.h>

extern "C"
{
  int redirect_nslog(const char *prefix, const char *buffer, int size);
  int stderr_redirect_nslog(void *inFD, const char *buffer, int size);
  int stdout_redirect_nslog(void *inFD, const char *buffer, int size);
}

Context* context = nullptr;
Headset* headset = nullptr;
Renderer* renderer = nullptr;
int openxr_is_done = 0;

#define DO_MIRROR

extern "C" int openxr_init();

extern "C" int openxr_main()
{
  printf("XRHax: OpenXR main!\n");
  //openxr_init();
  //while (!openxr_loop()) {}
  printf("XRHax: OpenXR main done!\n");
  return 0;
}


extern "C" int openxr_init()
{
  setlinebuf(stdout);
  setlinebuf(stderr);
  stdout->_write = stdout_redirect_nslog;
  stderr->_write = stderr_redirect_nslog;
  //setenv("XR_RUNTIME_JSON", "/Users/maxamillion/workspace/XRGyroControls_OpenXR_2/openxr_monado-dev.json", 1);

  printf("XRHax: OpenXR init!\n");
  context = new Context();
  if (!context || !context->isValid())
  {
    return EXIT_FAILURE;
  }

  printf("XRHax: createDevice!\n");

  if (!context->createDevice())
  {
    return EXIT_FAILURE;
  }

  printf("XRHax: create Headset!\n");

  headset = new Headset(context);
  if (!headset || !headset->isValid())
  {
    return EXIT_FAILURE;
  }

  printf("XRHax: create Renderer!\n");

  renderer = new Renderer(context, headset);
  if (!renderer || !renderer->isValid())
  {
    printf("XRHax: create Renderer failed!\n");
    return EXIT_FAILURE;
  }

  printf("XRHax: OpenXR init success!\n");

  return EXIT_SUCCESS;
}

extern "C" int openxr_loop()
{
  //printf("XRHax: OpenXR loop start\n");
  if (!context || !context->isValid()) {
    if(openxr_init() != EXIT_SUCCESS) {
      openxr_is_done = 1;
      return 1;
    }
  }

  // Main loop
  if (headset->isExitRequested()) {
    openxr_is_done = 1;
    return 1;
  }

  uint32_t swapchainImageIndex;
  const Headset::BeginFrameResult frameResult = headset->beginFrame(swapchainImageIndex);
  if (frameResult == Headset::BeginFrameResult::Error)
  {
    openxr_is_done = 1;
    return 1;
  }
  else if (frameResult == Headset::BeginFrameResult::RenderFully)
  {
    renderer->render(swapchainImageIndex);
    renderer->submit(false);
  }

  if (frameResult == Headset::BeginFrameResult::RenderFully || frameResult == Headset::BeginFrameResult::SkipRender)
  {
    headset->endFrame();
  }
  //printf("XRHax: OpenXR loop done\n");

  //context.sync(); // Sync before destroying so that resources are free
  return 0;
}

extern "C" int openxr_done()
{
  return openxr_is_done;
}

extern "C" int openxr_cleanup()
{
  if (!context) return 0;
  context->sync();

  delete renderer;
  delete headset;
  delete context;

  context = nullptr;
  headset = nullptr;
  renderer = nullptr;
  return 0;
}

extern "C" void openxr_headset_get_data(openxr_headset_data* out)
{
  if (!headset) return;

  const XrView& eyePose0 = headset->eyePoses.at(0);
  const XrView& eyePose1 = headset->eyePoses.at(1);

  out->x = (eyePose0.pose.position.x + eyePose1.pose.position.x) / 2.0;
  out->y = (eyePose0.pose.position.y + eyePose1.pose.position.y) / 2.0;
  out->z = (eyePose0.pose.position.z + eyePose1.pose.position.z) / 2.0;

  out->qx = eyePose0.pose.orientation.x;
  out->qy = eyePose0.pose.orientation.y;
  out->qz = eyePose0.pose.orientation.z;
  out->qw = eyePose0.pose.orientation.w;
}