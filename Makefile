CC=gcc
#CC=g++
LIBS=-lpthread -lm -lrt

#CFLAGS = -march=native -O3 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE -std=c99 -W -Wall -Werror

CFLAGS = -Wall -Wextra
#CFLAGS += -fms-extensions
#CFLAGS += -std=c99
CFLAGS += -std=gnu99

# tell gcc my cpus don't do avx?...
#CFLAGS += ?????

#CFLAGS += -march=native -mtune=native
CFLAGS += -march=corei7
#CFLAGS += -march=nocona



# enable this to get debugging
CFLAGS += -g

CFLAGS += -ggdb

# enable this to get profiling with 'prof'
#CFLAGS += -p

# enable this to get profiling with 'gprof'
#CFLAGS += -pg

CFLAGS += -DHAVE_EVENTFD
CFLAGS += -DHAVE_SCHED_GETCPU
CFLAGS += -DSET_PRIORITIES

all: test_process_pingpong

f = test_process_pingpong
f += units
f += sched
f += setup
f += tcp udp socket_pair sem eventfd futex spin nop mq

#SRCS = test_process_pingpong.c
C_SRCS = $(addsuffix .c,$(f))
H_SRCS = $(addsuffix .h,$(f))
SRCS = $(C_SRCS) $(H_SRCS)

OBJS = $(addsuffix .o,$(f))


test_process_pingpong:	$(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LIBS)

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $<
.cpp.o:
	$(CC) -s$(CFLAGS) $(INCLUDES) $<

clean:
	@rm test_process_pingpong $(OBJS)
