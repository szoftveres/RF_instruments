OBJDIR = .
SRCDIR = .
INCLUDES = -I $(shell pwd)
export UPPER_INCLUDES = $(INCLUDES)
OUTDIR = .

## General Flags
PROGRAM = instr_os_unix
CC = gcc
LD = gcc
CFLAGS = -Wall -Wextra -O0 $(INCLUDES)
LDFLAGS = -lm -lpulse-simple

## Objects that must be built in order to link
OBJECTS =                                               \
                        $(OBJDIR)/main.o                \
                        $(OBJDIR)/adcdac.o              \
                        $(OBJDIR)/instances.o           \
                        $(OBJDIR)/unixfs_wrapper.o      \
                        $(OBJDIR)/hal_plat.o            \



LD_OBJECTS =                                     \
                        os/lex/lex.o             \
                        os/bda4700.o             \
                        os/blockdevice.o         \
                        os/commands.o            \
                        os/config.o              \
                        os/dsp_maths.o           \
                        os/fatsmall_fs.o         \
                        os/fifo.o                \
                        os/fs_broker.o           \
                        os/globals.o             \
                        os/keyword.o             \
                        os/levelcal.o            \
                        os/max2871.o             \
                        os/modem.o               \
                        os/parser.o              \
                        os/pcmstream.o           \
                        os/program.o             \
                        os/resource.o            \
                        os/taskscheduler.o       \
                        os/terminal_input.o      \
                        adcdac.o                 \
                        hal_plat.o               \
                        instances.o              \
                        unixfs_wrapper.o         \
                        main.o                   \


SUBDIRS = os
.PHONY : $(SUBDIRS)

## Build both compiler and program
all: $(PROGRAM)


$(SUBDIRS):
	if test ! -f os/Makefile; then rm -rf os ; ln -s ../os os ; fi
	$(MAKE) -C $@

$(PROGRAM): $(SUBDIRS) $(OBJECTS)
	$(LD) -o $(OBJDIR)/$(PROGRAM).bin $(LD_OBJECTS) $(LDFLAGS)


## Compile source files
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/$*.o $< 

clean:
	-rm -rf $(OBJECTS)
	for dir in $(SUBDIRS); do $(MAKE) clean -C $$dir; done

