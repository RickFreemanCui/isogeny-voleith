MAIN    = main
LATEX   = pdflatex
BIBER   = biber
FLAGS   = -interaction=nonstopmode -halt-on-error -file-line-error

.PHONY: all clean distclean view watch

all: $(MAIN).pdf

$(MAIN).pdf: $(MAIN).tex references.bib
	$(LATEX) $(FLAGS) $(MAIN).tex
	$(BIBER) $(MAIN)
	$(LATEX) $(FLAGS) $(MAIN).tex
	$(LATEX) $(FLAGS) $(MAIN).tex

quick:
	$(LATEX) $(FLAGS) $(MAIN).tex

view: $(MAIN).pdf
	open $(MAIN).pdf

watch:
	latexmk -pdf -pvc -interaction=nonstopmode $(MAIN).tex

clean:
	rm -f *.aux *.log *.toc *.out *.bbl *.bcf *.blg *.run.xml *.fls *.fdb_latexmk *.synctex.gz *.nav *.snm *.vrb

distclean: clean
	rm -f $(MAIN).pdf
