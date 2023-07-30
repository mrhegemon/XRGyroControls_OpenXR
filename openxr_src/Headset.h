#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vulkan/vulkan.h>

#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>
#include <mutex>

#include "PoseData.h"

class Context;
class RenderTarget;

#define HAND_LEFT_INDEX (0)
#define HAND_RIGHT_INDEX (1)
#define HAND_COUNT (2)
#define STORED_POSE_COUNT (6)

class Headset final
{
public:
  Headset();
  Headset(const Context* context);
  ~Headset();

  enum class BeginFrameResult
  {
    Error,       // An error occurred
    RenderFully, // Render this frame normally
    SkipRender,  // Skip rendering the frame but end it
    SkipFully    // Skip processing this frame entirely without ending it
  };
  BeginFrameResult beginFrame(int* pPoseIdx);
  void beginFrameRender(uint32_t& swapchainImageIndex);
  void endFrame(int poseIdx) const;

  bool isValid() const;
  bool isExitRequested() const;
  VkRenderPass getRenderPass() const;
  size_t getEyeCount() const;
  VkExtent2D getEyeResolution(size_t eyeIndex) const;
  RenderTarget* getRenderTarget(size_t swapchainImageIndex) const;

  XrActionSet gameplay_actionset;

  XrPath grip_pose_path[HAND_COUNT];
  XrPath thumbstick_y_path[HAND_COUNT];
  XrPath trigger_value_path[HAND_COUNT];
  XrPath grip_value_path[HAND_COUNT];
  XrPath select_click_path[HAND_COUNT];
  XrPath system_click_path[HAND_COUNT];
  XrPath menu_click_path[HAND_COUNT];
  XrPath b_click_path[HAND_COUNT];
  XrPath y_click_path[HAND_COUNT];

  XrHandTrackerEXT leftHandTracker;
  XrHandTrackerEXT rightHandTracker;

  std::mutex eyePoseMutex;

  PoseData storedPoses[STORED_POSE_COUNT];
  int lastPoseIdx;

private:
  bool valid = true;
  bool exitRequested = false;
  bool left_hand_valid = false;
  bool right_hand_valid = false;

  const Context* context = nullptr;

  size_t eyeCount = 0u;

  XrSession session = nullptr;
  XrSessionState sessionState = XR_SESSION_STATE_UNKNOWN;
  XrSpace space = nullptr;

  std::vector<XrViewConfigurationView> eyeImageInfos;

  XrSwapchain swapchain = nullptr;
  std::vector<RenderTarget*> swapchainRenderTargets;

  VkRenderPass renderPass = nullptr;

  // Depth buffer
  VkImage depthImage = nullptr;
  VkDeviceMemory depthMemory = nullptr;
  VkImageView depthImageView = nullptr;

  XrAction hand_pose_action;
  XrSpace hand_pose_spaces[HAND_COUNT];
  XrAction grab_action_float;
  XrAction grip_action_float;
  XrAction system_action_bool;
  XrAction b_y_action_bool;
  XrPath hand_paths[HAND_COUNT];

  bool beginSession() const;
  bool endSession() const;
};