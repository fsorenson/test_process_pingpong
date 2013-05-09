#CC=color-gcc
CC=gcc
#CC=gcc
#CC=g++
LIBS=-lpthread -lm -lpthread

target = test_process_pingpong

# for glibc
LIBS += -lrt

# for dietlibc:
#CC := diet $(CC)
#LIBS += -lcompat


base_dir = $(CURDIR)
base_dir_name = $(notdir $(base_dir))
parent_dir = $(shell dirname $(base_dir))
#$(info base_dir = $(base_dir))
#$(info base_dir_name = $(base_dir_name))
#$(info parent_dir = $(parent_dir))

comms_dir = comms
objs_dir = objs
deps_dir = deps
stab_dir = stabs

LDFLAGS=
CPPFLAGS= -iquote .
CFLAGS=

CPPFLAGS += -std=gnu11
#CPPFLAGS += -std=c99

# only enable if 'static' is really what we want
#CPPFLAGS += -static

#CFLAGS += -D_XOPEN_SOURCE=600 -D_GNU_SOURCE

# tell gcc my cpus don't do avx?...
#CFLAGS += ?????

#CFLAGS += -march=native -mtune=native
#CFLAGS += -march=corei7 -mtune=corei7
#CFLAGS += -march=corei7 -mtune=corei7

# check for eventfd.h
HAVE_EVENTFD = $(shell [[ -f /usr/include/sys/eventfd.h ]] && echo "-DHAVE_EVENTFD_H")


COMPILE_DEBUG =

# normal verbose
#COMPILE_DEBUG += -v

# only enable these if you want to see _exceptionally_verbose_ debugging messages during compilation
# cool idea, though
#COMPILE_DEBUG += -Q

# statistics about compilation
#COMPILE_DEBUG += -ftime-report
#COMPILE_DEBUG += -fmem-report
#COMPILE_DEBUG += -time

# > 0 - output same info as -dRS
# > 1 - output basic block probability, detailed ready list info, and unit/insn info
# > 2 - RTL at abort point, control-flow and regions info
# > 4 - includes dependence info
# > 9 - output always goes to stderr
#COMPILE_DEBUG += -fsched-verbose=


CFLAGS += $(COMPILE_DEBUG)


# begin with empty lists
C_SRCS =
H_SRCS =
SRCS =

objs =
deps =
stabs =
gcovs =

sub_dirs =
sub_dirs += $(objs) $(deps) $(stabs)

# some warnings
WARNINGS =

WARNINGS += -Wall -Wextra
WARNINGS += -Wmissing-include-dirs
WARNINGS += -Wcomment -Wmultichar

# enable whenever we want to do massive cleanup, otherwise too chatty
# WARNINGS += -pedantic -Waggregate-return

# these reviewed, and do not appear to be covered by above flags
WARNINGS += -Wswitch-default -Wswitch-enum -Winit-self
WARNINGS += -Wpacked -Wpadded -Wwrite-strings -Wlogical-op
WARNINGS += -Wstrict-overflow -Wconversion
WARNINGS += -Wmissing-declarations -Wredundant-decls -Wdeclaration-after-statement
WARNINGS += -Wmissing-prototypes -Wstrict-prototypes -Wunused-parameter
WARNINGS += -Wold-style-definition
WARNINGS += -Wbad-function-cast -Wcast-qual -Wcast-align
WARNINGS += -Wundef -Wendif-labels -Wshadow

WARNINGS += -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn
WARNINGS += -Winline -Wnested-externs
WARNINGS += -Wtrampolines -Wstack-protector
WARNINGS += -Wdisabled-optimization -Wunsafe-loop-optimizations

WARNINGS += -Wunsuffixed-float-constants -Wdouble-promotion -Wfloat-equal

#additional format options NOT in Wall:
WARNINGS += -Wformat-y2k -Wno-format-extra-args -Wno-format-zero-length -Wformat-nonliteral -Wformat-security -Wformat=2

# Cant find these in the documentation
WARNINGS += -Wunused-macros -Wunreachable-code -Wmissing-noreturn

#may be too annoying
#WARNINGS += -Wtraditional -Wtraditional-conversion
WARNINGS += -Wmissing-format-attribute
#WARNINGS += -Wlong-long -Wvla
WARNINGS += --all-warnings
#WARNINGS += -Wc++-compat 



# optimizations
# -O -OO -O1 -O2 -O3 -Os -Ofast
# older gcc doesn't understand '-Ofast'
OPTIMIZATIONS =
OPTIMIZATIONS += -Ofast
#OPTIMIZATIONS += -O3
OPTIMIZATIONS += -fshort-enums
OPTIMIZATIONS += -malign-double
OPTIMIZATIONS += -fif-conversion -fif-conversion2
OPTIMIZATIONS += -finline-functions -finline-functions-called-once -finline-small-functions
OPTIMIZATIONS += -fargument-alias

# testing
OPTIMIZATIONS += -falign-functions -falign-loops -falign-jumps -falign-labels
OPTIMIZATIONS += -fdevirtualize -fexpensive-optimizations
OPTIMIZATIONS += -fstrict-overflow
OPTIMIZATIONS += -fvariable-expansion-in-unroller -funswitch-loops
#OPTIMIZATIONS += 


#OPTIMIZATIONS += -fwhole-program

#CFLAGS += -fweb -flto


