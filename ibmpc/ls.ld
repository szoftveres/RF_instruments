
OUTPUT_ARCH(i8086)

SECTIONS
{
  . = 0x0000;
  .text : {
    *(.text*)
    }
  .data : {
    *(.data*)
    *(.rodata*) 
  }
  .bss : { *(.bss*) }
}
