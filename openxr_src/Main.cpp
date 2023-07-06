#include "../openxr-Bridging-Header.h"
#include "Context.h"
#include "Headset.h"
#include "Renderer.h"
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>

extern "C"
{
  #include "../libusb/libusb/libusb.h"

  int redirect_nslog(const char *prefix, const char *buffer, int size);
  int stderr_redirect_nslog(void *inFD, const char *buffer, int size);
  int stdout_redirect_nslog(void *inFD, const char *buffer, int size);

  //extern void dyld_all_image_infos();
  void *getDyldBase(void) {
    struct task_dyld_info dyld_info;
    mach_vm_address_t image_infos;
    struct dyld_all_image_infos *infos;
    
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    kern_return_t ret;
    
    ret = task_info(mach_task_self_,
                    TASK_DYLD_INFO,
                    (task_info_t)&dyld_info,
                    &count);
    
    if (ret != KERN_SUCCESS) {
        return NULL;
    }
    
    image_infos = dyld_info.all_image_info_addr;
    
    infos = (struct dyld_all_image_infos *)image_infos;
    return (void*)infos->dyldImageLoadAddress;
}

static void print_devs(libusb_device **devs)
{
  libusb_device *dev;
  int i = 0, j = 0;
  uint8_t path[8]; 

  while ((dev = devs[i++]) != NULL) {
    struct libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0) {
      fprintf(stderr, "failed to get device descriptor");
      return;
    }

    printf("%04x:%04x (bus %d, device %d)",
      desc.idVendor, desc.idProduct,
      libusb_get_bus_number(dev), libusb_get_device_address(dev));

    r = libusb_get_port_numbers(dev, path, sizeof(path));
    if (r > 0) {
      printf(" path: %d", path[0]);
      for (j = 1; j < r; j++)
        printf(".%d", path[j]);
    }
    printf("\n");
  }
}

int libusb_test_main(void)
{
  libusb_device **devs;
  int r;
  ssize_t cnt;

  r = libusb_init(/*ctx=*/NULL);
  if (r < 0)
    return r;

  cnt = libusb_get_device_list(NULL, &devs);
  printf("Found %u devices\n", cnt);
  if (cnt < 0){
    libusb_exit(NULL);
    return (int) cnt;
  }

  print_devs(devs);
  libusb_free_device_list(devs, 1);

  libusb_exit(NULL);
  return 0;
}
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

#if 1
  const char* pszModule = "/Users/maxamillion/workspace/XRGyroControls_OpenXR_2/IOUSBLib_ios_hax.dylib";
  void* h = dlopen(pszModule, RTLD_GLOBAL);
  if (h == NULL)
  {
      printf("Failed to dlopen %s.  %s\n", pszModule, dlerror() );
  }
  void* test_dyld = dlsym(h, "IOUSBLibFactory");
  printf("Found symbol? %p\n", test_dyld);

  //libusb_test_main();

  /*int count = _dyld_image_count();
  for (int i = 0; i < count; i++)
  {
    printf("%s\n", _dyld_get_image_name(i));
  }*/
  //printf("%p\n", getDyldBase());
  //printf("%08x\n", *(uint32_t*)getDyldBase());
  //printf("%08x\n", *(uint32_t*)((intptr_t)getDyldBase() + 0x0003d94c));

  //void* (*test_dlsym)(void*, const char*) = ( void* (*)(void*, const char*))((intptr_t)getDyldBase() + 0x0003d94c);
  

  //test_dyld = test_dlsym(RTLD_DEFAULT, "dlopen");
  //printf("Found symbol? %p %p\n", test_dyld, dlopen);
#endif
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