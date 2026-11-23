CC	::=	gcc
CFLAGS	::=	-Wall -Wextra -std=gnu23 -fanalyzer

LIBS	::=	glfw3 glew

CFLAGS	+=	$(shell pkg-config --cflags $(LIBS))
LDFLAGS	+=	$(shell pkg-config --libs $(LIBS))

srcs	::=	$(wildcard src/*.c)
objs	::=	$(srcs:.c=.o)

target	::=	scrolling-score.exe

.PHONY: all
all: $(target)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(target): $(objs)
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f $(objs) $(target)
