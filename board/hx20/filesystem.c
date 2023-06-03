#include "lfs.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "inttypes.h"
#include "util.h"
#include "hooks.h"
#include "console.h"
#include "spi_flash.h"
#include "gpio.h"

#define CPUTS(outstr) cputs(CC_SYSTEM, outstr)
#define CPRINTS(format, args...) cprints(CC_SYSTEM, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_SYSTEM, format, ## args)

// The MEC1521 in the hx20 is paired with a 1MB SPI flash chip.
// Let's dump a filesystem in the other half of it...
// Persistent configuration info is stored at 0x80000
#define FS_FLASH_OFFSET 0x90000 // 576kb
#define FS_FLASH_LENGTH 0x70000 // 448kb

static uint8_t lfs_read_buffer[1024];
static uint8_t lfs_prog_buffer[1024];
static uint8_t lfs_lookahead_buffer[8] __attribute__((aligned(32)));

int hx_flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size) {
	int rv = 0;
	gpio_set_level(GPIO_TYPEC_G_DRV2_EN, 0);
	gpio_set_alternate_function(1, 0x4000, 2);
	rv = spi_flash_read((uint8_t*)buffer, (FS_FLASH_OFFSET + (block * c->block_size) + off), size);
	gpio_set_alternate_function(1, 0x4000, 1);
	gpio_set_level(GPIO_TYPEC_G_DRV2_EN, 1);
	return rv;
}

int hx_flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size) {
	int rv = 0;
	gpio_set_level(GPIO_TYPEC_G_DRV2_EN, 0);
	gpio_set_alternate_function(1, 0x4000, 2);
	rv = spi_flash_write(FS_FLASH_OFFSET + (block * c->block_size) + off, size, (const uint8_t*)buffer);
	spi_flash_wait();
	gpio_set_alternate_function(1, 0x4000, 1);
	gpio_set_level(GPIO_TYPEC_G_DRV2_EN, 1);
	return rv;
}

int hx_flash_erase(const struct lfs_config *c, lfs_block_t block) {
	int rv = 0;
	gpio_set_level(GPIO_TYPEC_G_DRV2_EN, 0);
	gpio_set_alternate_function(1, 0x4000, 2);
	rv = spi_flash_erase(FS_FLASH_OFFSET + (block * c->block_size), c->block_size);
	spi_flash_wait();
	gpio_set_alternate_function(1, 0x4000, 1);
	gpio_set_level(GPIO_TYPEC_G_DRV2_EN, 1);
	return rv;
}

int hx_flash_sync(const struct lfs_config *c) {
	// All writes are committed directly to flash; there is nothing to sync.
	return 0;
}

const struct lfs_config cfg = {
	.read = hx_flash_read,
	.prog = hx_flash_prog,
	.erase = hx_flash_erase,
	.sync = hx_flash_sync,

	.read_size = CONFIG_FLASH_WRITE_IDEAL_SIZE,
	.prog_size = CONFIG_FLASH_WRITE_IDEAL_SIZE,
	.block_size = CONFIG_FLASH_ERASE_SIZE,
	.block_count = FS_FLASH_LENGTH / CONFIG_FLASH_ERASE_SIZE,
	.block_cycles = -1,

	.cache_size = 1024, // block_size (4096) / 4; read/prog_size * 4
	.lookahead_size = 8,

	.read_buffer = lfs_read_buffer,
	.prog_buffer = lfs_prog_buffer,
	.lookahead_buffer = lfs_lookahead_buffer,
};

lfs_t lfs_fs;
static uint8_t mounted;

static int fs_mount(void) {
	int rv = lfs_mount(&lfs_fs, &cfg);
	if (rv < 0) {
		CPRINTF("lfs failed to mount: %d\n", rv);
		return rv;
	}
	CPUTS("lfs mounted\n");
	mounted = 1;
	return rv;
}

static int fs_format(void) {
	int rv = 0;
	if (mounted) {
		return -EC_ERROR_INVAL;
	}
#if 0
	rv = lfs_format(&lfs_fs, &cfg)
	if (rv < 0) {
		CPRINTF("lfs failed to format: %d\n", rv);
		return rv;
	}
	CPUTS("lfs formatted\n");
#endif
	return rv;
}

static int fs_unmount(void) {
	lfs_unmount(&lfs_fs);
	mounted = 0;
	return 0;
}

static void filesystem_startup(void) {
	int rv = fs_mount();
	if (rv < 0) {
		rv = fs_format();
		rv = fs_mount();
		if (rv < 0) {
			CPRINTF("Formatting new LFS FS failed: %d\n", rv);
		}
	}
}
DECLARE_HOOK(HOOK_CHIPSET_STARTUP, filesystem_startup, HOOK_PRIO_DEFAULT);

static int block_print_cb(void* context, lfs_block_t block) {
	CPRINTF("%d \n", block);
	return 0;
}

static int cc_lfs(int argc, char** argv) {
	if(argc >= 2) {
		if (!strncmp(argv[1], "ls", 2)) {
			char* path;
			lfs_dir_t dir;
			struct lfs_info info;
			int rv = 0;
			path = (argc>=3 && argv[2][0]!='\0' ? argv[2] : "/");
			rv = lfs_dir_open(&lfs_fs, &dir, path);
			if (rv < 0) {
				CPRINTF("LFS: failed to open %s: %d\n", path, rv);
			} else {
				while(true) {
					rv = lfs_dir_read(&lfs_fs, &dir, &info);
					if (rv < 0) {
						lfs_dir_close(&lfs_fs, &dir);
						return rv;
					}
					if (!rv) break;
					CPRINTF("%d %s%s\n", info.type == LFS_TYPE_REG ? info.size : 0, info.name, info.type == LFS_TYPE_DIR ? "/" : "");
				}
				rv = lfs_dir_close(&lfs_fs, &dir);
			}
			return rv;
		} else if (!strncmp(argv[1], "mount", 5)) {
			return fs_mount();
		} else if (!strncmp(argv[1], "unmount", 7)) {
			return fs_unmount();
		} else if (!strncmp(argv[1], "rm", 2)) {
#if 0
			if (argc >= 3) {
				int rv = lfs_remove(&lfs_fs, argv[2]);
				if (rv < 0) {
					CPRINTF("LFS: failed to remove %s: %d\n", argv[2], rv);
				}
				return rv;
			}
			CPRINTF("Syntax: lfs rm <path>\n");
#endif
			return -1;
		} else if (!strncmp(argv[1], "format", 6)) {
#if 0
			return fs_format();
#endif
		//} else if (!strncmp(argv[1], "", 000)) {
		}
	} else {
		CPUTS("LFS Blocks: ");
		lfs_fs_traverse(&lfs_fs, &block_print_cb, NULL);
		CPUTS("\n");
	}
	return 0;
}
DECLARE_CONSOLE_COMMAND(lfs, cc_lfs, "[ls|rm|mount|unmount|format] [path]", "lfs stuff");
