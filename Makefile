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

-include $(CDEP)

lang: $(COBJ) $(HCPY) build/lang.tab.c
	gcc -o $@ $(CFLAGS) $+

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

valgrind: lang
	@for f in assets/*.prog; do \
		valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all \
			./lang $$f --all --rand \
			1>/dev/null 2>vg.report; \
		if grep "no leaks are possible" vg.report &>/dev/null; then \
			echo "With $$f: no leaks are possible"; \
		else \
			cat vg.report; \
			exit 1; \
		fi; \
	done

clean:
	rm -rf build
	rm -f $(BIN)
	rm -f report.bison vgcore.* vg.report
	rm -f assets/*.png assets/*.dot

.PHONY: clean
