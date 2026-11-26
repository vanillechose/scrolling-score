#version 460 core

in vec2 corner;
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
#define NOTE_THICKNESS (4.0 / NOTE_COUNT)

flat out float tick_start_out;
flat out float tick_end_out;

flat out vec2  radius;
     out vec2  pos;

void main() {
    float x0 = (tick_start - t) * TICK_WIDTH;
    float x1 = (tick_end   - t) * TICK_WIDTH;

    float y0 = 2.0 * (note - LOW_NOTE) / NOTE_COUNT - 1.0;
    float y1 = y0 + NOTE_THICKNESS;

    vec2 center = vec2(x0 + x1, y0 + y1) * 0.5;

    radius  = vec2(x1 - x0, y1 - y0) * 0.5;
    pos     = (2.0 * corner - 1.0) * radius;

    gl_Position = vec4(center + pos, 0.0, 1.0);

    tick_start_out = tick_start;
    tick_end_out   = tick_end;
}
