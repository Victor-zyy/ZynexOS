OUTPUT_ARCH("riscv")

ENTRY(start)

MEMORY
{
	flash (rxai) : ORIGIN = 0x20000000, LENGTH = 512K
        sram  (rxaiw)  : ORIGIN = 0x00008000, LENGTH = 32K
}


SECTIONS
{
	.text :
	{
		KEEP(*(.text))

                . = ALIGN(8);

                _etext = .;
                _sidata = _etext;
	} > flash

        . = SIZEOF(.text);

        .data :
        {
            . = ALIGN(4);
            
            _sdata = .;

            KEEP(*(.data*));
            KEEP(*(.got*));

            . = ALIGN(4);
            _edata = .;

        } > sram AT > flash /* LMA flash */
}
