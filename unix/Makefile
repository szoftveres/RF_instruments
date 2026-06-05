OBJDIR = .
SRCDIR = .
INCLUDES = -I $(shell pwd)
OUTDIR = .

## General Flags
PROGRAM = instr_os_unix


# This is pretty much the config
SUBDIRS = pcmstream os
.PHONY : $(SUBDIRS) clean

INCLUDES += $(foreach sd,$(SUBDIRS),-I ../$(sd))
BUILD_LIBS += $(foreach sd,$(SUBDIRS), $(sd)/$(sd).a)

$(info $(INCLUDES))

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


## Build both compiler and program
all: $(PROGRAM).bin

$(SUBDIRS):
	if test ! -f $@/Makefile; then rm -rf $@ ; ln -s ../$@ $@ ; fi
	$(MAKE) -C $@

$(PROGRAM).bin: $(SUBDIRS) $(OBJECTS)
	$(PLATFORM_LD) $(OBJECTS) $(BUILD_LIBS) $(PLATFORM_LDFLAGS) -o $(OBJDIR)/$(PROGRAM).bin


## Compile source files
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(PLATFORM_CC) $(PLATFORM_CFLAGS) -c -o $(OBJDIR)/$*.o $< 

clean:
	rm -rf $(OBJECTS)
	rm -rf $(PROGRAM).bin
	for dir in $(SUBDIRS); do $(MAKE) clean -C $$dir; done
	rm -rf $(SUBDIRS)

