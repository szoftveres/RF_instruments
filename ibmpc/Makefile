OBJDIR = .
SRCDIR = .
INCLUDES = -I $(shell pwd)
OUTDIR = .

## General Flags
PROGRAM = instr_os_unix


# This is pretty much the config
SUBDIRS = os
.PHONY : $(SUBDIRS) clean

INCLUDES += $(foreach sd,$(SUBDIRS),-I ../$(sd))
BUILD_LIBS += $(foreach sd,$(SUBDIRS), $(sd)/$(sd).a)

$(info $(INCLUDES))

export PLATFORM_CC = ia16-elf-gcc
export PLATFORM_LD = ia16-elf-gcc
export PLATFORM_AR = ia16-elf-ar
export PLATFORM_CFLAGS = -Wall -O1 -fno-PIC -funsigned-char -nostdlib -ffreestanding -march=i8086 -mtune=i8086 -I /home/mk/mgc/embedded/codebench/lib/gcc/ia16-elf/6.2.0/include $(INCLUDES)
export PLATFORM_LDFLAGS = -march=i8086 -mtune=i8086 -nostdlib -ffreestanding -T ls.ld

## Objects that must be built in order to link
OBJECTS =                                               \
                        $(OBJDIR)/main.o                \
                        $(OBJDIR)/instances.o           \
                        $(OBJDIR)/hal_plat.o            \


## Build both compiler and program
all: $(PROGRAM).bin

$(SUBDIRS):
	if test ! -f $@/Makefile; then rm -rf $@ ; ln -s ../$@ $@ ; fi
	$(MAKE) -C $@

$(PROGRAM).bin: $(SUBDIRS) $(OBJECTS)
	$(PLATFORM_LD) $(PLATFORM_LDFLAGS) -o $(OBJDIR)/$(PROGRAM).bin $(OBJECTS) $(BUILD_LIBS)


## Compile source files
$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(PLATFORM_CC) $(PLATFORM_CFLAGS) -c -o $(OBJDIR)/$*.o $< 

clean:
	rm -rf $(OBJECTS)
	rm -rf $(PROGRAM).bin
	for dir in $(SUBDIRS); do $(MAKE) clean -C $$dir; done
	rm -rf $(SUBDIRS)

