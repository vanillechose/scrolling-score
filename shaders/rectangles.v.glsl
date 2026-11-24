#version 460 core

layout (location = 0) in vec2 corner;

in float note;
in float tick_start;
in float tick_end;

uniform float t;

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

flat out float tick_start_out;
flat out float tick_end_out;

void main() {
    float lo = (tick_start - t) * TICK_WIDTH;
    float hi = (tick_end   - t) * TICK_WIDTH;
    float xp = lo * (1.0 - corner.x) + hi * corner.x;

    float y = corner.y * NOTE_THICKNESS;

    float offset_y = 2.0 * (note - LOW_NOTE) / NOTE_COUNT;

    gl_Position = vec4(xp, y + offset_y - 1.0, 0.0, 1.0);

    tick_start_out = tick_start;
    tick_end_out   = tick_end;
}