# LD flags
# still experimenting to see which are/aren't useful/interesting
LDFLAGS += -Wl,--warn-common -Wl,--warn-constructors
LDFLAGS += -Wl,--warn-multiple-gp
#LDFLAGS += -Wl,--warn-section-align -Wl,--warn-shared-textrel -Wl,--warn-unresolved-symbols
#LDFLAGS += -Wl,--verbose=1

# remove symbols for ultra-small binaries--or use strip
#LDFLAGS += -Wl,-S -Wl,-s




# enable this to get debugging
DEBUG_FLAGS=
#DEBUG_FLAGS += -g -Og
#DEBUG_FLAGS += -ggdb -gdwarf-3


#DEBUG_FLAGS += -fvar-tracking -fvar-tracking-assignments -fvar-tracking-uninit

# enable this to get 'backtrace()' symbols
#DEBUG_FLAGS += -rdynamic

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

all: $(target)

f = $(target)
f += common
f += units
f += sched
f += threads_monitor threads_children
f += signals
f += setup
#f += tcp udp pipe socket_pair sem eventfd futex spin nop mq

#SRCS = $(target).c
C_SRCS += $(addsuffix .c,$(f))
H_SRCS += $(addsuffix .h,$(f))
SRCS += $(C_SRCS) $(H_SRCS)

objs += $(addprefix $(objs_dir)/, $(addsuffix .o,$(f)))
deps += $(addprefix $(deps_dir)/, $(addsuffix .d,$(f)))
stabs += $(addprefix $(stab_dir)/, $(addsuffix .s,$(f)))
gcovs += $(addprefix $(objs_dir)/, $(addsuffix .gcda,$(f)))
gcovs += $(addprefix $(objs_dir)/, $(addsuffix .gcno,$(f)))


comms =
comms += tcp udp pipe socket_pair
comms += sem
comms += futex futex_test mq inotify signal
ifneq ($(HAVE_EVENTFD),)
comms += eventfd
endif
comms += spin yield race nop
comms += namedpipe
#comms += benaphore

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
	@mkdir -p $(@D)
	@$(SHELL) -ec '$(CC) -MM $(CPPFLAGS) $< | sed s@$*.o@objs/\&\ $@@ > $@'

$(deps_dir)/%.d: $(comms_dir)/%.c $(comms_dir)/%.h
	@mkdir -p $(@D)
	@$(SHELL) -ec '$(CC) -MM $(CPPFLAGS) $< | sed s@$*.o@objs/\&\ $@@ > $@'



# include the deps files
-include $(deps)



$(objs_dir)/%.o: %.c  $(deps_dir)/%.d
	@mkdir -p $(@D)
	@mkdir -p stabs
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -c $< -o $@ \
		-Wa,-adhlmns=$(stab_dir)/$(addsuffix .s,$(basename $(notdir $@))) -g -fverbose-asm -masm=intel

$(objs_dir)/%.o: $(comms_dir)/%.c $(deps_dir)/%.d
	@mkdir -p $(@D)
	@mkdir -p stabs
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -c $< -o $@ \
		-Wa,-adhlmns=$(stab_dir)/$(addsuffix .s,$(basename $(notdir $@))) -g -fverbose-asm -masm=intel



$(target):	$(comms_glue_objs) $(comms_objs) $(objs) $(deps)
	$(CC) $(comms_glue_objs) $(comms_objs) $(objs) $(CPPFLAGS) $(CFLAGS) $(LIBS) $(WARNINGS) $(LDFLAGS) -o $@


$(stab_dir)/%.s: $(comms_dir)/%.c $(deps_dir)/%.d
	@mkdir -p $(@D)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -g -Wa,-ahl=$@ -o /tmp/gcc_out.log


$(stab_dir)/%.s: %.c $(deps_dir)/%.d
	@mkdir -p $(@D)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -g -Wa,-ahl=$@ -o /tmp/gcc_out.log
#	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -g -Wa,-ahl=$@ -o /dev/null
#	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -gstabs -g -S -Wa,-ahl=$@
#	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -gstabs -g -S -asm -Wa,-ahl=$@

stabs: $(stabs)


src_release: $(SRCS)
	$(eval rel_srcs = $(addprefix $(base_dir_name)/, $(SRCS)))
	$(eval tarball = $(parent_dir)/$(base_dir_name).tar.bz2)
	$(info tarball = $(tarball))
	@( \
		cd .. ; \
		tar -C $(parent_dir) -cjf $(tarball) $(rel_srcs) \
			$(base_dir_name)/Makefile\
	)
	$(info wrote $(tarball))

full_release: $(SRCS) $(target)
	$(eval rel_srcs = $(addprefix $(base_dir_name)/,$(SRCS)))
	$(eval tarball = $(parent_dir)/$(base_dir_name).tar.bz2)
	$(info tarball = $(tarball))
	@( \
		tar -C $(parent_dir) -cjf $(tarball) $(rel_srcs) \
			$(base_dir_name)/Makefile \
			$(base_dir_name)/$(target) \
	)
	$(info wrote $(tarball))

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
	@rm -f $(target) >/dev/null 2>&1
	@rm -f $(objs) $(comms_objs) $(comms_glue_objs) >/dev/null 2>&1
	@rm -f $(deps) >/dev/null 2>&1
	@rm -f $(stabs) >/dev/null 2>&1
	@rm -f $(gcovs) >/dev/null 2>&1
	@ -rmdir $(objs_dir) $(deps_dir) $(stab_dir) >/dev/null 2>&1

