#version 460 core

uniform float tick;

#define PLAYING_COLOR vec4(1.0, 1.0, 1.0, 1.0)
#define IDLE_COLOR    vec4(1.0, 0.0, 0.0, 1.0)

out vec4 color;

void main() {
    /* hard coded quaver */
#   define TICK_START 0.00
#   define TICK_END   20.0

    bool playing = tick > TICK_START && tick < TICK_END;

    color = playing ? PLAYING_COLOR : IDLE_COLOR;
}
