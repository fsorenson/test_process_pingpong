CC=gcc
#CC=g++
LIBS=-lpthread -lm -lrt
OBJDIR=objs
DEPDIR=deps

CPPFLAGS = -std=gnu99

#CFLAGS = -march=native -O3 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE -std=c99 -W -Wall -Werror


CFLAGS = 


# some warnings
CFLAGS += -Wall -Wextra
CFLAGS += -Wshadow -Wundef -Wendif-labels -Wconversion -Wmissing-format-attribute -Wpointer-arith -Wfloat-equal -Wpacked -Wpadded -Wunreachable-code
#CFLAGS += -Wpointer-arith -Wunreachable-code -Winline

#CFLAGS += -fms-extensions
#CFLAGS += -std=c99

# tell gcc my cpus don't do avx?...
#CFLAGS += ?????

#CFLAGS += -march=native -mtune=native
CFLAGS += -march=corei7 -mtune=corei7
#CFLAGS += -march=nocona




# optimizations
# -O -OO -O1 -O2 -O3 -Os -Ofast
CFLAGS += -Ofast -fshort-enums
CFLAGS += -malign-double
CFLAGS += -fif-conversion -fif-conversion2
#CFLAGS += -finline-functions -finline-functions-called-once -finline-small-functions
CFLAGS += -fargument-alias

# won't work if any debugging is enabled
#CFLAGS += -fomit-frame-pointer

# nice...
CFLAGS += -flto-report

# enable this to get debugging
#CFLAGS += -g
#CFLAGS += -ggdb

# enable this to get profiling with 'prof'
#CFLAGS += -p

# enable this to get profiling with 'gprof'
#CFLAGS += -pg

# interesting one... will it work?
#CFLAGS += -fbranch-probabilities

# to generate gcov profiling files:
#CFLAGS += -ftest-coverage -fprofile-arcs -g --coverage


# use generate or use
#CFLAGS += -fprofile-generate
#CFLAGS += -fprofile-use


#CFLAGS += -DHAVE_SCHED_GETCPU

all: test_process_pingpong

f = test_process_pingpong
f += units
f += sched
f += threads signals
f += setup
f += tcp udp socket_pair sem eventfd futex spin nop mq

#SRCS = test_process_pingpong.c
C_SRCS = $(addsuffix .c,$(f))
H_SRCS = $(addsuffix .h,$(f))
SRCS = $(C_SRCS) $(H_SRCS)

OBJS = $(addprefix $(OBJDIR)/, $(addsuffix .o,$(f)))
DEPS = $(addprefix $(DEPDIR)/, $(addsuffix .d,$(f)))
STABS = $(addsuffix .s,$(f))

# build the deps files
$(DEPDIR)/%.d: %.c %.h
	@$(SHELL) -ec '$(CC) -MM $(CPPFLAGS) $< | sed s@$*.o@objs/\&\ deps/$@@g > $@'

# include the deps files
-include $(DEPS)

$(OBJDIR)/%.o: %.c $(DEPDIR)/%.d
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


test_process_pingpong:	$(OBJS)
	@$(CC) -o $@ $(OBJS) $(CFLAGS) $(CPPFLAGS) $(LIBS)

%.s: %.c $(DEPDIR)/%.d
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -gstabs -S

stabs: $(STABS)


.PHONY: cov clean
.PRECIOUS: $(DEPDIR)/%.d

cov:
	lcov --directory=`pwd` --capture --output-file gcov/app.info
	genhtml --output-directory gcov gcov/app.info

clean:
	@rm test_process_pingpong $(OBJS) $(DEPS) $(STABS)
