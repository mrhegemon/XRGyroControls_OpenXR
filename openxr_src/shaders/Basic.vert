#extension GL_EXT_multiview : enable

layout(binding = 0) uniform UniformBufferObject
{
    mat4 world;
    mat4 viewProjection[2];
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 position; // In world space
layout(location = 2) out vec2 uv;

void main()
{
  vec4 pos = vec4(inPosition.x, inPosition.y, inPosition.z, 1.0);
  gl_Position = pos;
  color = inColor;
  position = pos.xyz;
  uv = inUV;
}