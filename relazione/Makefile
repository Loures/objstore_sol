FILES=$(filter-out Makefile relazione.tex, $(wildcard *))
relazione:
	@latexmk -shell-escape -synctex=1 -interaction=nonstopmode -file-line-error relazione.tex
	pdflatex -shell-escape -synctex=1 -interaction=nonstopmode -file-line-error relazione.tex
	
clean:
	$(RM) -r $(FILES)

.PHONY: clean