/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lfs.h"
#include "lfs_util.h"
#include "cli.h"
#include "elf_loader.h"
#include "elf.h"  // documentation: "man 5 elf"

#define EN_REAL_LOAD		(1)
#define MAX_MEM_CHUNK		(16)


typedef struct _mem_chunk
{
	uint32_t address;
	uint32_t load_size;
	uint32_t mem_size;
	uint32_t file_offset;
} mem_chunk;

extern lfs_t lfs;

static bool verbose = true;
static mem_chunk a_mem_chunks[MAX_MEM_CHUNK];

static int read_elf_file_header(uint32_t* p_entry, struct lfs_file* p_file)
{
	Elf32_Ehdr ehdr;

	if(lfs_file_read(&lfs, p_file, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
	{
		printf("ERROR: could not read the elf file header !\n");
		return -7;
	}
	// magic numbers in header
	if((ELFMAG0 != ehdr.e_ident[EI_MAG0])
	   || (ELFMAG1 != ehdr.e_ident[EI_MAG1])
	   || (ELFMAG2 != ehdr.e_ident[EI_MAG2])
	   || (ELFMAG3 != ehdr.e_ident[EI_MAG3]))
	{
		printf("not an elf file !\n");
		return -9;
	}
	if(ELFCLASS32 != ehdr.e_ident[EI_CLASS])
	{
		printf("ERROR: not an 32 bit elf file !\n");
		return -10;
	}
	if(true == verbose)
	{
		printf("Entry point at 0x%08lX !\n", (uint32_t)ehdr.e_entry);
		// program header
		if(0 == ehdr.e_phoff)
		{
			printf("No Program header table !\n");
		}
		else
		{
			printf("Program header table at offset 0x%lX !\n", (uint32_t)ehdr.e_phoff);
			printf("program header entries are 0x%X bytes long\n", (uint16_t)ehdr.e_phentsize);
			printf("program header table has %d entries\n", (uint16_t)ehdr.e_phnum);
		}
		// section header
		if(0 == ehdr.e_shoff)
		{
			printf("No section header table !\n");
		}
		else
		{
			printf("section header table at offset 0x%lX !\n", (uint32_t)ehdr.e_shoff);
			printf("section header entries are 0x%X bytes long\n", (uint16_t)ehdr.e_shentsize);
			printf("section header table has %d entries\n", (uint16_t)ehdr.e_shnum);
		}
	}
	if(0 == ehdr.e_phoff)
	{
		printf("ERROR: does not contain a program header table !\n");
		return -11;
	}
	if(ehdr.e_phentsize != sizeof(Elf32_Phdr))
	{
		printf("ERROR: reports wrong program header table entry size !\n");
		return -12;
	}
	if(1 > ehdr.e_phnum)
	{
		printf("ERROR: contains an empty program header table !\n");
		return -13;
	}
	if((uint16_t)PN_XNUM < ehdr.e_phnum)
	{
		printf("ERROR: contains an program header table with an invalid number of entries!\n");
		return -15;
	}
	*p_entry = ehdr.e_entry;
	return ehdr.e_phnum;
}

void add_mem_chunk(int index, uint32_t target_addr, uint32_t load_size, uint32_t mem_size, uint32_t file_offset)
{
	mem_chunk* p_mem = a_mem_chunks + index;
	printf("size:%4lX(%4lX)  target addr: %8lX, file offset: %lX\n",
		load_size, mem_size, target_addr, file_offset);
	p_mem->address = target_addr;
	p_mem->load_size = load_size;
	p_mem->mem_size = mem_size;
	p_mem->file_offset = file_offset;
}

#define LOAD_SIZE		(64)
static void dump_mem(int cnt_load,  struct lfs_file* p_file)
{
	for(int i = 0; i < cnt_load; i++)
	{
		mem_chunk* p_mem = a_mem_chunks + i;
		
		printf("chunk %2d size:%4lX(%4lX)  target addr: %8lX, file offset: %lX\n",
				i, p_mem->load_size, p_mem->mem_size, p_mem->address, p_mem->file_offset);

		if(p_mem->file_offset > 0)
		{
			lfs_file_seek(&lfs, p_file, p_mem->file_offset, LFS_SEEK_SET);

			int loaded = 0;
			uint8_t* dst = (uint8_t*)p_mem->address;
	#if (0 == EN_REAL_LOAD)
			uint8_t buffer[LOAD_SIZE];
	#endif
			while(loaded < p_mem->load_size)
			{
				int to_read = p_mem->load_size - loaded;
				if(to_read > LOAD_SIZE)
				{
					to_read = LOAD_SIZE;
				}
	#if (0 == EN_REAL_LOAD)
				lfs_file_read(&lfs, p_file, buffer, to_read);
				printf("0x%8X : ", dst);
				for(int i = 0; i< 4; i++)
				{
					printf("%02X ", buffer[i]);
				}
				printf("\n");
				break;
	#else
				lfs_file_read(&lfs, p_file, dst, to_read);
	#endif
				dst += to_read;
				loaded += to_read;
			}

			uint32_t fill_size = p_mem->mem_size - p_mem->load_size;
			if(fill_size > 0)
			{
				memset((void*)(p_mem->address + p_mem->load_size), 0, fill_size);
			}

			printf("load done\n");
		}
		else
		{
			memset((void*)p_mem->address, 0, p_mem->load_size);
			printf("zero fill done\n");
		}
	}
}


static int read_program_table(int cnt_ptbl, struct lfs_file* p_file)
{
	Elf32_Phdr program_table;
	int cnt_load = 0;
	for(int prog_entry = 0; prog_entry < cnt_ptbl; prog_entry++)
	{
		if(lfs_file_read(&lfs, p_file, &program_table, sizeof(Elf32_Phdr)) != sizeof(Elf32_Phdr))
		{
			printf("ERROR: could not read the program table !\n");
			return 8;
		}

		// check Program Table Entry
		uint32_t target_start_addr;
		if(PT_LOAD == program_table.p_type)
		{
			if(0 == program_table.p_paddr)
			{
				// physical address is 0
				// -> use virtual address
				target_start_addr = program_table.p_vaddr;
			}
			else
			{
				// physical address is valid
				// -> use it
				target_start_addr = program_table.p_paddr;
			}

			if((0 == program_table.p_memsz) || (0 == program_table.p_filesz))
			{
				// No data for that section
				continue;
			}

			add_mem_chunk(cnt_load, program_table.p_vaddr, program_table.p_filesz, program_table.p_memsz, program_table.p_offset);
			cnt_load++;
		}
		else
		{
			printf("Non loadable : %X, %X, %X, %X, %X\n", 
				program_table.p_type,
				program_table.p_vaddr,
				program_table.p_paddr,
				program_table.p_memsz,
				program_table.p_flags);
		}
	}
	return cnt_load;
}

typedef int (MainFunc)(int argc, char* argv[]);
typedef MainFunc* FuncPtr;

int load(FuncPtr* entry, char* filename)
{
	struct lfs_file elf_file;
	if(lfs_file_open(&lfs, &elf_file, filename, LFS_O_RDONLY) < 0)
	{
		printf("ERROR: could not open the elf file %s !\n", filename);
		return -6;
	}

	// read elf file header
	int cnt_ptbl = read_elf_file_header((uint32_t*)entry, &elf_file);
	if(cnt_ptbl <= 0)
	{
		// something went wrong -> exit
		lfs_file_close(&lfs, &elf_file);
		return cnt_ptbl;
	}
	// else -> go on
	int cnt_load = read_program_table(cnt_ptbl, &elf_file);
	if(cnt_load <= 0)
	{
		// something went wrong -> exit
		lfs_file_close(&lfs, &elf_file);
		return cnt_load;
	}
	dump_mem(cnt_load, &elf_file);
	lfs_file_close(&lfs, &elf_file);
	return 0;
}

FuncPtr gp_entry;

void cmd_Load(uint8_t argc, char* argv[])
{
	if(argc < 2)
	{
		printf("needs file name\n");
		return;
	}
	int load_result = load(&gp_entry, argv[1]);
	if(load_result < 0)
	{
		printf("load error: %d\n", load_result);
		return;
	}

//	printf("Pass : %X, %X, %X\n", &(argv[1]), atoi(argv[2]), *argv[2]);

	int ret = gp_entry(argc - 1, &(argv[1]));
	printf("\n\nResult: %d\n", ret);
}

void cmd_MemDump(uint8_t argc, char* argv[])
{
	if(argc < 2)
	{
		printf("Need Address.\n");
		return;
	}

	uint32_t nAddr = CLI_GetInt(argv[1]);
	uint8_t* pAddr = (uint8_t*)nAddr;
	printf("Dumping memory at %p\n",pAddr);
	for(uint8_t nIdx = 0; nIdx < 16; nIdx++)
	{
		printf("%02X ",pAddr[nIdx]);
	}
	printf("\n");
}

void cmd_Jump(uint8_t argc, char* argv[])
{
	if(argc > 1)
	{
		gp_entry = (FuncPtr)CLI_GetInt(argv[1]);
	}
	printf("Entry point: %p\n", gp_entry);
	int ret = gp_entry(0xAA, 0xBB);
	printf("\n\nResult: %X\n", ret);
}

void Loader_Init()
{
	CLI_Register("load", cmd_Load);
	CLI_Register("jump", cmd_Jump);
	CLI_Register("dump", cmd_MemDump);
}
