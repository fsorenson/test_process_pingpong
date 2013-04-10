CC=gcc
#CC=g++
LIBS=-lpthread -lm -lrt

objs_dir=objs
deps_dir=deps
stab_dir=stabs

LDFLAGS=
CPPFLAGS=
CFLAGS=

CPPFLAGS += -std=gnu99
#CPPFLAGS += -std=c99

#CFLAGS += -D_XOPEN_SOURCE=600 -D_GNU_SOURCE

# tell gcc my cpus don't do avx?...
#CFLAGS += ?????

#CFLAGS += -march=native -mtune=native
#CFLAGS += -march=corei7 -mtune=corei7
#CFLAGS += -march=corei7 -mtune=corei7



# some warnings
CFLAGS += -Wall -Wextra
CFLAGS += -Wunused-macros -Wunused -Wunused-parameter -Wunreachable-code
#CFLAGS += -pedantic
CFLAGS += -Wshadow -Wundef -Wendif-labels -Wconversion -Wmissing-format-attribute -Wpointer-arith -Wfloat-equal -Wpacked -Wpadded -Winline
CFLAGS += -Wmissing-noreturn -Wmissing-format-attribute -Wmissing-field-initializers
CFLAGS += -Wdeclaration-after-statement -Wbad-function-cast
CFLAGS += -Wmissing-declarations -Wredundant-decls -Wnested-externs -Wunreachable-code

# doesn't seem to work on older gcc
#-Wlogical-op -Waddress


# too annoying to enable
#CFLAGS += -Wstrict-prototypes -Wold-style-definition



# optimizations
# -O -OO -O1 -O2 -O3 -Os -Ofast
# older gcc doesn't understand '-Ofast'
#CFLAGS += -Ofast
CFLAGS += -O3
CFLAGS += -fshort-enums
CFLAGS += -malign-double
CFLAGS += -fif-conversion -fif-conversion2
#CFLAGS += -finline-functions -finline-functions-called-once -finline-small-functions
CFLAGS += -fargument-alias
CFLAGS += -Wbad-function-cast


#CFLAGS += -flto-report

# remove symbols for ultra-small binaries--or use strip
#LDFLAGS += -Wl,-S -Wl,-s


# enable this to get debugging
DEBUG_FLAGS=
#DEBUG_FLAGS += -g
#DEBUG_FLAGS += -ggdb


PROFILING_FLAGS=
# enable this to get profiling with 'prof'
#PROFILING_FLAGS += -p

# enable this to get profiling with 'gprof'
#PROFILING_FLAGS += -pg

# interesting one... will it work?
#PROFILING_FLAGS += -fbranch-probabilities

# to generate gcov profiling files:
#PROFILING_FLAGS += -ftest-coverage -fprofile-arcs -g --coverage

# use generate or use
#PROFILING_FLAGS += -fprofile-generate
#PROFILING_FLAGS += -fprofile-use


# omit-frame-pointer is incompatible with debugging & profiing
# flags, so only allow it if neither is in use
ifeq ($(strip $(PROFILING_FLAGS)),)
  ifeq ($(strip $(DEBUG_FLAGS)),)
    CFLAGS += -fomit-frame-pointer -fstrict-aliasing 
  endif
endif

CFLAGS += $(DEBUG_FLAGS) $(PROFILING_FLAGS)


CFLAGS += -DHAVE_SCHED_GETCPU

all: test_process_pingpong

f = test_process_pingpong
f += common
f += units
f += sched
f += threads signals
f += setup
#f += tcp udp pipe socket_pair sem eventfd futex spin nop mq

#SRCS = test_process_pingpong.c
C_SRCS = $(addsuffix .c,$(f))
H_SRCS = $(addsuffix .h,$(f))
SRCS = $(C_SRCS) $(H_SRCS)

OBJS = $(addprefix $(objs_dir)/, $(addsuffix .o,$(f)))
DEPS = $(addprefix $(deps_dir)/, $(addsuffix .d,$(f)))
stabs = $(addprefix $(stab_dir)/,$(addsuffix .s,$(f)))



comms = tcp udp pipe socket_pair sem futex mq eventfd spin nop yield unconstrained

comms_c_srcs = $(addsuffix .c,$(comms))
comms_h_srcs = $(addsuffix .h,$(comms))
comms_srcs = $(comms_c_srcs) $(comms_h_srcs)
comms_objs = $(addprefix $(objs_dir)/, $(addsuffix .o,$(comms)))
comms_deps = $(addprefix $(deps_dir)/, $(addsuffix .d,$(comms)))
comms_stabs += $(addprefix $(stab_dir)/,$(addsuffix .s,$(comms)))


SRCS += $(comms_srcs)
#OBJS += $(comms_objs)
DEPS += $(comms_deps)
stabs += $(comms_stabs)


comms_glue = comms

comms_glue_c_srcs = $(addsuffix .c,$(comms_glue))
comms_glue_h_srcs = $(addsuffix .h,$(comms_glue))
comms_glue_srcs = $(comms_glue_c_srcs) $(comms_glue_h_srcs)
comms_glue_objs = $(addprefix $(objs_dir)/, $(addsuffix .o,$(comms_glue)))
comms_glue_deps = $(addprefix $(deps_dir)/, $(addsuffix .d,$(comms_glue)))
comms_glue_stabs += $(addprefix $(stab_dir)/,$(addsuffix .s,$(comms_glue)))

SRCS += $(comms_glue_srcs)
#OBJS += $(comms_glue_objs)
DEPS += $(comms_glue_deps)
stabs += $(comms_glue_stabs)



# build the deps files
$(deps_dir)/%.d: %.c %.h
	@$(SHELL) -ec '$(CC) -MM $(CPPFLAGS) $< | sed s@$*.o@objs/\&\ deps/$@@g > $@'

# include the deps files
-include $(DEPS)

$(objs_dir)/%.o: %.c $(deps_dir)/%.d
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


test_process_pingpong:	$(comms_glue_objs) $(comms_objs) $(OBJS)
	$(CC) $(comms_glue_objs) $(comms_objs) $(OBJS) $(CPPFLAGS) $(CFLAGS) $(LIBS) -o $@


$(stab_dir)/%.s: %.c $(deps_dir)/%.d
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -gstabs -S -o $@

stabs: $(stabs)


.PHONY: cov clean
.PRECIOUS: $(deps_dir)/%.d

gprof: $(OBJS)
	@$(CC) -o $@ $(OBJS) $(CFLAGS) $(CPPFLAGS) $(LIBS) -pg -O 

cov:
	lcov --directory=`pwd` --capture --output-file gcov/app.info
	genhtml --output-directory gcov gcov/app.info

clean:
	#echo "objs=$(OBJS)"
	#echo "deps=$(DEPS)"
	#echo "stabs=$(STABS)"
	#echo "comms_obs=$(comms_objs), comms_glue_objs=$(comms_glue_objs)"
	@rm -f test_process_pingpong $(OBJS) $(DEPS) $(STABS) $(comms_objs) $(comms_glue_objs)



