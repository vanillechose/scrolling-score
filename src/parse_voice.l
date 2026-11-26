%{
#   include <ctype.h>

#   include "common.h"

    unsigned line_number;

    static void push_note(const char *);

%}

%option noyywrap
%option nounput
%option noinput

%%

[ \t]+    /* skip */
\n        { line_number += 1; }

[a-hr]("es"|"is"|"s")?([1-9][0-9]*)?[.,']* { push_note(yytext); }

.         { die("unexpected note: %s", yytext); }

%%

static unsigned duration;
static unsigned octave;
static unsigned dots;

enum {
    NONE,
    FLAT,
    SHARP
};

static float encode(char letter, int alt) {
    float note;

    switch(letter) {
        case 'c': note = 0.0f; break;
        case 'd': note = 2.0f; break;
        case 'e': note = 4.0f; break;
        case 'f': note = 5.0f; break;
        case 'g': note = 7.0f; break;
        case 'a': note = 9.0f; break;
        case 'b': note = 10.0; break;
        case 'h': note = 11.0; break;

        default:
            die("line %lu: invalid note '%c'", line_number, letter);
    }

    switch(alt) {
        case FLAT:
            note -= 1.0;
            break;
        case SHARP:
            note += 1.0;
            break;
        default:
            break;
    }

    note = note + 24.0f + octave * 12.0f;

    return note;
}

static size_t       cap;
static size_t       len;
static struct note *notes;

static double tick_per_bar;
static double tick;

static void push_note(const char *tok) {
    char letter = *tok++;
    int  alt    = NONE;

         if(tok[0] == 'e' && tok[1] == 's') { alt = FLAT;  tok += 2; }
    else if(tok[0] == 's')                  { alt = FLAT;  tok += 1; }
    else if(tok[0] == 'i' && tok[1] == 's') { alt = SHARP; tok += 2; }

    if(isdigit(*tok)) {
        dots = 0;
        char *end;
        duration = strtoul(tok, &end, 10);
        tok = end;
    }

    for(; *tok; tok += 1) {
        if(*tok == '\'') octave += 1;
        if(*tok == ',')  octave -= 1;
        if(*tok == '.')  dots += 1;
    }

    if(cap == len) {
        cap *= 2;

        struct note *newbuf = xmalloc(cap * sizeof notes[0]);

        memcpy(newbuf, notes, len * sizeof notes[0]);

        free(notes);

        notes = newbuf;
    }

    double tick_end = tick + tick_per_bar / (double)duration;

    for(unsigned i = 0, dotted_duration = duration; i < dots; i += 1) {
        dotted_duration *= 2;
        tick_end += tick_per_bar / (double)dotted_duration;
    }

    /* printf("push_note %c alt=%d dur=%u oct=%u tpb=%f tick=%f end=%f\n",
            letter, alt, duration, octave, tick_per_bar, tick, tick_end); */

    if(letter != 'r') {
        notes[len].note  = encode(letter, alt);
        notes[len].start = (float)tick;
        notes[len].end   = (float)tick_end;

        len += 1;
    }

    tick = tick_end;
}

extern size_t parse_voice(struct note **dst, const char *text, double tpb) {
    extern int yylex();

    line_number = 0;
    duration = 4;
    octave   = 4;
    dots     = 0;

    cap   = 64;
    len   = 0;
    notes = xmalloc(cap * sizeof notes[0]);
    memset(notes, 0, 64 * sizeof notes[0]);

    tick_per_bar = tpb;
    tick         = 0.0;

    YY_BUFFER_STATE buf = yy_scan_string(text);

    yylex();

    yy_delete_buffer(buf);

    *dst = notes;

    return len;
}
