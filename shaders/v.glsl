#version 460 core

layout (location = 0) in vec2 corner;
layout (location = 1) in float note;
layout (location = 2) in float tick_start;
layout (location = 3) in float tick_end;

uniform float tick;

/* 160 ticks = semibreve (arbitrary) */
#define SEMIBREVE_TICKS 160.0
/* semibreve = 0.30 units (value from smalin's 2013 BWV 542 video) */
#define SEMIBREVE_WIDTH 0.60

#define TICK_WIDTH (SEMIBREVE_WIDTH / SEMIBREVE_TICKS)

/* arbitrary values, A1 and Fis6 */
#define LOW_NOTE  45.0
#define HIGH_NOTE 102.0

#define NOTE_COUNT     (HIGH_NOTE - LOW_NOTE)
#define NOTE_THICKNESS 0.08

out float tick_start_out;
out float tick_end_out;

void main() {
    float lo = (tick_start - tick) * TICK_WIDTH;
    float hi = (tick_end   - tick) * TICK_WIDTH;
    float xp = lo * (1.0 - corner.x) + hi * corner.x;

    float y = corner.y * 0.10;

    float offset_y = 2.0 * (note - LOW_NOTE) / NOTE_COUNT;

    gl_Position = vec4(xp, y + offset_y - 1.0, 0.0, 1.0);

    tick_start_out = tick_start;
    tick_end_out   = tick_end;
}
