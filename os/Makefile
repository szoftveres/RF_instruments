OBJDIR = .
SRCDIR = .
INCLDIR = .. .
OUTDIR = .

## General Flags
PROGRAM = instr_os
CC = gcc
LD = gcc
CFLAGS = -Wall -Wextra -O0 -I $(INCLDIR)

## Objects that must be built in order to link
OBJECTS =                                               \
                        $(OBJDIR)/analog.o              \
                        $(OBJDIR)/bda4700.o             \
                        $(OBJDIR)/blockdevice.o         \
                        $(OBJDIR)/commands.o            \
                        $(OBJDIR)/config.o              \
                        $(OBJDIR)/fatsmall_fs.o         \
                        $(OBJDIR)/fifo.o                \
                        $(OBJDIR)/fs_broker.o           \
                        $(OBJDIR)/globals.o             \
                        $(OBJDIR)/keyword.o             \
                        $(OBJDIR)/levelcal.o            \
                        $(OBJDIR)/max2871.o             \
                        $(OBJDIR)/parser.o              \
                        $(OBJDIR)/pcmstream.o           \
                        $(OBJDIR)/program.o             \
                        $(OBJDIR)/resource.o            \
                        $(OBJDIR)/taskscheduler.o       \
                        $(OBJDIR)/terminal_input.o      \

SUBDIRS = lex
.PHONY : $(SUBDIRS)

## Build both compiler and program
all: $(SUBDIRS) $(OBJECTS)

$(SUBDIRS):
	$(MAKE) -C $@

## Compile source files
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/$*.o $< 

clean:
	-rm -rf $(OBJECTS)
	for dir in $(SUBDIRS); do $(MAKE) clean -C $$dir; done

