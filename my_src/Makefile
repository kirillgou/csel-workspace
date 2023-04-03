DIRS=$(filter-out Makefile, $(wildcard *))

all clean install:
	# cd modules_noyaux/ex1 && make;
	for dir in $(DIRS); do $(MAKE) $@ -C $$dir; done
