#CC=color-gcc
CC=gcc
#CC=gcc
#CC=g++
LIBS=-lpthread -lm -lrt

comms_dir = comms
objs_dir = objs
deps_dir = deps
stab_dir = stabs

LDFLAGS=
CPPFLAGS= -iquote .
CFLAGS=

CPPFLAGS += -std=gnu99
#CPPFLAGS += -std=c99

#CFLAGS += -D_XOPEN_SOURCE=600 -D_GNU_SOURCE

# tell gcc my cpus don't do avx?...
#CFLAGS += ?????

#CFLAGS += -march=native -mtune=native
#CFLAGS += -march=corei7 -mtune=corei7
#CFLAGS += -march=corei7 -mtune=corei7


# begin with empty lists
C_SRCS =
H_SRCS =
SRCS =

objs =
deps =
stabs =
gcovs =


# some warnings
CFLAGS += -Wall -Wextra
#CFLAGS += -pedantic
CFLAGS += -Wunused-macros -Wunused -Wunused-parameter -Wunreachable-code
CFLAGS += -Wshadow -Wundef -Wconversion -Wbad-function-cast
CFLAGS += -Wpointer-arith -Wfloat-equal
CFLAGS += -Wpacked -Wpadded -Winline -Wnested-externs
CFLAGS += -Wendif-labels
CFLAGS += -Wmissing-noreturn
CFLAGS += -Wmissing-format-attribute -Wmissing-field-initializers
CFLAGS += -Wmissing-declarations -Wredundant-decls -Wdeclaration-after-statement
CFLAGS += -Wbad-function-cast

# doesn't seem to work on older gcc
#-Wlogical-op -Waddress


# too annoying to enable
#CFLAGS += -Wstrict-prototypes -Wold-style-definition



# optimizations
# -O -OO -O1 -O2 -O3 -Os -Ofast
# older gcc doesn't understand '-Ofast'
OPTIMIZATIONS =
#OPTIMIZATIONS += -Ofast
OPTIMIZATIONS += -O3
OPTIMIZATIONS += -fshort-enums
OPTIMIZATIONS += -malign-double
OPTIMIZATIONS += -fif-conversion -fif-conversion2
#OPTIMIZATIONS += -finline-functions -finline-functions-called-once -finline-small-functions
OPTIMIZATIONS += -fargument-alias
#OPTIMIZATIONS += 
#OPTIMIZATIONS += 


#OPTIMIZATIONS += -fwhole-program

#CFLAGS += -fweb -flto

# remove symbols for ultra-small binaries--or use strip
#LDFLAGS += -Wl,-S -Wl,-s



# enable this to get debugging
DEBUG_FLAGS=
DEBUG_FLAGS += -g
#DEBUG_FLAGS += -ggdb -gdwarf-3


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


# only optimize if debuging is not enabled
ifeq ($(strip $(DEBUG_FLAGS)),)
  CFLAGS += $(OPTIMIZATIONS)
endif
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
f += threads_monitor threads_children
f += signals
f += setup
#f += tcp udp pipe socket_pair sem eventfd futex spin nop mq

#SRCS = test_process_pingpong.c
C_SRCS += $(addsuffix .c,$(f))
H_SRCS += $(addsuffix .h,$(f))
SRCS += $(C_SRCS) $(H_SRCS)

objs += $(addprefix $(objs_dir)/, $(addsuffix .o,$(f)))
deps += $(addprefix $(deps_dir)/, $(addsuffix .d,$(f)))
stabs += $(addprefix $(stab_dir)/, $(addsuffix .s,$(f)))
gcovs += $(addprefix $(objs_dir)/, $(addsuffix .gcda,$(f)))
gcovs += $(addprefix $(objs_dir)/, $(addsuffix .gcno,$(f)))


comms = tcp udp pipe socket_pair sem futex mq eventfd spin nop yield unconstrained

comms_c_srcs = $(addprefix $(comms_dir)/, $(addsuffix .c,$(comms)))
comms_h_srcs = $(addprefix $(comms_dir)/, $(addsuffix .h,$(comms)))
comms_srcs = $(comms_c_srcs) $(comms_h_srcs)
comms_objs = $(addprefix $(objs_dir)/, $(addsuffix .o,$(comms)))
comms_deps = $(addprefix $(deps_dir)/, $(addsuffix .d,$(comms)))
comms_stabs += $(addprefix $(stab_dir)/, $(addsuffix .s,$(comms)))


