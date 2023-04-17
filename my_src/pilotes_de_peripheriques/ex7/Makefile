DIRS=$(filter-out Makefile, $(wildcard *))

all clean install clean_all:
	# cd modules_noyaux/ex1 && make;
	for dir in $(DIRS); do $(MAKE) $@ -C $$dir; done
