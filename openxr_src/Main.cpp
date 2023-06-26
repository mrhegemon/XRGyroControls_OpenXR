#include "../openxr-Bridging-Header.h"
#include "Context.h"
#include "Headset.h"
#include "MirrorView.h"
#include "Renderer.h"

Context* context = nullptr;
MirrorView* mirrorView = nullptr;
Headset* headset = nullptr;
Renderer* renderer = nullptr;
int openxr_is_done = 0;

#define DO_MIRROR

extern "C" int openxr_init();

extern "C" int openxr_main()
{
  printf("OpenXR main!\n");
  //openxr_init();
  //while (!openxr_loop()) {}
  printf("OpenXR main done!\n");
  return 0;
}


extern "C" int openxr_init()
{
  printf("OpenXR init!\n");
  context = new Context();
  if (!context || !context->isValid())
  {
    return EXIT_FAILURE;
  }

#ifdef DO_MIRROR
  mirrorView = new MirrorView(context);
  if (!mirrorView || !mirrorView->isValid())
  {
    return EXIT_FAILURE;
  }

  if (!context->createDevice(mirrorView->getSurface()))
  {
    return EXIT_FAILURE;
  }
#endif

#ifndef DO_MIRROR
  VkSurfaceKHR surface = 0;
  if (!context->createDevice(surface))
  {
    return EXIT_FAILURE;
  }
  delete mirrorView;
#endif

  headset = new Headset(context);
  if (!headset || !headset->isValid())
  {
    return EXIT_FAILURE;
  }

  renderer = new Renderer(context, headset);
  if (!renderer || !renderer->isValid())
  {
    return EXIT_FAILURE;
  }

#ifdef DO_MIRROR
  if (!mirrorView->connect(headset, renderer))
  {
    return EXIT_FAILURE;
  }
#endif

  return EXIT_SUCCESS;
}

extern "C" int openxr_loop()
{
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

#ifdef DO_MIRROR
  if (mirrorView->isExitRequested()) {
    openxr_is_done = 1;
    return 1;
  }

  //mirrorView->processWindowEvents();
#endif

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

#ifdef DO_MIRROR
    const MirrorView::RenderResult mirrorResult = mirrorView->render(swapchainImageIndex);
    if (mirrorResult == MirrorView::RenderResult::Error)
    {
      openxr_is_done = 1;
      return 1;
    }

    const bool mirrorViewVisible = (mirrorResult == MirrorView::RenderResult::Visible);
    renderer->submit(mirrorViewVisible);

    if (mirrorViewVisible)
    {
      mirrorView->present();
    }
#endif
  }

  if (frameResult == Headset::BeginFrameResult::RenderFully || frameResult == Headset::BeginFrameResult::SkipRender)
  {
    headset->endFrame();
  }

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
  delete mirrorView;
  delete context;

  context = nullptr;
  mirrorView = nullptr;
  headset = nullptr;
  renderer = nullptr;
  return 0;
}

extern "C" void openxr_headset_get_data(openxr_headset_data* out)
{
  if (!headset) return;

  const XrView& eyePose = headset->eyePoses.at(0);

  out->x = eyePose.pose.position.x;
  out->y = eyePose.pose.position.y;
  out->z = eyePose.pose.position.z;

  out->pitch = eyePose.pose.orientation.x;
  out->yaw = eyePose.pose.orientation.y;
  out->roll = eyePose.pose.orientation.z;
}