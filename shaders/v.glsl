#version 460 core

layout (location = 0) in vec2 corner;

uniform float tick;

/* 160 ticks = semibreve (arbitrary) */
#define SEMIBREVE_TICKS 160.0
/* semibreve = 0.30 units (value from smalin's 2013 BWV 542 video) */
#define SEMIBREVE_WIDTH 0.60

#define TICK_WIDTH (SEMIBREVE_WIDTH / SEMIBREVE_TICKS)

void main() {
    /* hard coded quaver */
#   define TICK_START 0.00
#   define TICK_END   20.0

    float lo = (TICK_START - tick) * TICK_WIDTH;
    float hi = (TICK_END   - tick) * TICK_WIDTH;

    float xp = lo * (1.0 - corner.x) + hi * corner.x;
    float yp = corner.y * 0.5;

    gl_Position = vec4(xp, yp, 0.0, 1.0);
}
