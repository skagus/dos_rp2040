
#include "hardware/flash.h"
#include "lfs.h"

extern uint32_t __flash_fs_start;
extern uint32_t __flash_fs_size;

//#define FLASH_PAGE_SIZE		(256)
//#define FLASH_SECTOR_SIZE	(4096)

#define FS_BASE		((uint32_t)(&__flash_fs_start) - XIP_BASE)
#define FS_SIZE		(uint32_t)(&__flash_fs_size)

static int block_read(
	const struct lfs_config *cfg,
	lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
	const uint8_t *src = (uint8_t*)(XIP_BASE + FS_BASE + 
									(block * cfg->block_size) + off);
	memcpy(buffer, src, size);
	return 0;
}

static int block_prog(
		const struct lfs_config *cfg,
		lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
	uint32_t addr = FS_BASE + (block * cfg->block_size) + off;
	flash_range_program(addr, buffer, size);
	return 0;
}

static int block_erase(const struct lfs_config *cfg, lfs_block_t block)
{
	uint32_t addr = FS_BASE + (block * cfg->block_size);
	flash_range_erase(addr, cfg->block_size);
	return 0;
}

static int block_sync(const struct lfs_config *cfg)
{
	return 0;
}

#define CACHE_SIZE			(512)
#define LOOKAHEAD_SIZE		(32)
#define NUM_CHUNK			(8)

uint8_t lookahead_buffer[LOOKAHEAD_SIZE];
uint8_t read_buffer[CACHE_SIZE];
uint8_t prog_buffer[CACHE_SIZE];

uint8_t buffer[NUM_CHUNK][CACHE_SIZE];
uint8_t* gp_free;

struct lfs_config cfg = {
	.read  = block_read,
	.prog  = block_prog,
	.erase = block_erase,
	.sync  = block_sync,

	.read_size = FLASH_PAGE_SIZE,
	.prog_size = FLASH_PAGE_SIZE,
	.block_size = FLASH_SECTOR_SIZE,
//	.block_count = (uint32_t)(&__flash_fs_size) / FLASH_SECTOR_SIZE;
	.cache_size = CACHE_SIZE,
	.lookahead_size = LOOKAHEAD_SIZE,
	.block_cycles = 500,
	.read_buffer = read_buffer,
	.prog_buffer = prog_buffer,
	.lookahead_buffer = lookahead_buffer,
	.name_max = 255,
	.attr_max = 512,
	.file_max = 1024 * 128,
};

lfs_t lfs;

void* lfs_malloc(size_t size)
{
	if(size != 512)
	{
		printf("lfs_malloc: %d\n", size);
	}
	if(NULL != gp_free)
	{
		uint8_t* p = gp_free;
		gp_free = *(uint8_t**)gp_free;
		return p;
	}
	else
	{
		printf("No memory\n");
		return NULL;
	}
}

void lfs_free(void* p)
{
	*(uint8_t**)p = gp_free;
	gp_free = p;
}

void my_lfs_init()
{
	cfg.block_count = FS_SIZE / FLASH_SECTOR_SIZE;

	for(int i=0; i< NUM_CHUNK; i++)
	{
		lfs_free(buffer[i]);
	}
}
