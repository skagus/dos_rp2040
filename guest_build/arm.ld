
ENTRY(main)


/* Memory Spaces Definitions */

MEMORY
{
	FLASH (rx) : ORIGIN = 0x20020000, LENGTH = 64K
	RAM (rwx)  : ORIGIN = 0x20030000, LENGTH = 64K
}

/* highest address of the user mode stack */

SECTIONS
{
	.text :
	{
		. = ALIGN(4);
		KEEP(*(.entry)) /* Startup code */
	/*	*(EXCLUDE_FILE(*main.c.o).text*) */
		*(.text*)
		*(.rodata)                 /* read-only data (constants) */
		*(.rodata.*)
		*(.glue_7)
		*(.glue_7t)
		*(.vfp11_veneer)
		*(.v4_bx)
	} >FLASH

	/* for exception handling/unwind - some Newlib functions (in
	common with C++ and STDC++) use this. */
	.ARM.extab :
	{
		. = ALIGN(4);
		*(.ARM.extab* .gnu.linkonce.armextab.*)
		. = ALIGN(4);
	} > FLASH

	.ARM :
	{
		. = ALIGN(4);
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
		. = ALIGN(4);
	} > FLASH

	.preinit_array :
	{
		. = ALIGN(4);
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array*))
		PROVIDE_HIDDEN (__preinit_array_end = .);
		. = ALIGN(4);
	} >FLASH

	.init_array :
	{
		. = ALIGN(4);
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array*))
		PROVIDE_HIDDEN (__init_array_end = .);
		. = ALIGN(4);
	} >FLASH

	.fini_array :
	{
		. = ALIGN(4);
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT(.fini_array.*)))
		KEEP (*(.fini_array*))
		PROVIDE_HIDDEN (__fini_array_end = .);
		. = ALIGN(4);
	} >FLASH

	/* This is used by the startup in order to initialize the .data secion*/
	_sidata = LOADADDR(.data);

	/* This is the initialized data section
	The program executes knowing that the data is in the RAM
	but the loader puts the initial values in the FLASH (inidata).
	It is one task of the startup to copy the initial values from FLASH to RAM. */
	.data :
	{
		. = ALIGN(4);
		/* This is used by the startup in order to initialize the .data secion */
		_sdata = . ;
		*(.data*)
		KEEP(*(.dbg));
		. = ALIGN(4);
		/* This is used by the startup in order to initialize the .data secion */
		_edata = . ;
	} >RAM

	.noinit :
	{
		*(.noinit*)
	} > RAM
	/* This is the uninitialized data section */
	.bss :
	{
		. = ALIGN(4);
		/* This is used by the startup in order to initialize the .bss secion */
		_sbss = .;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		/* This is used by the startup in order to initialize the .bss section */
		_ebss = . ;
	} >RAM

	.noinit(NOLOAD) :
	{
		. = ALIGN(4);
		KEEP(*(.noinit))
		. = ALIGN(4);
	} >RAM

	/* _sbrk needs 'end' */
	PROVIDE ( end = _ebss );

	/* after that it's only debugging information. */
	/* remove the debugging information from the standard libraries */
/*
	DISCARD :
	{
		libc.a ( * )
		libm.a ( * )
		libgcc.a ( * )
	}
*/

	/* Stabs debugging sections.  */
	.stab          0 : { *(.stab) }
	.stabstr       0 : { *(.stabstr) }
	.stab.excl     0 : { *(.stab.excl) }
	.stab.exclstr  0 : { *(.stab.exclstr) }
	.stab.index    0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment       0 : { *(.comment) }
	/* DWARF debug sections.
		Symbols in the DWARF debugging sections are relative to the beginning
		of the section so we begin them at 0.  */
    /* DWARF 1 */
	.debug          0 : { *(.debug) }
	.line           0 : { *(.line) }
	/* GNU DWARF 1 extensions */
	.debug_srcinfo  0 : { *(.debug_srcinfo) }
	.debug_sfnames  0 : { *(.debug_sfnames) }
	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	/* DWARF 2 */
	.debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_macinfo  0 : { *(.debug_macinfo) }
	/* SGI/MIPS DWARF 2 extensions */
	.debug_weaknames 0 : { *(.debug_weaknames) }
	.debug_funcnames 0 : { *(.debug_funcnames) }
	.debug_typenames 0 : { *(.debug_typenames) }
	.debug_varnames  0 : { *(.debug_varnames) }
}
