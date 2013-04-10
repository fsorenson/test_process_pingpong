#CC=gcc
CC=g++
LIBS=-lpthread -pthread

CFLAGS= -Wall -fms-extensions -g

all: test_process_pingpong

SRCS=test_process_pingpong.cpp
OBJS=$(SRCS:.[ch]pp=.o)

test_process_pingpong:	$(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LIBS)

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $<
.cpp.o:
	$(CC) -s$(CFLAGS) $(INCLUDES $<

clean:
	@rm test_process_pingpong test_process_pingpong.o
