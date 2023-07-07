#extension GL_EXT_multiview : enable

layout(location = 0) in vec3 color;
layout(location = 1) in vec3 position;
layout(location = 2) in vec2 uv;

layout (binding = 1) uniform sampler2D tex0;
layout (binding = 2) uniform sampler2D tex1;

layout(location = 0) out vec4 outColor;

void main()
{
  //outColor = vec4(color - position.y * 0.001, 1.0);

  if (gl_ViewIndex == 0)
  {
    outColor = vec4(texture(tex0, uv).rgb, 1.0);
  }
  else {
    outColor = vec4(texture(tex1, uv).rgb, 1.0);
  }
  
  //outColor = vec4(uv, 0.0, 1.0);
  //outColor = vec4(color * texture(tex0, uv).rgb, 1.0);
}