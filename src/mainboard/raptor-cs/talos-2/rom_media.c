/* SPDX-License-Identifier: GPL-2.0-only */

#include <string.h>
#include <boot_device.h>
#include <arch/io.h>
#include <console/console.h>
#include <endian.h>
#include "../../../../3rdparty/ffs/ffs/ffs.h"

/* Base is not NULL on real hardware, it will be patched later */
static struct mem_region_device boot_dev =
	MEM_REGION_DEV_RO_INIT(NULL, CONFIG_ROM_SIZE);

#define LPC_FLASH_MIN (MMIO_GROUP0_CHIP0_LPC_BASE_ADDR + LPCHC_FW_SPACE)
#define LPC_FLASH_TOP (LPC_FLASH_MIN + FW_SPACE_SIZE)

/* TODO: at some point we will have to change it to a bigger partition (HBI?) */
#define CBFS_PARTITION_NAME "HBB"

/* ffs_entry is not complete in included ffs.h, it lacks user data layout.
 * See https://github.com/open-power/skiboot/blob/master/libflash/ffs.h */

/* Data integrity flags */
#define FFS_ENRY_INTEG_ECC 0x8000

enum ecc_status {
	CLEAN=0,          //< No ECC Error was detected.
	CORRECTED=1,      //< ECC error detected and corrected.
	UNCORRECTABLE=2   //< ECC error detected and uncorrectable.
};
typedef enum  ecc_status ecc_status_t;

enum ecc_bitfields {
	GD = 0xff,      //< Good, ECC matches.
	UE = 0xfe,      //< Uncorrectable.
	E0 = 71,        //< Error in ECC bit 0
	E1 = 70,        //< Error in ECC bit 1
	E2 = 69,        //< Error in ECC bit 2
	E3 = 68,        //< Error in ECC bit 3
	E4 = 67,        //< Error in ECC bit 4
	E5 = 66,        //< Error in ECC bit 5
	E6 = 65,        //< Error in ECC bit 6
	E7 = 64         //< Error in ECC bit 7
};

static uint64_t ecc_matrix[] = {
	//0000000000000000111010000100001000111100000011111001100111111111
	0x0000e8423c0f99ff,
	//0000000011101000010000100011110000001111100110011111111100000000
	0x00e8423c0f99ff00,
	//1110100001000010001111000000111110011001111111110000000000000000
	0xe8423c0f99ff0000,
	//0100001000111100000011111001100111111111000000000000000011101000
	0x423c0f99ff0000e8,
	//0011110000001111100110011111111100000000000000001110100001000010
	0x3c0f99ff0000e842,
	//0000111110011001111111110000000000000000111010000100001000111100
	0x0f99ff0000e8423c,
	//1001100111111111000000000000000011101000010000100011110000001111
	0x99ff0000e8423c0f,
	//1111111100000000000000001110100001000010001111000000111110011001
	0xff0000e8423c0f99
};

static uint8_t syndrome_matrix[] = {
	GD, E7, E6, UE, E5, UE, UE, 47, E4, UE, UE, 37, UE, 35, 39, UE,
	E3, UE, UE, 48, UE, 30, 29, UE, UE, 57, 27, UE, 31, UE, UE, UE,
	E2, UE, UE, 17, UE, 18, 40, UE, UE, 58, 22, UE, 21, UE, UE, UE,
	UE, 16, 49, UE, 19, UE, UE, UE, 23, UE, UE, UE, UE, 20, UE, UE,
	E1, UE, UE, 51, UE, 46,  9, UE, UE, 34, 10, UE, 32, UE, UE, 36,
	UE, 62, 50, UE, 14, UE, UE, UE, 13, UE, UE, UE, UE, UE, UE, UE,
	UE, 61,  8, UE, 41, UE, UE, UE, 11, UE, UE, UE, UE, UE, UE, UE,
	15, UE, UE, UE, UE, UE, UE, UE, UE, UE, 12, UE, UE, UE, UE, UE,
	E0, UE, UE, 55, UE, 45, 43, UE, UE, 56, 38, UE,  1, UE, UE, UE,
	UE, 25, 26, UE,  2, UE, UE, UE, 24, UE, UE, UE, UE, UE, 28, UE,
	UE, 59, 54, UE, 42, UE, UE, 44,  6, UE, UE, UE, UE, UE, UE, UE,
	5, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE,
	UE, 63, 53, UE,  0, UE, UE, UE, 33, UE, UE, UE, UE, UE, UE, UE,
	3, UE, UE, 52, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE,
	7, UE, UE, UE, UE, UE, UE, UE, UE, 60, UE, UE, UE, UE, UE, UE,
	UE, UE, UE, UE,  4, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE, UE,
};

