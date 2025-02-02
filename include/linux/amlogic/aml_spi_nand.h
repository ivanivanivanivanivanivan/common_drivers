/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_SPI_NAND_H_
#define __AML_SPI_NAND_H_

/* Max total is 1024 as romboot says so... */
#define SPI_NAND_BOOT_TOTAL_PAGES	(1024)
/* This depends on uboot size */
#define SPI_NAND_BOOT_PAGES_PER_COPY (1024)
#define SPI_NAND_BOOT_COPY_NUM (SPI_NAND_BOOT_TOTAL_PAGES / SPI_NAND_BOOT_PAGES_PER_COPY)

#define SPI_NAND_BL2_SIZE			(64 * 1024)
#define SPI_NAND_BL2_OCCUPY_PER_PAGE		2048
#define SPI_NAND_BL2_PAGES			(SPI_NAND_BL2_SIZE / SPI_NAND_BL2_OCCUPY_PER_PAGE)
#define SPI_NAND_BL2_COPY_NUM		8
#define SPI_NAND_TPL_SIZE_PER_COPY	0x200000
#define SPI_NAND_TPL_COPY_NUM		4
#define SPI_NAND_NBITS		2

extern const struct spinand_manufacturer dosilicon_spinand_manufacturer;

enum info_page_mode {
	NORMAL_INFO_P = 0,
	FRONT_INFO_P = 1,
	NO_INFO_P = 2,
};

struct spinand_info_page {
	char magic[8];	/* magic header of info page */
	/* info page version, +1 when you update this struct */
	u8 version;	/* 1 for now */
	u8 mode;	/* 1 discrete, 0 compact */
	u8 bl2_num;	/* bl2 copy number */
	u8 fip_num;	/* fip copy number */
	union {
		struct {
#define SPINAND_MAGIC       "AMLIFPG"
#define SPINAND_INFO_VER    1
			u8 rd_max; /* spi nand max read io */
			u8 oob_offset; /* user bytes offset */
			u8 reserved[2];
			u32 fip_start; /* start pages */
			u32 fip_pages; /* pages per fip */
			u32 page_size; /* spi nand page size (bytes) */
			u32 page_per_blk;	/* page number per block */
			u32 oob_size;	/* valid oob size (bytes) */
			u32 bbt_start; /* bbt start pages */
			u32 bbt_valid; /* bbt valid offset pages */
			u32 bbt_size;	/* bbt occupied bytes */
		} s;/* spi nand */
		struct {
			u32 reserved;
		} e;/* emmc */
	} dev;

};

struct spinand_front_info_page {
#define SPINAND_MAGIC_V2      "BOOTINFO"
#define SPINAND_MAGIC_V2_LEN  8
#define SPINAND_INFO_VER_2    2
	char magic[8];
	unsigned char version;		/* need to greater than or equal to 2 */
	unsigned char reserved[2];	/* reserve zero */
	/* bit0~1: page per bbt */
	unsigned char common;
	struct {
		unsigned int page_size;
		/* bit0~3: planes_per_lun bit4~7: plane_shift */
		unsigned char planes_per_lun;
		/* bit0~3: bus_width bit4~7: cache_plane_shift */
		unsigned char bus_width;
	} dev_cfg;
	unsigned int checksum;
};

int meson_spinand_init(struct spinand_device *spinand, struct mtd_info *mtd);
int meson_add_mtd_partitions(struct mtd_info *mtd);
void spinand_get_tpl_info(u32 *fip_size, u32 *fip_copies);
bool meson_spinand_isbad(struct nand_device *nand, const struct nand_pos *pos);
int spinand_mtd_write_unlock(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops);
int spinand_mtd_read_unlock(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops);
/* spinand add info page support */
bool spinand_is_info_page(struct nand_device *nand, int page);
int spinand_set_info_page(struct mtd_info *mtd, void *buf);
/* spinand add front info page support */
bool spinand_is_front_info_page(struct nand_device *nand, int page);
int spinand_set_front_info_page(struct mtd_info *mtd, void *buf);
u32 spinand_get_info_page_mode(void);
#endif
