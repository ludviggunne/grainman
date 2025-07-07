prefix=.
override CFLAGS:=-Wall -O3 -Iexternal $(CFLAGS)
override LDFLAGS:=-lsndfile -lsamplerate -ljack -lm $(LDFLAGS)

grainman_sources:=\
	$(wildcard src/*.c) \
	$(wildcard src/cmd/*.c) \
	external/linenoise/linenoise.c

grainman_objects:=$(grainman_sources:%.c=%.o)
grainman_dependencies:=$(grainman_sources:%.c=%.d)

tools_sources:=$(wildcard tools/*.c)
tools=$(tools_sources:%.c=%)

all: grainman $(tools)

debug: CFLAGS+=-g -O0 -Wno-cpp
debug: all

grainman: $(grainman_objects)
	$(CC) $(LDFLAGS) -o $(@) $(^)

tools/midi-dump: LDFLAGS=-ljack
tools/jack-record: LDFLAGS=-ljack -lsndfile
tools/%: tools/%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(@) $(<)

-include $(grainman_dependencies)

%.o: %.c
	$(CC) -MMD $(CFLAGS) -o $(@) -c $(<)

clean:
	rm -rf $(grainman_objects) $(grainman_dependencies) grainman $(tools)

install:
	install -Dm755 grainman $(prefix)/bin/grainman
	install -Dm755 midi-dump $(prefix)/bin/midi-dump

.PHONY: clean install