static inline uint32_t rd32(void *addr)
{
	uint64_t ret;

	/* Cache-inhibited load word */
	asm volatile("lwzcix %0, 0, %1"
				 : "=r" (ret)
				 : "r" (addr)
				 : );

	return ret;
}

static uint64_t rd64_unaligned(void *addr, int first_read)
{
	static uint64_t tmp1;	/* static is used to reduce number of PNOR reads */
	uint64_t tmp2;
	uint64_t ret;
	uint64_t addr_aligned = ALIGN_DOWN((uint64_t)addr, 8);
	unsigned shift = 8 * ((uint64_t) addr - addr_aligned);

	if (shift == 0			/* Previous tmp2 ended with ECC byte */
	    || first_read) {	/* or it is the first invocation from remove_ecc */
		asm volatile("ldcix %0, 0, %1"
					 : "=r" (tmp1)
					 : "r" (addr_aligned)
					 : );
	}

	asm volatile("ldcix %0, 0, %1"
				 : "=r" (tmp2)
				 : "r" (addr_aligned+8)
				 : );

	ret = (tmp1 << shift) | (tmp2 >> (64 - shift));
	tmp1 = tmp2;

	return ret;
}

/*
 * memcpy from cache-inhibited source
 *
 * Assume both dest and src are 8B-aligned and do not overlap. Copies ALIGN(n,8)
 * bytes, make sure dest is big enough.
 */
static inline void memcpy_ci_src(void *dest, const void *src, size_t n)
{
	int i;
	uint64_t tmp;
	for (i = 0; i < n; i+=8) {
		asm volatile("ldcix %0, %1, %2"
					 : "=r" (tmp)
					 : "b"(src), "r" (i));
		asm volatile("stdx %0, %1, %2"
					 :: "r" (tmp), "b"(dest), "r" (i)
					 : "memory");
	}
}

static uint8_t generate_ecc(uint64_t i_data)
{
	uint8_t result = 0;

	for (int i = 0; i < 8; i++) {
		result |= __builtin_parityll(ecc_matrix[i] & i_data) << i;
	}
	return result;
}

static uint8_t verify_ecc(uint64_t i_data, uint8_t i_ecc)
{
	return syndrome_matrix[generate_ecc(i_data) ^ i_ecc ];
}

static uint8_t correct_ecc(uint64_t *io_data, uint8_t *io_ecc)
{
	uint8_t bad_bit = verify_ecc(*io_data, *io_ecc);

	if ((bad_bit != GD) && (bad_bit != UE)) { /* Good is done, UE is hopeless */
		/* Determine if the ECC or data part is bad, do bit flip. */
		if (bad_bit >= E7) {
			*io_ecc ^= (1 << (bad_bit - E7));
		} else {
			*io_data ^=(1ull << (63 - bad_bit));
		}
	}
	return bad_bit;
}

