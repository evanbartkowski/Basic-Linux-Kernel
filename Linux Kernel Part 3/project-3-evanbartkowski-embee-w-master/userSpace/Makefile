# Compiler/flags
CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -std=gnu11 -D_FILE_OFFSET_BITS=64 -Wno-type-limits
LDFLAGS = `pkg-config fuse --cflags --libs`

BIN_MEMEFS = memefs
BIN_MKMEMEFS = mkmemefs

IMG = filesystem.img
MOUNTDIR = mountpoint
LABEL = "label"

all: $(BIN_MEMEFS) $(BIN_MKMEMEFS)

#FUSE driver
$(BIN_MEMEFS): memefs.c driver_funcs.o img_funcs.o
	$(CC) $(CFLAGS) memefs.c driver_funcs.o img_funcs.o $(LDFLAGS) -o $(BIN_MEMEFS)

driver_funcs.o:	driver_funcs.h driver_funcs.c img_funcs.h
	$(CC) $(CFLAGS) -c driver_funcs.c $(LDFLAGS)

img_funcs.o: img_funcs.h img_funcs.c
	$(CC) $(CFLAGS) -c img_funcs.c


#image creator
$(BIN_MKMEMEFS): mkmemefs.c
	$(CC) $(CFLAGS) mkmemefs.c -o $(BIN_MKMEMEFS)

#tools
.PHONY: image
.PHONY: mount
.PHONY: mount_debug
.PHONY: mount_valgrind
.PHONY: umount
.PHONY: clean

image:
	./mkmemefs $(IMG)

mount:
	mkdir -p $(MOUNTDIR)
	./memefs --volume_label=$(LABEL) $(MOUNTDIR) $(IMG)

mount_debug:
	mkdir -p $(MOUNTDIR)
	./memefs --volume_label=$(LABEL) -f $(MOUNTDIR) $(IMG)

mount_valgrind:
	mkdir -p $(MOUNTDIR)
	valgrind ./memefs --volume_label=$(LABEL)  $(MOUNTDIR) $(IMG)

umount:
	umount $(MOUNTDIR)

clean:
	rm -f $(BIN_MEMEFS) $(BIN_MKMEMEFS)
	rm -f *.o
