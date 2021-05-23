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

build/%.h: src/%.h Makefile |build
	cp $< $@

build/%.c: src/%.c build/%.h Makefile |build 
	cp $< $@

build/lex.yy.c: src/lex.l Makefile |build
	lex -o $@ $<

build/lang.tab.c: src/lang.y build/lex.yy.c Makefile |build
	bison -o $@ $< -v --report-file=report.bison

build:
	mkdir -p build
	cp src/* build/

valgrind: lang Makefile
	@for f in assets/*.prog; do \
		valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all \
			./lang $$f --all --rand --trace \
			1>/dev/null 2>vg.report; \
		if grep "no leaks are possible" vg.report &>/dev/null; then \
			echo "With $$f: no leaks are possible"; \
		else \
			cat vg.report; \
			exit 1; \
		fi; \
	done

README.pdf: tex/*.tex tex/sample-ast.dump tex/sample-repr.dump build/sort.prog.png Makefile
	cd tex; \
	pdflatex \
		-jobname=README \
		-output-directory=../build \
		--interaction=nonstopmode --halt-on-error \
		main.tex
	mv {build,.}/README.pdf

# sample executions for --ast and --repr
tex/sample-ast.dump: lang Makefile
	make FILE=lock ARG=ast create-dump

tex/sample-repr.dump: lang Makefile
	make FILE=lock ARG=repr create-dump

create-dump: lang Makefile
	CMD="./lang assets/${FILE}.prog --${ARG} -c" ; \
	DEST="tex/sample-${ARG}.dump" ; \
		echo "Executing $$CMD" ; \
		echo "\\tbf{Sample}: \\ttt{$$CMD}" > $$DEST ; \
		echo "\\begin{lstlisting}" >> $$DEST ; \
		$$CMD >> $$DEST ; \
		echo "\\end{lstlisting}" >> $$DEST

build/%prog.png: lang
	./lang assets/sort.prog --dot

clean:
	rm -rf build
	rm -f $(BIN)
	rm -f report.bison vgcore.* vg.report
	rm -f assets/*.png assets/*.dot
	rm -f tex/*.dump

.PHONY: clean