static ecc_status_t remove_ecc(uint8_t* io_src, size_t i_srcSz,
                               uint8_t* o_dst, size_t i_dstSz)
{
	ecc_status_t rc = CLEAN;
	int first_read = 1;

	for(size_t i = 0, o = 0; i < i_srcSz;
	    i += sizeof(uint64_t) + sizeof(uint8_t), o += sizeof(uint64_t)) {
		/*
		 * Read data and ECC parts. Reads from cache-inhibited storage always
		 * have to be aligned!
		 */
		uint64_t data = rd64_unaligned(&io_src[i], first_read);
		first_read = 0;

		uint8_t ecc = io_src[i + sizeof(uint64_t)];

		/* Calculate failing bit and fix data */
		uint8_t bad_bit = correct_ecc(&data, &ecc);

		/* Perform correction and status update */
		if (bad_bit == UE)
		{
			rc = UNCORRECTABLE;
		}
		/* Unused, our source is not writable */
		/*
		else if (bad_bit != GD)
		{
			if (rc != UNCORRECTABLE)
			{
				rc = CORRECTED;
			}

			*(uint64_t*)(&io_src[i]) = data;
			io_src[i + sizeof(uint64_t)] = ecc;
		}
		*/

		/* Copy fixed data to destination buffer */
		*(uint64_t*)(&o_dst[o]) = data;
	}
	return rc;
}

/*
 * This buffer makes it possible to use small mmap-ed structures without the
 * need for readat(). It has some drawbacks: limited size, only one mmaped
 * region possible at the time (using this rdev) and possibly more reads from
 * flash device than necessary.
 *
 * Such approach works for small structures like cbfs_file or its filename.
 * It does not work for LZMA decompression, as it tries to mmap whole compressed
 * file. LZ4 works nice, it uses in-place decompression after compressed image
 * is read with rdev_readat().
 */
static uint8_t buf[0x100];
static int buf_in_use;

static ssize_t ecc_readat(const struct region_device *rd, void *b,
				size_t offset, size_t size)
{
	const struct mem_region_device *mdev;
	uint8_t tmp[8];
	size_t off_a = ALIGN_DOWN(offset, 8);
	size_t size_left = size;

	mdev = container_of(rd, __typeof__(*mdev), rdev);

	/* If offset is not 8B-aligned */
	if (offset & 0x7) {
		int i;
		remove_ecc((uint8_t *) &mdev->base[(off_a * 9)/8], 9, tmp, 8);
		for (i = 8 - (offset & 7); i < 8; i++) {
			*((uint8_t *)(b++)) = tmp[i];
			if (!--size_left)
				return size;
		}
		off_a += 8;
	}

	/* Align down size_left to 8B */
	remove_ecc((uint8_t *) &mdev->base[(off_a * 9)/8],
	           (ALIGN_DOWN(size_left, 8) * 9) / 8,
	           b,
	           ALIGN_DOWN(size_left, 8));

	/* Copy the rest of requested unaligned data, if any */
	if (size_left & 7) {
		off_a += ALIGN_DOWN(size_left, 8);
		b += ALIGN_DOWN(size_left, 8);
		int i;
		remove_ecc((uint8_t *) &mdev->base[(off_a * 9)/8], 9, tmp, 8);
		for (i = 0; i < (size_left & 7); i++) {
			*((uint8_t *)(b++)) = tmp[i];
		}
	}

	return size;
}

static void *ecc_mmap(const struct region_device *rd, size_t offset, size_t size)
{
	if (buf_in_use) {
		printk(BIOS_ERR, "ecc_mmap: cannot mmap multiple ranges at once\n");
		return NULL;
	}
	if (size > sizeof(buf)) {
		printk(BIOS_ERR, "ecc_mmap: size too big (%lx)\n", size);
		return NULL;
	}
	buf_in_use = 1;
	ecc_readat(rd, buf, offset, size);
	return buf;
}

static int ecc_munmap(const struct region_device *rd, void *mapping)
{
	buf_in_use = 0;
	return 0;
}

struct region_device_ops ecc_rdev_ops = {
	.mmap = ecc_mmap,
	.munmap = ecc_munmap,
	.readat = ecc_readat,
};

