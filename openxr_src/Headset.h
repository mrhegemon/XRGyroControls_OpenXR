#pragma once

#include <glm/mat4x4.hpp>

#include <vulkan/vulkan.h>

#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>
#include <mutex>

class Context;
class RenderTarget;

#define HAND_LEFT_INDEX (0)
#define HAND_RIGHT_INDEX (1)
#define HAND_COUNT (2)

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
  BeginFrameResult beginFrame(uint32_t& swapchainImageIndex);
  void endFrame() const;

  bool isValid() const;
  bool isExitRequested() const;
  VkRenderPass getRenderPass() const;
  size_t getEyeCount() const;
  VkExtent2D getEyeResolution(size_t eyeIndex) const;
  glm::mat4 getEyeViewMatrix(size_t eyeIndex) const;
  glm::mat4 getEyeProjectionMatrix(size_t eyeIndex) const;
  RenderTarget* getRenderTarget(size_t swapchainImageIndex) const;

  XrActionStateFloat grab_value[HAND_COUNT];
  XrSpaceLocation tracked_locations[64];
  XrActionSet gameplay_actionset;

  bool pinch_l;
  bool pinch_r;

  glm::mat4 l_eye_mat;
  glm::mat4 r_eye_mat;

  XrPath grip_pose_path[HAND_COUNT];
  XrPath haptic_path[HAND_COUNT];
  XrPath thumbstick_y_path[HAND_COUNT];
  XrPath trigger_value_path[HAND_COUNT];
  XrPath select_click_path[HAND_COUNT];

  XrHandTrackerEXT leftHandTracker;
  XrHandTrackerEXT rightHandTracker;

  XrHandJointLocationEXT leftJointLocations[XR_HAND_JOINT_COUNT_EXT];
  XrHandJointVelocityEXT leftJointVelocities[XR_HAND_JOINT_COUNT_EXT];

  XrHandJointLocationEXT rightJointLocations[XR_HAND_JOINT_COUNT_EXT];
  XrHandJointVelocityEXT rightJointVelocities[XR_HAND_JOINT_COUNT_EXT];

  XrHandJointVelocitiesEXT leftVelocities;
  XrHandJointLocationsEXT leftLocations;

  XrHandJointVelocitiesEXT rightVelocities;
  XrHandJointLocationsEXT rightLocations;

  std::vector<XrView> eyePoses;
  std::vector<glm::mat4> eyeViewMatrices;
  std::vector<glm::mat4> eyeProjectionMatrices;
  float eyeTangents_l[4];
  float eyeTangents_r[4];

  std::mutex eyePoseMutex;

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
  XrFrameState frameState = {};
  XrViewState viewState = {};

  std::vector<XrViewConfigurationView> eyeImageInfos;
  std::vector<XrCompositionLayerProjectionView> eyeRenderInfos;

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
  XrAction haptic_action;
  XrPath hand_paths[HAND_COUNT];

  bool beginSession() const;
  bool endSession() const;
};