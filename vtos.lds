OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS {
	. = 0x30000000;
	
	. = ALIGN(4);
	.text :
	{
	obj/start_s.o (.text)
	obj/start_c.o (.text)
	obj/nandflash.o (.text)
	*(.text)
	}

	. = ALIGN(4);
	.data : 
	{
	*(.data)
	}
	
	. = ALIGN(4);
	bss_start = .;
	.bss : 
	{
	*(.bss) 
	}
	bss_end = .;
	. = ALIGN(4);
	heap_start = .;
}
