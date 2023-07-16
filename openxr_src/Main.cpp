#include "../openxr-Bridging-Header.h"
#include "../simui_src/simui_types.h"
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
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <thread>
#include <deque>
#include "Util.h"

uint32_t swapchainImageIndex;
int frame_started = 0;
std::mutex renderMutex;
std::mutex renderMutex2;
std::mutex dataMutex;

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

MTLTexture_id g_tex_l;
MTLTexture_id g_tex_r;
uint32_t g_tex_w = 0;
uint32_t g_tex_h = 0;
Context* context = nullptr;
Headset* headset = nullptr;
Renderer* renderer = nullptr;
int openxr_is_done = 0;
std::thread* render_thread;
void* shm_addr;

 std::chrono::time_point<std::chrono::high_resolution_clock> last_loop;

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

extern "C" void render_loop()
{
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  while (1)
  {
    openxr_pre_loop();
    //openxr_loop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}


extern "C" int openxr_init()
{
  setlinebuf(stdout);
  setlinebuf(stderr);
  stdout->_write = stdout_redirect_nslog;
  stderr->_write = stderr_redirect_nslog;
  //setenv("XR_RUNTIME_JSON", "/Users/maxamillion/workspace/XRGyroControls_OpenXR_2/openxr_monado-dev.json", 1);

  frame_started = 0;
  renderMutex2.lock();

  char tmp[1024];
  snprintf(tmp, sizeof(tmp), "%s/IOUSBLib_ios_hax.dylib", getenv("XRHAX_ROOTDIR") ? getenv("XRHAX_ROOTDIR") : ".");
  const char* pszModule = tmp;
  void* h = dlopen(pszModule, RTLD_GLOBAL);
  if (h == NULL)
  {
      printf("Failed to dlopen %s.  %s\n", pszModule, dlerror() );
  }

  {
    // Open the shared memory, with required access mode and permissions.
    // This is analagous to how we open file with open()
    int fd = shm_open("/tmp/Sim2OpenXR_shmem", O_CREAT | O_RDWR /* open flags */, S_IRUSR | S_IWUSR /* mode */);

    // extend or shrink to the required size (specified in bytes)
    ftruncate(fd, 1048576); // picking 1MB size as an example

    // map the shared memory to an address in the virtual address space
    // with the file descriptor of the shared memory and necessary protection.
    // The flags to mmap must be set to MAP_SHARED for other process to access.
    shm_addr = mmap(NULL, 1048576, PROT_READ | PROT_WRITE /*protection*/, MAP_SHARED /*flags*/, fd, 0);
  
    printf("SimUI sharedmem startup: %u %p %x\n", fd, shm_addr, *(uint32_t*)shm_addr);
  }

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

  renderer = new Renderer(context, headset, g_tex_l, g_tex_r, g_tex_w, g_tex_h);
  if (!renderer || !renderer->isValid())
  {
    printf("XRHax: create Renderer failed!\n");
    return EXIT_FAILURE;
  }

  printf("XRHax: OpenXR init success!\n");

  //render_thread = new std::thread(render_loop);

  return EXIT_SUCCESS;
}

extern "C" int openxr_set_textures(MTLTexture_id tex_l, MTLTexture_id tex_r, uint32_t w, uint32_t h)
{
  g_tex_l = tex_l;
  g_tex_r = tex_r;
  g_tex_w = w;
  g_tex_h = h;

  if (!context || !context->isValid()) {
    if(openxr_init() != EXIT_SUCCESS) {
      openxr_is_done = 1;
      return 1;
    }
  }

  if (!renderer) return 1;

  renderer->metal_tex_l = g_tex_l;
  renderer->metal_tex_r = g_tex_r;
  renderer->metal_tex_w = g_tex_w;
  renderer->metal_tex_h = g_tex_h;
  return 0;
}

extern "C" int openxr_pre_loop()
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

  if (frame_started) {
    return 0;
  }

  const Headset::BeginFrameResult frameResult = headset->beginFrame(swapchainImageIndex);
  if (frameResult == Headset::BeginFrameResult::Error)
  {
    openxr_is_done = 1;
    return 1;
  }

  frame_started = 1;

  return 0;
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

  if (!frame_started) return 0;

  static int throttle_prints = 0;
  auto elapsed = std::chrono::high_resolution_clock::now() - last_loop;
  long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
  if (++throttle_prints > 240) {
    printf("%zu microseconds loop\n", (size_t)microseconds);
    throttle_prints = 0;
  }

  last_loop = std::chrono::high_resolution_clock::now();

  //else if (frameResult == Headset::BeginFrameResult::RenderFully)
  {
    renderer->render(swapchainImageIndex);
    renderer->submit(false);
  }

  //if (frameResult == Headset::BeginFrameResult::RenderFully || frameResult == Headset::BeginFrameResult::SkipRender)
  {
    headset->endFrame();
  }
  //printf("XRHax: OpenXR loop done\n");

  frame_started = 0;
  

  //context.sync(); // Sync before destroying so that resources are free
  return 0;
}

extern "C" int openxr_full_loop()
{
  uint32_t local_swapchainImageIndex = 0;
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

  //renderMutex.lock();

  //std::lock_guard<std::mutex> guard(renderMutex);

  static int throttle_prints = 0;
  auto elapsed = std::chrono::high_resolution_clock::now() - last_loop;
  long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
  if (++throttle_prints > 240) {
    printf("%zu microseconds loop\n", (size_t)microseconds);
    throttle_prints = 0;
  }
  last_loop = std::chrono::high_resolution_clock::now();


  const Headset::BeginFrameResult frameResult = headset->beginFrame(local_swapchainImageIndex);
  dataMutex.unlock();
  if (frameResult == Headset::BeginFrameResult::Error)
  {
    openxr_is_done = 1;
    return 1;
  }
  else if (frameResult == Headset::BeginFrameResult::RenderFully)
  {
    //printf("wait for frame...\n");
    for (int i = 0; i < 32; i++)
    {
      if (renderMutex2.try_lock()) {
        break;
      }
      //dataMutex.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    renderer->render(local_swapchainImageIndex);
    renderer->submit(false);
  }

  if (frameResult == Headset::BeginFrameResult::RenderFully || frameResult == Headset::BeginFrameResult::SkipRender)
  {
    headset->endFrame();
  }
  //printf("XRHax: OpenXR loop done\n");

  //context.sync(); // Sync before destroying so that resources are free
  renderMutex.unlock();
  //printf("releasing lock\n");

  return 0;
}

extern "C" void openxr_spawn_renderframe()
{
  for (int i = 0; i < 32; i++)
  {
    if (renderMutex.try_lock()) {
      //printf("spawn render...\n");
      std::thread(openxr_full_loop).detach();
      return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  //printf("couldn't spawn render...\n");
  dataMutex.unlock();
}

extern "C" void openxr_complete_renderframe()
{
  //printf("frame completed, unlocking\n");
  renderMutex2.unlock();
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

std::deque<glm::vec3> z_vec_history;

float offset_x, offset_y, offset_z;
bool offsets_set = false;

extern "C" void openxr_headset_get_data(openxr_headset_data* out)
{
  if (!out) return;
  out->view_l = NULL;
  out->view_r = NULL;
  out->proj_l = NULL;
  out->proj_r = NULL;
  out->tangents_l = NULL;
  out->tangents_r = NULL;
  if (!headset) return;

  dataMutex.lock();

  glm::mat4 ctrl_l = util::poseToMatrix(headset->tracked_locations[0].pose);
  glm::mat4 ctrl_r = util::poseToMatrix(headset->tracked_locations[1].pose);

  //std::lock_guard<std::mutex> guard(headset->eyePoseMutex);

  const XrView& eyePose0 = headset->eyePoses.at(0);
  const XrView& eyePose1 = headset->eyePoses.at(1);

  if (!offsets_set) {
    offset_x = eyePose0.pose.position.x;
    offset_y = eyePose0.pose.position.y + 1.3;
    offset_z = eyePose0.pose.position.z - 0.5;
    offsets_set = true;
  }

  out->l_x  = eyePose0.pose.position.x - offset_x;
  out->l_y  = eyePose0.pose.position.y - offset_y;
  out->l_z  = eyePose0.pose.position.z - offset_z;
  out->l_qx = eyePose0.pose.orientation.x;
  out->l_qy = eyePose0.pose.orientation.y;
  out->l_qz = eyePose0.pose.orientation.z;
  out->l_qw = eyePose0.pose.orientation.w;

  out->r_x  = eyePose1.pose.position.x - offset_x;
  out->r_y  = eyePose1.pose.position.y - offset_y;
  out->r_z  = eyePose1.pose.position.z - offset_z;
  out->r_qx = eyePose1.pose.orientation.x;
  out->r_qy = eyePose1.pose.orientation.y;
  out->r_qz = eyePose1.pose.orientation.z;
  out->r_qw = eyePose1.pose.orientation.w;

  out->view_l = glm::value_ptr(headset->eyeViewMatrices.at(0));
  out->view_r = glm::value_ptr(headset->eyeViewMatrices.at(1));
  out->proj_l = glm::value_ptr(headset->eyeProjectionMatrices.at(0));
  out->proj_r = glm::value_ptr(headset->eyeProjectionMatrices.at(1));
  out->tangents_l = headset->eyeTangents_l;
  out->tangents_r = headset->eyeTangents_r;

  out->view_l[12] = out->l_x;
  out->view_l[13] = out->l_y;
  out->view_l[14] = out->l_z;
  out->view_r[12] = out->r_x;
  out->view_r[13] = out->r_y;
  out->view_r[14] = out->r_z;

  if (shm_addr) {
    sharedmem_data* dat = (sharedmem_data*)shm_addr;
    dat->l_x  = out->l_x;
    dat->l_y  = out->l_y;
    dat->l_z  = out->l_z;
    dat->l_qx = out->l_qx;
    dat->l_qy = out->l_qy;
    dat->l_qz = out->l_qz;
    dat->l_qw = out->l_qw;

    dat->r_x  = out->r_x;
    dat->r_y  = out->r_y;
    dat->r_z  = out->r_z;
    dat->r_qx = out->r_qx;
    dat->r_qy = out->r_qy;
    dat->r_qz = out->r_qz;
    dat->r_qw = out->r_qw;

    glm::quat l_q(dat->l_qx, dat->l_qy, dat->l_qz, dat->l_qw);
    glm::vec3 l_euler = glm::eulerAngles(l_q);// * 3.14159f / 180.f;

    /*if ( l_euler.x > 3.14159f/2.0 )
        l_euler.x = (3.14159f - l_euler.x);
    if ( l_euler.y > 3.14159f/2.0 )
        l_euler.y = (l_euler.y - 3.14159f);
    if ( l_euler.z > 3.14159f/2.0 )
        l_euler.z = (3.14159f - l_euler.z);*/

    dat->l_ep = l_euler.z;
    dat->l_ey = l_euler.y;
    dat->l_er = l_euler.x;

    glm::quat r_q(dat->r_qx, dat->r_qy, dat->r_qz, dat->r_qw);
    glm::vec3 r_euler = glm::eulerAngles(r_q);// * 3.14159f / 180.f; 

    /*if ( r_euler.x > 3.14159f/2.0 )
        r_euler.x = (3.14159f - r_euler.x);
    if ( r_euler.y > 3.14159f/2.0 )
        r_euler.y = (3.14159f - r_euler.y);
    if ( r_euler.z > 3.14159f/2.0 )
        r_euler.z = (3.14159f - r_euler.z);*/

    dat->r_ep = r_euler.z;
    dat->r_ey = r_euler.y;
    dat->r_er = r_euler.x;

    memcpy(dat->l_view, out->view_l, sizeof(dat->l_view));
    memcpy(dat->r_view, out->view_r, sizeof(dat->r_view));

    //dat->grab_val[0] = headset->pinch_l ? 0.9 : 0.0;//headset->grab_value[0].currentState;
    //dat->grab_val[1] = headset->pinch_r ? 0.9 : 0.0;//headset->grab_value[1].currentState;
    dat->grab_val[0] = headset->grab_value[0].currentState;
    dat->grab_val[1] = headset->grab_value[1].currentState;
    dat->grip_val[0] = headset->grip_value[0].currentState;
    dat->grip_val[1] = headset->grip_value[1].currentState;

    memcpy(dat->l_controller, glm::value_ptr(ctrl_l), sizeof(dat->l_controller));
    memcpy(dat->r_controller, glm::value_ptr(ctrl_r), sizeof(dat->r_controller));
    //printf("grabs %f %f\n", dat->grab_val[0], dat->grab_val[1]);

    memcpy(dat->gaze_mat, glm::value_ptr(headset->l_eye_mat), sizeof(dat->gaze_mat));

    glm::vec3 z_vec = glm::vec3(-dat->gaze_mat[8], -dat->gaze_mat[9], -dat->gaze_mat[10]);

    dat->l_controller[12] -= offset_x;
    dat->l_controller[13] -= offset_y;
    dat->l_controller[14] -= offset_z;
    dat->r_controller[12] -= offset_x;
    dat->r_controller[13] -= offset_y;
    dat->r_controller[14] -= offset_z;

    dat->system_button = headset->system_button ? 1 : 0;
    dat->menu_button = headset->menu_button ? 1 : 0;
    dat->left_touch_button = headset->left_touch_button ? 1 : 0;
    dat->right_touch_button = headset->right_touch_button ? 1 : 0;

    //dat->gaze_mat[12] -= offset_x;
    //dat->gaze_mat[13] -= offset_y;
    //dat->gaze_mat[14] -= offset_z;
    //printf("system %x\n", dat->system_button);

#if 0
    float filter_alpha = 0.15;
    float filter_alpha_iter = (1.0 - filter_alpha);
    glm::vec3 z_vec_filtered = z_vec * filter_alpha;
    /*for (int i = 0; i < z_vec_history.size(); i++) {
      z_vec_filtered += z_vec_history[i] * filter_alpha_iter;
      filter_alpha_iter *= filter_alpha_iter;
    }*/

    if (z_vec_history.size()) {
       z_vec_filtered += z_vec_history[z_vec_history.size()-1] * filter_alpha_iter;
    }
    else {
      z_vec_filtered = z_vec;
    }
   

    z_vec_history.push_back(z_vec_filtered);

    if (z_vec_history.size() > 10) {
      z_vec_history.pop_front();
    }

    z_vec_filtered = glm::normalize(z_vec_filtered);
#endif

    memcpy(dat->gaze_vec, glm::value_ptr(z_vec), sizeof(dat->gaze_vec));

    //printf("Left:  %f %f %f %f\n", z_vec_filtered[0], z_vec_filtered[1], z_vec_filtered[2]);
  }

  memcpy(out->l_controller, glm::value_ptr(ctrl_l), sizeof(out->l_controller));
  memcpy(out->r_controller, glm::value_ptr(ctrl_r), sizeof(out->r_controller));

  out->l_controller[12] -= offset_x;
  out->l_controller[13] -= offset_y;
  out->l_controller[14] -= offset_z;
  out->r_controller[12] -= offset_x;
  out->r_controller[13] -= offset_y;
  out->r_controller[14] -= offset_z;
}