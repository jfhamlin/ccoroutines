
#### Command, Flag Variables ####

# debug build is the default
CONFIG=debug

INCDIRS = inc deps

CFLAGS = $(addprefix -I,$(INCDIRS)) -std=c11

GCOV_OUTPUT = *.gcda *.gcno *.gcov
ifeq ($(CONFIG),debug)
  GCOV_CFLAGS = -fprofile-arcs -ftest-coverage
  DBG_CFLAGS = -g -DDEBUG $(GCOV_CFLAGS)
  CFLAGS += ${DBG_CFLAGS}
  OPTFLAGS = -O0
else
  ifneq ($(CONFIG),release)
    $(error Invalid value for CONFIG: $(CONFIG))
  endif

  OPTFLAGS = -O3
endif

CFLAGS += $(OPTFLAGS)

.PHONY: clean test debug

all: co.a

#### Directories, Sources, Intermediate Files ####

OUTDIR_BASE = ./bin
ifeq ($(CONFIG),debug)
  OUTDIR = $(OUTDIR_BASE)/debug
else
  OUTDIR = $(OUTDIR_BASE)/release
endif

SRCDIR = src

SRCS = src/co.c

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
	$(CC) $(CFLAGS) -c -o $@ $<

#### Concrete Target Rules ####

co.a: $(OBJS)
	ar crv $@ $^

### Test targets

TEST_FILES = $(wildcard test/*.c)
TEST_OBJECT_FILES = $(addprefix $(OUTDIR)/, $(subst .c,.o,$(TEST_FILES)))

include $(patsubst %,$(OUTDIR)/%, $(TEST_FILES:.c=.d))

test: INCDIRS += test
test: co.a $(TEST_OBJECT_FILES)
	$(CC) $(CFLAGS) -o test_prints $^
	./test_prints

#### Clean ####

clean:
	rm -rf $(OUTDIR_BASE)/* *~ co.a co.dSYM TAGS test_prints

#### flymake helper

.PHONY: check-syntax
check-syntax:
	$(CC) $(CFLAGS) -o /dev/null -S ${CHK_SOURCES}

#### tags

tags: $(SRCS)
	ctags -e -R -f TAGS $(SRCDIR) $(INCDIRS) test
