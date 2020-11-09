SECTIONS
{
    . = 0x20000000;
    .text : { *(.text) }
    . = 0x20200000;
    .data : { *(.data) }
}