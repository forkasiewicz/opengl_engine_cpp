#version 330 core

in vec3 vColor;

out vec4 FragColor;

uniform vec3 u_customColor;

void main()
{
  // FragColor = vec4(vColor * u_customColor, 1.0);
  FragColor = vec4(u_customColor, 1.0);
}
