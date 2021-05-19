BIN = lang

all: $(BIN)

lang: build/lang.tab.c
	gcc -g -o $@ $+

build/lex.yy.c: src/lex.l |build
	lex -o $@ $<

build/lang.tab.c: src/lang.y build/lex.yy.c |build
	bison -o $@ $< -v --report-file=report.bison

build:
	mkdir -p build

clean:
	rm -rf build
	rm -f $(BIN)
	rm -f report.bison

.PHONY: clean
