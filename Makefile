CC	::=	gcc
CFLAGS	::=	-Wall -Wextra -std=gnu23 -fanalyzer -ggdb

LIBS	::=	glfw3 glew

CFLAGS	+=	$(shell pkg-config --cflags $(LIBS))
LDFLAGS	+=	$(shell pkg-config --libs $(LIBS))

srcs	::=	$(wildcard src/*.c) src/parse_voice.c
objs	::=	$(srcs:.c=.o)

target	::=	scrolling-score.exe

.PHONY: all
all: $(target)

src/parse_voice.c: src/parse_voice.l
	flex -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(target): $(objs)
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f $(objs) $(target) src/parse_voice.c
