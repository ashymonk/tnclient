TARGET := tnclient
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

CC := gcc
CFLAGS +=
LDFLAGS +=

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

.PHONY: all clean run
all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJS)

run: $(TARGET)
	./$(TARGET) $(ARGS)

default: all
