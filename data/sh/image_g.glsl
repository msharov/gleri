#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices=4) out;

in GeomVertex { vec4 pos; vec4 tex; } g[1];
invariant out vec4 gl_Position;
invariant out vec2 f_tex;

void main() {
    gl_Position = vec4(g[0].pos.xy,1,1);
    f_tex = g[0].tex.xy;
    EmitVertex();
    gl_Position = vec4(g[0].pos.xw,1,1);
    f_tex = g[0].tex.xw;
    EmitVertex();
    gl_Position = vec4(g[0].pos.zy,1,1);
    f_tex = g[0].tex.zy;
    EmitVertex();
    gl_Position = vec4(g[0].pos.zw,1,1);
    f_tex = g[0].tex.zw;
    EmitVertex();
    EndPrimitive();
}
