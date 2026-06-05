OBJDIR = .
SRCDIR = .
INCLDIR = .
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
PROGRAM = bda4700

## Objects that must be built in order to link
OBJECTS = $(OBJDIR)/bda4700.o

## Build both compiler and program
all: $(PROGRAM).a

$(PROGRAM).a: $(OBJECTS)
	$(PLATFORM_AR) rcs $(PROGRAM).a $(OBJECTS)

## Compile source files
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(PLATFORM_CC) $(PLATFORM_CFLAGS) -c -o $(OBJDIR)/$*.o $< 

clean:
	-rm -rf $(OBJECTS)
	-rm -rf $(PROGRAM).a

