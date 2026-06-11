#version 330 core

out vec4 frag_color;
uniform vec3 custom_color;

void main()
{
  frag_color = vec4(custom_color, 1.0);
}
