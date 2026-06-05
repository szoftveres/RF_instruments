OBJDIR = .
SRCDIR = .
INCLUDES = -I $(shell pwd)
OUTDIR = .

ifndef PLATFORM_CC
$(error PLATFORM_CC undefined)
endif
ifndef PLATFORM_CFLAGS
$(error PLATFORM_CFLAGS undefined)
endif
ifndef PLATFORM_AR
$(error PLATFORM_AR undefined)
endif

## General Flags
PROGRAM = os

## Objects that must be built in order to link
OBJECTS =                                               \
                        $(OBJDIR)/blockdevice.o         \
                        $(OBJDIR)/commands.o            \
                        $(OBJDIR)/dsp_maths.o           \
                        $(OBJDIR)/fatsmall_fs.o         \
                        $(OBJDIR)/fifo.o                \
                        $(OBJDIR)/fs_broker.o           \
                        $(OBJDIR)/globals.o             \
                        $(OBJDIR)/keyword.o             \
                        $(OBJDIR)/modem.o               \
                        $(OBJDIR)/parser.o              \
                        $(OBJDIR)/program.o             \
                        $(OBJDIR)/resource.o            \
                        $(OBJDIR)/taskscheduler.o       \
                        $(OBJDIR)/terminal_input.o      \

SUBDIRS = lex
.PHONY : $(SUBDIRS)

## Build both compiler and program
all: $(PROGRAM).a

$(PROGRAM).a: $(SUBDIRS) $(OBJECTS)
	$(PLATFORM_AR) -x lex/lex.a ## TODO: cleanup the object after this
	$(PLATFORM_AR) rcs $(PROGRAM).a *.o

$(SUBDIRS):
	if test ! -f lex/Makefile; then rm -rf lex ; git clone https://github.com/szoftveres/lex.git ../lex; ln -s ../lex lex ; fi
	$(MAKE) -C $@

## Compile source files
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(PLATFORM_CC) $(PLATFORM_CFLAGS) -c -o $(OBJDIR)/$*.o $< 

clean:
	rm -rf $(OBJECTS)
	rm -rf $(PROGRAM).a
	for dir in $(SUBDIRS); do $(MAKE) clean -C $$dir; done
	rm -rf $(SUBDIRS)

