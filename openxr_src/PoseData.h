#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

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

class PoseData final
{
public:
  PoseData();
  ~PoseData();

  XrActionStateFloat grab_value[HAND_COUNT];
  XrActionStateFloat grip_value[HAND_COUNT];
  XrActionStateBoolean system_value[HAND_COUNT];
  XrActionStateBoolean b_y_value[HAND_COUNT];
  XrSpaceLocation tracked_locations[64];

  bool pinch_l;
  bool pinch_r;
  bool system_button;
  bool menu_button;
  bool left_touch_button = false;
  bool right_touch_button = false;

  glm::quat l_eye_quat;
  glm::quat r_eye_quat;
  glm::mat4 l_eye_mat;
  glm::mat4 r_eye_mat;

  XrHandJointVelocitiesEXT leftVelocities;
  XrHandJointLocationsEXT leftLocations;
  XrHandJointVelocitiesEXT rightVelocities;
  XrHandJointLocationsEXT rightLocations;

  XrHandJointLocationEXT leftJointLocations[XR_HAND_JOINT_COUNT_EXT];
  XrHandJointVelocityEXT leftJointVelocities[XR_HAND_JOINT_COUNT_EXT];

  XrHandJointLocationEXT rightJointLocations[XR_HAND_JOINT_COUNT_EXT];
  XrHandJointVelocityEXT rightJointVelocities[XR_HAND_JOINT_COUNT_EXT];

  std::vector<XrView> eyePoses;
  std::vector<glm::mat4> eyeViewMatrices;
  std::vector<glm::mat4> eyeProjectionMatrices;
  float eyeTangents_l[4];
  float eyeTangents_r[4];

  std::mutex eyePoseMutex;
};