SECTIONS
{
    . = 0x400000;
    .text : { *(.text) }
    . = 0x600000;
    .data : { *(.data) }
}