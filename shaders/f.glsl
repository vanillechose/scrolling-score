#version 460 core

in float tick_start_out;
in float tick_end_out;

uniform float tick;

#define PLAYING_COLOR vec4(1.0, 1.0, 1.0, 1.0)
#define IDLE_COLOR    vec4(1.0, 0.0, 0.0, 1.0)

out vec4 color;

void main() {
    bool playing = tick > tick_start_out && tick <= tick_end_out;

    color = playing ? PLAYING_COLOR : IDLE_COLOR;
}