static void find_cbfs_in_pnor(void)
{
	char *cbfs = NULL;
	char *pnor_base = NULL;
	unsigned int i, block_size, entry_count = 0;
	struct ffs_hdr *hdr_pnor = (struct ffs_hdr *)LPC_FLASH_TOP;

	/* This loop could be skipped if we may assume that PNOR is always 64M */
	while (hdr_pnor > (struct ffs_hdr *)LPC_FLASH_MIN) {
		uint32_t csum = 0;
		/* Size is aligned up to 8 because of how memcpy_ci_src works */
		uint8_t buffer[ALIGN(FFS_HDR_SIZE, 8)];
		struct ffs_hdr *hdr = (struct ffs_hdr *)buffer;

		/* Assume block_size = 4K */
		hdr_pnor = (struct ffs_hdr *)(((char *)hdr_pnor) - 0x1000);

		if (FFS_MAGIC != rd32(&hdr_pnor->magic))
			continue;

		if (FFS_VERSION_1 != rd32(&hdr_pnor->version))
			continue;

		/* Copy the header so we won't have to rd32() for further accesses */
		memcpy_ci_src(buffer, hdr_pnor, FFS_HDR_SIZE);
		csum = hdr->magic ^ hdr->version ^ hdr->size ^ hdr->entry_size ^
		       hdr->entry_count ^ hdr->block_size ^ hdr->block_count ^
		       hdr->resvd[0] ^ hdr->resvd[1] ^ hdr->resvd[2] ^ hdr->resvd[3] ^
		       hdr->checksum;
		if (csum != 0) continue;

		pnor_base = (char *) LPC_FLASH_TOP - hdr->block_size * hdr->block_count;
		entry_count = hdr->entry_count;
		block_size = hdr->block_size;

		/* TODO: add a flag to reduce verbosity when building for SEEPROM,
		 * every byte counts */
		printk(BIOS_DEBUG, "Found FFS header at %p\n", hdr_pnor);
		printk(BIOS_SPEW, "    size %x\n", hdr->size);
		printk(BIOS_SPEW, "    entry_size %x\n", hdr->entry_size);
		printk(BIOS_SPEW, "    entry_count %x\n", hdr->entry_count);
		printk(BIOS_SPEW, "    block_size %x\n", hdr->block_size);
		printk(BIOS_SPEW, "    block_count %x\n", hdr->block_count);
		printk(BIOS_DEBUG, "PNOR base at %p\n", pnor_base);
		break;
	}

	if (hdr_pnor <= (struct ffs_hdr *)LPC_FLASH_MIN)
		die("FFS header not found!\n");

	for (i = 0; i < entry_count; i++) {
		uint32_t *val, csum = 0;
		int j;
		/* Size is aligned up to 8 because of how memcpy_ci_src works */
		uint8_t buffer[ALIGN(FFS_ENTRY_SIZE, 8)];
		struct ffs_entry *e = (struct ffs_entry *)buffer;

		/* Copy the entry so we won't have to rd32() for further accesses */
		memcpy_ci_src(buffer, &hdr_pnor->entries[i], FFS_ENTRY_SIZE);

		printk(BIOS_SPEW, "%s: base %x, size %x  (%x)\n\t type %x, flags %x\n",
		       e->name, e->base, e->size, e->actual, e->type, e->flags);

		if (strcmp(e->name, CBFS_PARTITION_NAME) != 0)
			continue;

		val = (uint32_t *) e;
		for (j = 0; j < (FFS_ENTRY_SIZE / sizeof(uint32_t)); j++)
			csum ^= val[j];

		if (csum != 0)
			continue;

		cbfs = (char *) pnor_base + block_size * e->base;

		/* Skip PNOR partition header size */
		cbfs += 0x1000;
		if (e->user.data[0] & FFS_ENRY_INTEG_ECC) {
			printk(BIOS_DEBUG, "CBFS partition has ECC\n");
			boot_dev.rdev.ops = &ecc_rdev_ops;
			cbfs += 0x200;
		}

		printk(BIOS_DEBUG, "Found CBFS partition at %p\n", cbfs);
		boot_dev.base = cbfs;
		break;
	}
}

const struct region_device *boot_device_ro(void)
{
	if (boot_dev.base == NULL)
		find_cbfs_in_pnor();

	return &boot_dev.rdev;
}
