#version 330 core

in vec4 f_color;
invariant out vec4 FragColor;

void main() {
    FragColor = f_color;
}
