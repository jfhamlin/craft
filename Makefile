
#### Command, Flag Variables ####

# debug build is the default
CONFIG=debug

INCDIR = inc

CFLAGS = -Wall -I$(INCDIR) -std=c11 # -std=c11 -stdlib=libc

ifeq ($(CONFIG),debug)
  DBG_CFLAGS = -g -DDEBUG
  CFLAGS += ${DBG_CFLAGS}
  OPTFLAGS = -O0
else
  ifneq ($(CONFIG),release)
    $(error Invalid value for CONFIG: $(CONFIG))
  endif

  OPTFLAGS = -O3
endif

CC=clang

PYTHON=python

.PHONY: clean test debug

all: raft

#### Directories, Sources, Intermediate Files ####

OUTDIR_BASE = ./bin
ifeq ($(CONFIG),debug)
  OUTDIR = $(OUTDIR_BASE)/debug
else
  OUTDIR = $(OUTDIR_BASE)/release
endif

SRCDIR = src

SRCS = src/raft_state.c src/raft.c src/raft_rpc.c src/raft_log.c \
	src/raft_util.c

OBJS = $(patsubst %,$(OUTDIR)/%, $(SRCS:.c=.o))

#### Auto Deps ####

include $(patsubst %,$(OUTDIR)/%, $(SRCS:.c=.d))

$(OUTDIR)/%.d: %.c
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) -MT "$(@:.d=.o) $@" $< > $@

#### General Rules ####

$(OUTDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(OPTFLAGS) -c -o $@ $<

#### Concrete Target Rules ####

raft: $(OBJS) $(OUTDIR)/src/test.o
	$(CC) $(CFLAGS) $(OPTFLAGS) -o $@ $^

### Test targets

test: $(OBJS) $(OUTDIR)/tests/test_main.o
	$(CC) $(CFLAGS) $(OPTFLAGS) -o $@ $^
	./test 2> /dev/null

#### Clean ####

clean:
	rm -rf $(OUTDIR_BASE)/* *~ raft raft.dSYM TAGS

#### flymake helper

.PHONY: check-syntax
check-syntax:
	$(CC) $(CFLAGS) $(OPTFLAGS) -o /dev/null -S ${CHK_SOURCES}

#### tags

tags: $(SRCS)
	ctags -e -R -f TAGS $(SRCDIR) $(INCDIR) tests