SRCS += $(comms_srcs)
#objs += $(comms_objs)
deps += $(comms_deps)
stabs += $(comms_stabs)
gcovs += $(addprefix $(objs_dir)/, $(addsuffix .gcda,$(comms)))
gcovs += $(addprefix $(objs_dir)/, $(addsuffix .gcno,$(comms)))


comms_glue = comms

comms_glue_c_srcs = $(addsuffix .c,$(comms_glue))
comms_glue_h_srcs = $(addsuffix .h,$(comms_glue))
comms_glue_srcs = $(comms_glue_c_srcs) $(comms_glue_h_srcs)
comms_glue_objs = $(addprefix $(objs_dir)/, $(addsuffix .o,$(comms_glue)))
comms_glue_deps = $(addprefix $(deps_dir)/, $(addsuffix .d,$(comms_glue)))
comms_glue_stabs += $(addprefix $(stab_dir)/,$(addsuffix .s,$(comms_glue)))

SRCS += $(comms_glue_srcs)
#objs += $(comms_glue_objs)
deps += $(comms_glue_deps)
stabs += $(comms_glue_stabs)
gcovs += $(addprefix $(objs_dir)/, $(addsuffix .gcda,$(comms_glue)))
gcovs += $(addprefix $(objs_dir)/, $(addsuffix .gcno,$(comms_glue)))




# build the deps files
$(deps_dir)/%.d: %.c %.h
	@$(SHELL) -ec '$(CC) -MM $(CPPFLAGS) $< | sed s@$*.o@objs/\&\ $@@ > $@'

$(deps_dir)/%.d: $(comms_dir)/%.c $(comms_dir)/%.h
	@$(SHELL) -ec '$(CC) -MM $(CPPFLAGS) $< | sed s@$*.o@objs/\&\ $@@ > $@'



# include the deps files
-include $(deps)



$(objs_dir)/%.o: %.c  $(deps_dir)/%.d
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(objs_dir)/%.o: $(comms_dir)/%.c $(deps_dir)/%.d
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@




test_process_pingpong:	$(comms_glue_objs) $(comms_objs) $(objs) $(deps)
	$(CC) $(comms_glue_objs) $(comms_objs) $(objs) $(CPPFLAGS) $(CFLAGS) $(LIBS) -o $@


$(stab_dir)/%.s: $(comms_dir)/%.c $(deps_dir)/%.d
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -g -Wa,-ahl=$@ -o /tmp/gcc_out.log


$(stab_dir)/%.s: %.c $(deps_dir)/%.d
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -g -Wa,-ahl=$@ -o /tmp/gcc_out.log
#	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -g -Wa,-ahl=$@ -o /dev/null
#	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -gstabs -g -S -Wa,-ahl=$@
#	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -gstabs -g -S -asm -Wa,-ahl=$@

stabs: $(stabs)


.PHONY: cov clean
.PRECIOUS: $(deps_dir)/%.d

gprof: $(objs)
	@$(CC) -o $@ $(objs) $(CFLAGS) $(CPPFLAGS) $(LIBS) -pg -O 

cov:
	lcov --directory=`pwd` --capture --ignore-errors source --base-directory `pwd` \
		--output-file gcov/app.info
	genhtml --output-directory gcov gcov/app.info
	genpng --output-filename gcov/calls.png gcov/app.info
#	rm -rf gcov && mkdir gcov
#	lcov --directory=`pwd` --add-tracefile gcov/comms.info --output-file gcov/app.info
#	lcov --directory=`pwd`/comms --capture --ignore-errors source --base-directory `pwd` \
#		--no-recursion --output-file gcov/comms.info
#	lcov --directory=`pwd` --capture --ignore-errors source --base-directory `pwd` \
#		--no-recursion --output-file gcov/base.info
#	lcov --directory=`pwd` --add-tracefile gcov/comms.info --add-tracefile gcov/base.info --output-file gcov/app.info

clean:
	#echo "objs=$(objs)"
	#echo "deps=$(deps)"
	#echo "stabs=$(stabs)"
	#echo "comms_obs=$(comms_objs), comms_glue_objs=$(comms_glue_objs)"
	#echo "gcovs=$(gcovs)"
	@rm -f test_process_pingpong $(objs) $(deps) $(stabs) $(comms_objs) $(comms_glue_objs) $(gcovs)



