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
	@F=assets/sort.prog ; \
		for FLAGS in \
			"-a $$F" \
			"-r $$F" \
			"-d $$F" \
			"-ard $$F" \
			"-At $$F" \
			"-Rt $$F" \
			"-a" \
			"-ardARt $$F" \
			"--invalid" \
			"-a no-such-file" \
		; do \
		valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all \
			./lang $$f $$FLAGS \
			1>/dev/null 2>vg.report; \
		if grep "no leaks are possible" vg.report &>/dev/null; then \
			echo "With $$FLAGS: no leaks are possible"; \
		else \
			cat vg.report; \
			exit 1; \
		fi; \
	done

README.pdf: tex/*.tex tex/ast.dump tex/repr.dump tex/trace.dump assets/sort.prog.png
	cd tex; \
	pdflatex \
		-jobname=README \
		-output-directory=../build \
		--interaction=nonstopmode --halt-on-error \
		main.tex
	mv {build,.}/README.pdf


# sample executions for --ast and --repr
tex/ast.dump:
	make FILE=lock ARG=ast EXTRA= create-dump

tex/repr.dump:
	make FILE=lock ARG=repr EXTRA= create-dump

tex/trace.dump:
	make FILE=lock ARG=trace EXTRA=-A create-dump

create-dump: lang
	CMD="./lang assets/$(FILE).prog --$(ARG) $(EXTRA) -c" ; \
	DEST="tex/$(ARG).dump" ; \
		echo "Executing $$CMD" ; \
		echo "\\begin{lstlisting}" > $$DEST ; \
		echo "$$ $$CMD" >> $$DEST ; \
		$$CMD >> $$DEST ; \
		echo "\\end{lstlisting}" >> $$DEST

assets/sort.prog.png: lang assets/sort.prog
	./lang assets/sort.prog --dot

ARCHIVE = NVILLANI_LF-Project
tar:
	make README.pdf
	make clean
	mkdir $(ARCHIVE)
	cp -r assets $(ARCHIVE)/
	cp -r src $(ARCHIVE)/
	cp -r Makefile README.pdf $(ARCHIVE)/
	tar czf $(ARCHIVE).tar.gz $(ARCHIVE)

clean:
	rm -rf build
	rm -f $(BIN)
	rm -f report.bison vgcore.* vg.report
	rm -f assets/*.png assets/*.dot
	rm -f tex/*.dump
	rm -rf $(ARCHIVE) $(ARCHIVE).tar.gz

.PHONY: clean tar valgrind
