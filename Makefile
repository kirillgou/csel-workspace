SRCS= rapport_csel.md#$(wildcard *.md)
PDFS=$(SRCS:%.md=%.pdf)
UID=$(shell id -u)
GID=$(shell id -g)

doc: $(PDFS)

all: doc

# Thanks to Florent Gluck for the following makefile
%.pdf: %.md
	docker run --user $(UID):$(GID) --rm --mount type=bind,src="$(PWD)",dst=/src thxbb12/md2pdf build_lab $<

clean:
	rm -f $(PDFS) $(PDFS_CORR)

