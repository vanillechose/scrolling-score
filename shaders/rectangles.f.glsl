#version 460 core

flat in float tick_start_out;
flat in float tick_end_out;

flat in vec2  radius;
     in vec2  pos;

uniform float t;

out vec4 color;

void main() {
    bool playing = tick_start_out < t && t <= tick_end_out;

    float c = playing ? 1.0 : 0.0;
    color = vec4(1.0, c, c, 1.0);
}
