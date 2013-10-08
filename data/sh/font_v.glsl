#version 330 core

uniform mat4 Transform;
uniform vec4 FontSize;
layout(location=0) in vec4 Vertex;
out GeomVertex { vec4 pos; vec4 tex; } g;

void main() {
    vec4 glyphtl = Transform*vec4(Vertex.xy,1,1);
    vec4 glyphbr = Transform*vec4(Vertex.xy+FontSize.xy,1,1);
    g.pos = vec4(glyphtl.xy,glyphbr.xy);
    vec2 textl = Vertex.zw+vec2(.5,.5);
    g.tex = vec4(textl,textl+FontSize.xy)/FontSize.zwzw;
}
