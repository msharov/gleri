#version 330 core

uniform sampler2D Texture;
uniform vec4 Color;
in vec2 f_tex;
out vec4 FragColor;

void main() {
    FragColor = vec4(Color.rgb,Color.a*texture2D(Texture,f_tex).r);
}
