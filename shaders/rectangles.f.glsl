#version 460 core

flat in float tick_start_out;
flat in float tick_end_out;
flat in float note_out;

flat in vec2  radius;
     in vec2  pos;

uniform float t;

out vec4 color;

void main() {
    bool playing = tick_start_out < t && t <= tick_end_out;

    float p = mod(note_out, 12) / 12.0;

#   define PI    3.141592
#   define SHIFT (PI / 3.0) * 2.0
    float r = 0.5 + sin(2.0 * PI * p);
    float g = 0.5 + sin(2.0 * PI * p + SHIFT);
    float b = 0.5 + sin(2.0 * PI * p + 2.0 * SHIFT);

    color = playing ? vec4(1.0, 1.0, 1.0, 1.0) : vec4(r, g, b, 1.0);
}
