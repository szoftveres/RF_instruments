OBJDIR = .
SRCDIR = .
INCLUDES = -I $(shell pwd)
OUTDIR = .

## General Flags
PROGRAM = instr_os_unix

export PLATFORM_CC = gcc
export PLATFORM_LD = gcc
export PLATFORM_AR = ar
export PLATFORM_CFLAGS = -Wall -Wextra -O0 $(INCLUDES)
export PLATFORM_LDFLAGS = -lm -lpulse-simple

## Objects that must be built in order to link
OBJECTS =                                               \
                        $(OBJDIR)/main.o                \
                        $(OBJDIR)/adcdac.o              \
                        $(OBJDIR)/instances.o           \
                        $(OBJDIR)/unixfs_wrapper.o      \
                        $(OBJDIR)/hal_plat.o            \

SUBDIRS = os
.PHONY : $(SUBDIRS)

## Build both compiler and program
all: $(PROGRAM).bin

$(SUBDIRS):
	if test ! -f os/Makefile; then rm -rf os ; ln -s ../os os ; fi
	$(MAKE) -C $@

$(PROGRAM).bin: $(SUBDIRS) $(OBJECTS)
	$(PLATFORM_LD) $(OBJECTS) os/os.a $(PLATFORM_LDFLAGS) -o $(OBJDIR)/$(PROGRAM).bin


## Compile source files
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(PLATFORM_CC) $(PLATFORM_CFLAGS) -c -o $(OBJDIR)/$*.o $< 

clean:
	rm -rf $(OBJECTS)
	rm -rf $(PROGRAM).bin
	for dir in $(SUBDIRS); do $(MAKE) clean -C $$dir; done

