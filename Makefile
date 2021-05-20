BIN = lang
CFLAGS = -g -Wall -Wextra -Wpedantic -Wconversion -Wshadow

all: $(BIN)

.SUFFIXES:

CSRC = $(wildcard src/*.c)
HSRC = $(wildcard src/*.h)
CCPY = $(CSRC:src/%=build/%)
HCPY = $(HSRC:src/%=build/%)
COBJ = $(CCPY:%.c=%.o)
CDEP = $(COBJ:%.o=%.d)

# when compiling produce a .d file as well
%.o: %.c
	gcc -c -o $@ $(CFLAGS) -MD -MP -MF ${@:.o=.d} $<

# don't fail on missing .d files
# there won't be any on the first run
-include $(CDEP)

lang: $(COBJ) $(HCPY) build/lang.tab.c
	gcc -o $@ $+

build/%.h: src/%.h |build
	cp $< $@

build/%.c: src/%.c build/%.h |build
	cp $< $@

build/lex.yy.c: src/lex.l |build
	lex -o $@ $<

build/lang.tab.c: src/lang.y build/lex.yy.c |build
	bison -o $@ $< -v --report-file=report.bison

build:
	mkdir -p build
	cp src/* build/

clean:
	rm -rf build
	rm -f $(BIN)
	rm -f report.bison

.PHONY: clean
