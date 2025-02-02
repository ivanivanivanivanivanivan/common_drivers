// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/major.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
/* Local include */
#include "hdmi_rx_repeater.h"
#include "hdmi_rx_drv.h"
#include "hdmi_rx_edid.h"
#include "hdmi_rx_hw.h"

enum edid_delivery_mothed_e edid_delivery_mothed;
/* buff to store downstream EDID or EDID load from bin */
static char edid_buf1[EDID_BUF_SIZE] = {0};
static char edid_buf2[EDID_BUF_SIZE] = {0};
static char edid_buf3[EDID_BUF_SIZE] = {0};
static char edid_buf4[EDID_BUF_SIZE] = {0};
char edid_cur[EDID_SIZE] = {0};
#ifdef CONFIG_AMLOGIC_HDMITX
//0: hdmi repeater
//1: use tx edid directly
//2: use default edid.

u32 tx_hdr_priority;
unsigned char tx_vic_list[31] = { 0 };
unsigned char def_vic_list[31] = { 0 };
static unsigned char edid_tx[EDID_SIZE];
#endif

u8 need_support_atmos_bit = 0xff;
static unsigned int earc_cap_ds_len;
static unsigned char recv_earc_cap_ds[EARC_CAP_DS_MAX_LENGTH] = {0};
bool new_earc_cap_ds;
static u8 com_aud[DB_LEN_MAX - 1] = {0};
/* use vsvdb of edid bin by default, but
 * after DV enable/disable setting, use
 * vsvdb from DV module or remove vsvdb
 */
static unsigned char recv_vsvdb_len = 0xFF;
static unsigned char recv_vsvdb[VSVDB_LEN] = {0};
u32 vsvdb_update_hpd_en = 1;

int edid_mode;
u8 port_hpd_rst_flag;
int port_map;

MODULE_PARM_DESC(port_map, "\n port_map\n");
module_param(port_map, int, 0664);

/*
 * 1:reset hpd after atmos edid update
 * 0:not reset hpd after atmos edid update
 */
u32 atmos_edid_update_hpd_en = 1;
static u_char tmp_sad_len;
static u_char tmp_sad[30];

/* if free space is not enough in edid to do
 * data blk splice, then remove the last dtd(s)
 */
u32 en_take_dtd_space = 1;
/*
 * 1:reset hpd after cap ds edid update
 * 0:not reset hpd after cap ds edid update
 */
u32 earc_cap_ds_update_hpd_en = 1;

unsigned int edid_select;
unsigned int edid_reset_max = 500;

/* hdmi1.4 edid */
static unsigned char edid_dbg[] = {
0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
0x05, 0xAC, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00,
0x20, 0x19, 0x01, 0x03, 0x80, 0x73, 0x41, 0x78,
0x0A, 0xCF, 0x74, 0xA3, 0x57, 0x4C, 0xB0, 0x23,
0x09, 0x48, 0x4C, 0x21, 0x08, 0x00, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x74,
0x00, 0x30, 0xF2, 0x70, 0x5A, 0x80, 0xB0, 0x58,
0x8A, 0x00, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E,
0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40,
0x58, 0x2C, 0x45, 0x00, 0x20, 0xC2, 0x31, 0x00,
0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x41,
0x4D, 0x4C, 0x20, 0x54, 0x56, 0x0A, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD,
0x00, 0x3B, 0x46, 0x1F, 0x8C, 0x3C, 0x00, 0x0A,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xDA,
0x02, 0x03, 0x58, 0xF0, 0x4E, 0x60, 0x61, 0x5F,
0x5E, 0x5D, 0x10, 0x1F, 0x22, 0x21, 0x20, 0x04,
0x13, 0x03, 0x12, 0x2C, 0x0F, 0x7F, 0x05, 0x15,
0x07, 0x50, 0x57, 0x06, 0x01, 0x67, 0x54, 0x03,
0x83, 0x01, 0x00, 0x00, 0x6D, 0x03, 0x0C, 0x00,
0x10, 0x00, 0xB8, 0x3C, 0x2F, 0x80, 0x60, 0x01,
0x02, 0x03, 0x68, 0xD8, 0x5D, 0xC4, 0x01, 0x78,
0x80, 0x03, 0x02, 0xE3, 0x05, 0x40, 0x01, 0xE2,
0x0F, 0x03, 0xE3, 0x06, 0x0D, 0x01, 0xEB, 0x01,
0x46, 0xD0, 0x00, 0x44, 0x54, 0x23, 0x88, 0x52,
0x6B, 0x94, 0xE5, 0x01, 0x8B, 0x84, 0x90, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDD,
};

/* correspond to enum hdmi_vic_e */
const char *hdmi_fmt[] = {
	"HDMI_UNKNOWN",
	"HDMI_640x480p60_4x3",
	"HDMI_720x480p60_4x3",
	"HDMI_720x480p60_16x9",
	"HDMI_1280x720p60_16x9",
	"HDMI_1920x1080i60_16x9",
	"HDMI_720x480i60_4x3",
	"HDMI_720x480i60_16x9",
	"HDMI_720x240p60_4x3",
	"HDMI_720x240p60_16x9",
	"HDMI_2880x480i60_4x3",
	"HDMI_2880x480i60_16x9",
	"HDMI_2880x240p60_4x3",
	"HDMI_2880x240p60_16x9",
	"HDMI_1440x480p60_4x3",
	"HDMI_1440x480p60_16x9",
	"HDMI_1920x1080p60_16x9",
	"HDMI_720x576p50_4x3",
	"HDMI_720x576p50_16x9",
	"HDMI_1280x720p50_16x9",
	"HDMI_1920x1080i50_16x9",
	"HDMI_720x576i50_4x3",
	"HDMI_720x576i50_16x9",
	"HDMI_720x288p_4x3",
	"HDMI_720x288p_16x9",
	"HDMI_2880x576i50_4x3",
	"HDMI_2880x576i50_16x9",
	"HDMI_2880x288p50_4x3",
	"HDMI_2880x288p50_16x9",
	"HDMI_1440x576p_4x3",
	"HDMI_1440x576p_16x9",
	"HDMI_1920x1080p50_16x9",
	"HDMI_1920x1080p24_16x9",
	"HDMI_1920x1080p25_16x9",
	"HDMI_1920x1080p30_16x9",
	"HDMI_2880x480p60_4x3",
	"HDMI_2880x480p60_16x9",
	"HDMI_2880x576p50_4x3",
	"HDMI_2880x576p50_16x9",
	"HDMI_1920x1080i_t1250_50_16x9",
	"HDMI_1920x1080i100_16x9",
	"HDMI_1280x720p100_16x9",
	"HDMI_720x576p100_4x3",
	"HDMI_720x576p100_16x9",
	"HDMI_720x576i100_4x3",
	"HDMI_720x576i100_16x9",
	"HDMI_1920x1080i120_16x9",
	"HDMI_1280x720p120_16x9",
	"HDMI_720x480p120_4x3",
	"HDMI_720x480p120_16x9",
	"HDMI_720x480i120_4x3",
	"HDMI_720x480i120_16x9",
	"HDMI_720x576p200_4x3",
	"HDMI_720x576p200_16x9",
	"HDMI_720x576i200_4x3",
	"HDMI_720x576i200_16x9",
	"HDMI_720x480p240_4x3",
	"HDMI_720x480p240_16x9",
	"HDMI_720x480i240_4x3",
	"HDMI_720x480i240_16x9",
	/* Refer to CEA 861-F */
	"HDMI_1280x720p24_16x9",
	"HDMI_1280x720p25_16x9",
	"HDMI_1280x720p30_16x9",
	"HDMI_1920x1080p120_16x9",
	"HDMI_1920x1080p100_16x9",
	"HDMI_1280x720p24_64x27",
	"HDMI_1280x720p25_64x27",
	"HDMI_1280x720p30_64x27",
	"HDMI_1280x720p50_64x27",
	"HDMI_1280x720p60_64x27",
	"HDMI_1280x720p100_64x27",
	"HDMI_1280x720p120_64x27",
	"HDMI_1920x1080p24_64x27",
	"HDMI_1920x1080p25_64x27",
	"HDMI_1920x1080p30_64x27",
	"HDMI_1920x1080p50_64x27",
	"HDMI_1920x1080p60_64x27",
	"HDMI_1920x1080p100_64x27",
	"HDMI_1920x1080p120_64x27",
	"HDMI_1680x720p24_64x27",
	"HDMI_1680x720p25_64x27",
	"HDMI_1680x720p30_64x27",
	"HDMI_1680x720p50_64x27",
	"HDMI_1680x720p60_64x27",
	"HDMI_1680x720p100_64x27",
	"HDMI_1680x720p120_64x27",
	"HDMI_2560x1080p24_64x27",
	"HDMI_2560x1080p25_64x27",
	"HDMI_2560x1080p30_64x27",
	"HDMI_2560x1080p50_64x27",
	"HDMI_2560x1080p60_64x27",
	"HDMI_2560x1080p100_64x27",
	"HDMI_2560x1080p120_64x27",
	"HDMI_3840x2160p24_16x9",
	"HDMI_3840x2160p25_16x9",
	"HDMI_3840x2160p30_16x9",
	"HDMI_3840x2160p50_16x9",
	"HDMI_3840x2160p60_16x9",
	"HDMI_4096x2160p24_256x135",
	"HDMI_4096x2160p25_256x135",
	"HDMI_4096x2160p30_256x135",
	"HDMI_4096x2160p50_256x135",
	"HDMI_4096x2160p60_256x135",
	"HDMI_3840x2160p24_64x27",
	"HDMI_3840x2160p25_64x27",
	"HDMI_3840x2160p30_64x27",
	"HDMI_3840x2160p50_64x27",
	"HDMI_3840x2160p60_64x27",
	"HDMI_RESERVED",
};

const char *_3d_structure[] = {
	/* hdmi1.4 spec, Table H-7 */
	/* value: 0x0000 */
	"Frame packing",
	"Field alternative",
	"Line alternative",
	"Side-by-Side(Full)",
	"L + depth",
	"L + depth + graphics + graphics-depth",
	"Top-and-Bottom",
	/* value 0x0111: Reserved for future use */
	"Resrvd",
	/* value 0x1000 */
	"Side-by-Side(Half) with horizontal sub-sampling",
	/* value 0x1001-0x1110:Reserved for future use */
	"Resrvd",
	"Resrvd",
	"Resrvd",
	"Resrvd",
	"Resrvd",
	"Resrvd",
	"Side-by-Side(Half) with all quincunx sub-sampling",
};

const char *_3d_detail_x[] = {
	/* hdmi1.4 spec, Table H-8 */
	/* value: 0x0000 */
	"All horizontal sub-sampling and quincunx matrix",
	"Horizontal sub-sampling",
	"Not_in_Use1",
	"Not_in_Use2",
	"Not_in_Use3",
	"Not_in_Use4",
	"All_Quincunx",
	"ODD_left_ODD_right",
	"ODD_left_EVEN_right",
	"EVEN_left_ODD_right",
	"EVEN_left_EVEN_right",
	"Resrvd1",
	"Resrvd2",
	"Resrvd3",
	"Resrvd4",
	"Resrvd5",
};

const char *aud_fmt[] = {
	"HEADER",
	"L-PCM",
	"AC3",
	"MPEG1",
	"MP3",
	"MPEG2",
	"AAC",
	"DTS",
	"ATRAC",
	"OBA",
	"DDP", /* E_AC3 */
	"DTS_HD",
	"MAT", /* TRUE_HD*/
	"DST",
	"WMAPRO",
};

unsigned char *edid_list[] = {
	edid_buf1,
	edid_buf2,
	edid_buf3,
	edid_buf4,
	edid_dbg,
};

static struct cta_blk_pair cta_blk[] = {
	{
		.tag_code = AUDIO_TAG,
		.blk_name = "Audio_DB",
	},
	{
		.tag_code = VIDEO_TAG,
		.blk_name = "Video_DB",
	},
	{
		.tag_code = VENDOR_TAG,
		.blk_name = "Vendor_Specific_DB",
	},
	{
		.tag_code = HF_VENDOR_DB_TAG,
		.blk_name = "HF_Vendor_Specific_DB",
	},
	{
		.tag_code = SPEAKER_TAG,
		.blk_name = "Speaker_Alloc_DB",
	},
	{
		.tag_code = VDTCDB_TAG,
		.blk_name = "VESA_DTC_DB",
	},
	{
		.tag_code = EXTENDED_VCDB_TAG,
		.blk_name = "Video_Cap_DB",
	},
	{
		.tag_code = VSVDB_DV_TAG,
		.blk_name = "VSVDB_DV",
	},
	{
		.tag_code = VSVDB_HDR10P_TAG,
		.blk_name = "VSVDB_HDR10P",
	},
	{
		.tag_code = EXTENDED_VDDDB_TAG,
		.blk_name = "VESA_Display_Device_DB",
	},
	{
		.tag_code = EXTENDED_VVTBE_TAG,
		.blk_name = "VESA_Video_Timing_Blk_Ext",
	},
	{
		.tag_code = EXTENDED_CDB_TAG,
		.blk_name = "Colorimetry_DB",
	},
	{
		.tag_code = EXTENDED_HDR_STATIC_TAG,
		.blk_name = "HDR_Static_Metadata",
	},
	{
		.tag_code = EXTENDED_HDR_DYNAMIC_TAG,
		.blk_name = "HDR_Dynamic_Metadata",
	},
	{
		.tag_code = EXTENDED_VFPDB_TAG,
		.blk_name = "Video_Fmt_Preference_DB",
	},
	{
		.tag_code = EXTENDED_Y420VDB_TAG,
		.blk_name = "YCbCr_420_Video_DB",
	},
	{
		.tag_code = EXTENDED_Y420CMDB_TAG,
		.blk_name = "YCbCr_420_Cap_Map_DB",
	},
	{
		.tag_code = EXTENDED_VSADB_TAG,
		.blk_name = "Vendor_Specific_Audio_DB",
	},
	{
		.tag_code = EXTENDED_RCDB_TAG,
		.blk_name = "Room_Configuration_DB",
	},
	{
		.tag_code = EXTENDED_SLDB_TAG,
		.blk_name = "Speaker_Location_DB",
	},
	{
		.tag_code = EXTENDED_IFDB_TAG,
		.blk_name = "Infoframe_DB",
	},
	{
		.tag_code = TAG_MAX,
		.blk_name = "Unrecognized_DB",
	},
	{},
};

static u_int edid_addr[E_PORT_NUM] = {
	TOP_EDID_ADDR_S,
	TOP_EDID_PORT2_ADDR_S,
	TOP_EDID_PORT3_ADDR_S,
	TOP_EDID_PORT4_ADDR_S
};

char *rx_get_cta_blk_name(u16 tag)
{
	int i = 0;

	for (i = 0; cta_blk[i].tag_code != TAG_MAX; i++) {
		if (tag == cta_blk[i].tag_code)
			break;
	}
	return cta_blk[i].blk_name;
}

unsigned char rx_get_edid_index(void)
{
	if (edid_mode < EDID_LIST_NUM &&
	    edid_mode > EDID_LIST_TOP)
		return edid_mode;
	if (edid_mode == 0) {
		rx_pr("edid: use Top edid\n");
		return EDID_LIST_TOP;
	}
	return EDID_LIST_NULL;
}

bool is_valid_edid_data(unsigned char *p_edid)
{
	bool ret = false;

	if (!p_edid)
		return ret;
	if (p_edid[0] == 0 &&
		p_edid[1] == 0xff &&
		p_edid[2] == 0xff &&
		p_edid[3] == 0xff &&
		p_edid[4] == 0xff &&
		p_edid[5] == 0xff &&
		p_edid[6] == 0xff &&
		p_edid[7] == 0)
		ret = true;

	return ret;
}
void hdmirx_fill_edid_buf(const char *buf, int size)
{
	bool err_chk = false;
	u32 u_offset;
	u32 i = 0;

	if (edid_delivery_mothed == EDID_DELIVERY_NULL)
		edid_delivery_mothed = EDID_DELIVERY_ALL_PORT;
	rx_pr("EDID size1 =%d\n", size);
	//return;
	//old delivery method. the size of each EDID is 256 BYTE
	if (size == EDID_TOTAL_SIZE ||
		size == 256 * rx_info.port_num * 2) {
		rx_pr("EDID size2 =%d\n", size);
		//port0
		memcpy(edid_buf1, buf, 256);
		if (!is_valid_edid_data(edid_buf1))
			err_chk = true;
		u_offset = 256;
		memcpy(edid_buf1 + EDID_SIZE, buf + u_offset, 256);
		if (!is_valid_edid_data(edid_buf1 + EDID_SIZE))
			err_chk = true;
		//port1
		u_offset += 256;
		memcpy(edid_buf2, buf + u_offset, 256);
		if (!is_valid_edid_data(edid_buf2))
			err_chk = true;
		u_offset += 256;
		memcpy(edid_buf2 + EDID_SIZE, buf + u_offset, 256);
		if (!is_valid_edid_data(edid_buf2 + EDID_SIZE))
			err_chk = true;
		//port2
		u_offset += 256;
		memcpy(edid_buf3, buf + u_offset, 256);
		if (!is_valid_edid_data(edid_buf3))
			err_chk = true;
		u_offset += 256;
		memcpy(edid_buf3 + EDID_SIZE, buf + u_offset, 256);
		if (!is_valid_edid_data(edid_buf3 + EDID_SIZE))
			err_chk = true;
		//port3
		memcpy(edid_buf4, edid_buf1, EDID_BUF_SIZE);
	}
	if (err_chk && rx_info.boot_flag) {
		rx_pr("EDID size =%d\n", size);
		for (i = 0; i < size; i++) {
			pr_cont("%02x", buf[i]);
			if (i % 128)
				rx_pr("");
		}
	} else {
		rx_pr("HDMIRX: fill edid buf, size %d\n", size);
	}
}

void hdmirx_fill_edid_with_port_buf(const char *buf, int size)
{
	u8 port_num;
	u8 edid_type;
	u32 i = 0;
	u32 j = 0;
	u32 k = 0;
	unsigned char buff[512] = {0};

	port_num = (buf[0] & 0x0f) - 1;
	edid_type = buf[0] >> 0x4;
	rx_pr("port%d edid size %d\n", port_num, size);

	if (hdmi_cec_en == 1) {
		port_hpd_rst_flag |= 1 << port_num;
		rx_set_port_hpd(port_num, 0);
		rx_pr("port%d_hpd_low\n", port_num);
	}
	if (size < 257) {
		rx_pr("Incomplete edid\n");
		return;
	}
	switch (port_num) {
	case 0:
		switch (edid_type) {
		case EDID_TYPE_256_PLUS_256:
			memcpy(edid_buf1, buf + 1, 256);
			memcpy(edid_buf1 + 512, buf + 257, 256);
		break;
		case EDID_TYPE_512_PLUS_512:
			memcpy(edid_buf1, buf + 1, 512);
			memcpy(edid_buf1 + 512, buf + 513, 512);
		break;
		case EDID_TYPE_256_PLUS_512:
			memcpy(edid_buf1, buf + 1, 256);
			memcpy(edid_buf1 + 512, buf + 257, size - 257);
		break;
		default:
			rx_pr("port 0 err edid_type\n");
		}
		if (!is_valid_edid_data(edid_buf1))
			rx_pr("1.4 edid error\n");
		if (!is_valid_edid_data(edid_buf1 + 512))
			rx_pr("2.0 edid error\n");
	break;
	case 1:
		switch (edid_type) {
		case EDID_TYPE_256_PLUS_256:
			memcpy(edid_buf2, buf + 1, 256);
			memcpy(edid_buf2 + 512, buf + 257, 256);
		break;
		case EDID_TYPE_512_PLUS_512:
			memcpy(edid_buf2, buf + 1, 512);
			memcpy(edid_buf2 + 512, buf + 513, 512);
		break;
		case EDID_TYPE_256_PLUS_512:
			memcpy(edid_buf2, buf + 1, 256);
			memcpy(edid_buf2 + 512, buf + 257, size - 257);
		break;
		default:
			rx_pr("port 1 err edid_type\n");
		}
		if (!is_valid_edid_data(edid_buf2))
			rx_pr("1.4 edid error\n");
		if (!is_valid_edid_data(edid_buf2 + 512))
			rx_pr("2.0 edid error\n");
	break;
	case 2:
		switch (edid_type) {
		case EDID_TYPE_256_PLUS_256:
			memcpy(edid_buf3, buf + 1, 256);
			memcpy(edid_buf3 + 512, buf + 257, 256);
		break;
		case EDID_TYPE_512_PLUS_512:
			memcpy(edid_buf3, buf + 1, 512);
			memcpy(edid_buf3 + 512, buf + 513, 512);
		break;
		case EDID_TYPE_256_PLUS_512:
			memcpy(edid_buf3, buf + 1, 256);
			memcpy(edid_buf3 + 512, buf + 257, size - 257);
		break;
		default:
			rx_pr("port 2 err edid_type\n");
		}
		if (!is_valid_edid_data(edid_buf3))
			rx_pr("1.4 edid error\n");
		if (!is_valid_edid_data(edid_buf3 + 512))
			rx_pr("2.0 edid error\n");
	break;
	case 3:
		switch (edid_type) {
		case EDID_TYPE_256_PLUS_256:
			memcpy(edid_buf4, buf + 1, 256);
			memcpy(edid_buf4 + 512, buf + 257, 256);
		break;
		case EDID_TYPE_512_PLUS_512:
			memcpy(edid_buf4, buf + 1, 512);
			memcpy(edid_buf4 + 512, buf + 513, 512);
		break;
		case EDID_TYPE_256_PLUS_512:
			memcpy(edid_buf4, buf + 1, 256);
			memcpy(edid_buf4 + 512, buf + 257, size - 257);
		break;
		default:
			rx_pr("port 3 err edid_type\n");
		}
		if (!is_valid_edid_data(edid_buf4))
			rx_pr("1.4 edid error\n");
		if (!is_valid_edid_data(edid_buf4 + 512))
			rx_pr("2.0 edid error\n");
	break;
	}
	rx_pr("EDID port%d - size %d\n", port_num, size);
	if (log_level & EDID_LOG) {
		for (i = 0; i * 16 < size; i++) {
			for (j = 0; j < 16; j++)
				k += sprintf(buff + k, "%02x", buf[i * 16 + j]);
			pr_info("%s", buff);
			k = 0;
		}
	}
}

void rx_edid_update_hdr_dv_info(unsigned char *p_edid)
{
	//if (hdmirx_repeat_support())
		//return;
#ifdef CONFIG_AMLOGIC_HDMITX
	if (tx_hdr_priority == 1) {
		//remove DV
		edid_rm_db_by_tag(p_edid, EXTENDED_VSVDB_TAG);
	} else if (tx_hdr_priority == 2) {
		//remove dv+hdr+hdr10p
		edid_rm_db_by_tag(p_edid, EXTENDED_HDR_STATIC_TAG);
		edid_rm_db_by_tag(p_edid, VSVDB_HDR10P_TAG);
		edid_rm_db_by_tag(p_edid, VSVDB_DV_TAG);
	}
#endif
}

void rx_edid_update_vrr_info(unsigned char *p_edid)
{
	u_int hf_vsdb_start = 0;
	u8 tag_len, i;

	if (!p_edid)
		return;
	hf_vsdb_start = rx_get_cea_tag_offset(p_edid, HF_VENDOR_DB_TAG);
	if (!hf_vsdb_start)
		return;
	tag_len = (p_edid[hf_vsdb_start] & 0xf) + 1;
	if (log_level & EDID_LOG)
		rx_pr("tag_len = %d", tag_len);
	if (tag_len <= 9)
		return;

	if (vrr_func_en) {
		if (rx_info.vrr_min == 0)
			return;
		p_edid[hf_vsdb_start + 9] = rx_info.vrr_min;
		p_edid[hf_vsdb_start + 10] =
			rx_info.vrr_max >= 100 ? rx_info.vrr_max : 0;
		if (log_level & EDID_LOG)
			rx_pr("modify vrr min = %d, vrr_max = %d\n",
				  rx_info.vrr_min, rx_info.vrr_max);
	} else {
		edid_rm_db_by_tag(p_edid, VSDB_FREESYNC_TAG);
		rx_pr("hf_vsdb_start = %d", hf_vsdb_start);
		p_edid[hf_vsdb_start] = 0x68;
		p_edid[hf_vsdb_start + 8] &= 0x02;
		for (i = hf_vsdb_start + 9; i < 254 - tag_len + 9; i++)
			p_edid[i] = p_edid[i + tag_len - 9];
		for (i = 254 - tag_len + 9; i < 254; i++)
			p_edid[i] = 0;
		/* dtd offset modify */
		p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -= tag_len - 9;
	}
}

void rx_edid_update_allm_info(unsigned char *p_edid)
{
	u_int hf_vsdb_start = 0;
	u8 tag_len;

	if (!p_edid)
		return;
	hf_vsdb_start = rx_get_cea_tag_offset(p_edid, HF_VENDOR_DB_TAG);
	if (!hf_vsdb_start)
		return;
	tag_len = p_edid[hf_vsdb_start] & 0xf;
	if (tag_len < 8 || allm_func_en == 0xff)
		return;
	if (log_level & EDID_LOG)
		rx_pr("tag_len = %d", tag_len);

	if (allm_func_en == 1) {
		p_edid[hf_vsdb_start + 8] |= 0x2;
		if (log_level & EDID_LOG)
			rx_pr("enable allm.\n");
	} else if (allm_func_en == 0) {
		p_edid[hf_vsdb_start + 8] &= ~0x2;
		if (log_level & EDID_LOG)
			rx_pr("disable allm.\n");
	} else {
		rx_pr("invalid allm_func_en: %d.\n", allm_func_en);
	}
}

unsigned int rx_exchange_bits(unsigned int value)
{
	unsigned int temp;

	temp = value & 0xF;
	value = (((value >> 4) & 0xF) | (value & 0xFFF0));
	value = ((value & 0xFF0F) | (temp << 4));
	temp = value & 0xF00;
	value = (((value >> 4) & 0xF00) | (value & 0xF0FF));
	value = ((value & 0x0FFF) | (temp << 4));
	return value;
}

u16 rx_get_tag_code(u_char *edid_data)
{
	u16 tag_code = TAG_MAX;
	u16 tmp_tag;
	unsigned int ieee_oui;

	if (!edid_data)
		return tag_code;

	tmp_tag = *edid_data >> 5;
	/* extern tag */
	if (tmp_tag == USE_EXTENDED_TAG) {
		if (edid_data[1] == VSVDB_TAG) {
			ieee_oui = (edid_data[4] << 16) |
				(edid_data[3] << 8) | edid_data[2];
			if (ieee_oui == 0x00D046)
				tag_code = VSVDB_DV_TAG;
			else if (ieee_oui == 0x90848B)
				tag_code = VSVDB_HDR10P_TAG;
		} else if (edid_data[1] == HDR_STATIC_TAG) {
			tag_code = EXTENDED_HDR_STATIC_TAG;
		} else if (edid_data[1] == HDR_DYNAMIC_TAG) {
			tag_code = EXTENDED_HDR_DYNAMIC_TAG;
		} else {
			tag_code =
				(USE_EXTENDED_TAG << 8) | edid_data[1];
		}
	} else if (tmp_tag == VENDOR_TAG) {
		/* diff VSDB with HF-VSDB */
		ieee_oui = (edid_data[3] << 16) |
			(edid_data[2] << 8) | edid_data[1];
		if (ieee_oui == 0x000C03)
			tag_code = tmp_tag;
		else if (ieee_oui == 0xC45DD8)
			tag_code = HF_VENDOR_DB_TAG;
		else if (ieee_oui == 0x00001A)
			tag_code = VSDB_FREESYNC_TAG;
	} else {
		tag_code = tmp_tag;
	}

	return tag_code;
}

u_int rx_get_ceadata_offset(u_char *cur_edid, u_char *addition)
{
	int i;
	u16 type;
	unsigned char max_offset;

	if (!cur_edid || !addition)
		return 0;

	type = rx_get_tag_code(addition);
	i = EDID_DEFAULT_START;/*block check start index*/
	max_offset = cur_edid[130] + EDID_EXT_BLK_OFF;

	while (i < max_offset) {
		if (type == rx_get_tag_code(cur_edid + i)) {
			if (log_level & EDID_LOG)
				rx_pr("type: %#x, start addr: %#x\n", type, i);
			return i;
		}
		i += (1 + (*(cur_edid + i) & 0x1f));
	}
	return 0;
}

u_int rx_get_cea_tag_offset(u8 *cur_edid, u16 tag_code)
{
	int i;
	unsigned char max_offset;

	if (!cur_edid)
		return 0;
	if (!cur_edid[126])
		return 0;
	i = EDID_DEFAULT_START;/*block check start index*/
	max_offset = cur_edid[130] + EDID_EXT_BLK_OFF;

	while (i < max_offset) {
		if (tag_code == rx_get_tag_code(cur_edid + i)) {
			if (log_level & EDID_LOG)
				rx_pr("tag: %#x, start addr: %#x\n", tag_code, i);
			return i;
		}
		i += (1 + (*(cur_edid + i) & 0x1f));
	}
	if (log_level & EDID_LOG)
		rx_pr("tag: %#x, start addr: %#x\n", tag_code, i);

	return 0;
}

int rx_edid_free_size(u8 *cur_edid, int size)
{
	int block_start;

	if (cur_edid == 0)
		return -1;
	/*get description offset*/
	block_start = cur_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET];
	block_start += EDID_BLOCK1_OFFSET;
	if (log_level & EDID_LOG)
		rx_pr("%s block_start:%d\n", __func__, block_start);
	/*find the empty data index*/
	while ((cur_edid[block_start] > 0) &&
	       (block_start < size - 1)) {
		if (log_level & EDID_LOG)
			rx_pr("%s running:%d\n", __func__, block_start);
		if ((cur_edid[block_start] & 0x1f) == 0)
			break;
		block_start += DETAILED_TIMING_LEN;
	}
	if (log_level & EDID_LOG)
		rx_pr("%s block_start end:%d\n", __func__, block_start);
	/*compute the free size*/
	if (block_start <= size - 1)
		return (size - block_start - 1);
	else
		return -1;
}

bool rx_edid_cal_phy_addr(u_int up_addr,
			  u_int portmap,
			  u_char *pedid,
			  u_int *phy_offset,
			  u_int *phy_addr)

{
	u_int root_offset = 0;
	u_int vsdb_offset = 0;
	u_int i;
	bool flag = false;

	if (!(pedid && phy_offset && phy_addr))
		return flag;

	/* find phy addr offset */
	vsdb_offset = rx_get_cea_tag_offset(pedid, VENDOR_TAG);
	if (vsdb_offset == 0x0)
		return flag;

	*phy_offset = vsdb_offset + 4;
	flag = true;
	/* reset phy addr to 0 firstly */
	pedid[vsdb_offset + 4] = 0x0;
	pedid[vsdb_offset + 5] = 0x0;
	if (1) {
		/*get the root index*/
		i = 0;
		while (i < 4) {
			if (((up_addr << (i * 4)) & 0xf000) != 0) {
				root_offset = i;
				break;
			}
			i++;
		}
		if (i == 4)
			root_offset = 4;

		for (i = 0; i < rx_info.port_num; i++) {
			if (root_offset == 0)
				phy_addr[i] = 0xFFFF;
			else
				phy_addr[i] = (up_addr |
				((((portmap >> i * 4) & 0xf) << 12) >>
				(root_offset - 1) * 4));

			phy_addr[i] = rx_exchange_bits(phy_addr[i]);
		}
	} else {
		for (i = 0; i < rx_info.port_num; i++)
			phy_addr[i] = ((portmap >> i * 4) & 0xf) << 4;
	}

	return flag;
}

void rx_edid_reset(u8 port)
{
	if (!rx_is_need_edid_reset(port))
		return;
	rx_edid_module_reset();
}

bool is_ddc_idle(unsigned char port_id)
{
	unsigned int sts;
	unsigned int ddc_sts;
	unsigned int ddc_offset;

	switch (port_id) {
	case 0:
		sts = hdmirx_rd_top(TOP_EDID_GEN_STAT, E_PORT0);
		break;
	case 1:
		sts = hdmirx_rd_top(TOP_EDID_GEN_STAT_B, E_PORT1);
		break;
	case 2:
		sts = hdmirx_rd_top(TOP_EDID_GEN_STAT_C, E_PORT2);
		break;
	case 3:
		sts = hdmirx_rd_top(TOP_EDID_GEN_STAT_D, E_PORT3);
		break;
	default:
		sts = 0;
		break;
	}

	ddc_sts = (sts >> 20) & 0x1f;
	ddc_offset = sts & 0xff;

	if (ddc_sts == 0 &&
	    (ddc_offset == 0xff ||
	     ddc_offset == 0))
		return true;

	if (log_level & VIDEO_LOG)
		rx_pr("ddc busy\n");

	return false;
}

bool is_edid_buff_normal(unsigned char port_id)
{
	unsigned int edid_sts_temp;
	unsigned int edid_addr_sts;

	switch (port_id) {
	case 0:
		edid_sts_temp = hdmirx_rd_top(TOP_EDID_GEN_STAT, E_PORT0);
		break;
	case 1:
		edid_sts_temp = hdmirx_rd_top(TOP_EDID_GEN_STAT_B, E_PORT1);
		break;
	case 2:
		edid_sts_temp = hdmirx_rd_top(TOP_EDID_GEN_STAT_C, E_PORT2);
		break;
	case 3:
		edid_sts_temp = hdmirx_rd_top(TOP_EDID_GEN_STAT_D, E_PORT3);
		break;
	default:
		edid_sts_temp = 0;
		break;
	}

	edid_addr_sts = edid_sts_temp & 0xffff;
	if (edid_addr_sts > 0x200) {
		if (log_level & 0x100)
			rx_pr("edid buff flow\n");
		return false;
	}

	return true;
}

enum edid_ver_e rx_parse_edid_ver(u8 *p_edid)
{
	if (rx_get_cea_tag_offset(p_edid, HF_VENDOR_DB_TAG))
		return EDID_V20;
	else
		return EDID_V14;
}

enum edid_ver_e get_edid_selection(u8 port)
{
	return rx[port].edid_type.edid_ver;
}

/* @func: seek dd+ atmos bit
 * @param:get audio type info by cec message:
 *	request short audio descriptor
 */
unsigned char rx_parse_arc_aud_type(const unsigned char *buff)
{
	unsigned char tmp[7] = {0};
	unsigned char aud_length = 0;
	unsigned char i = 0;
	unsigned int aud_data = 0;
	int ret = 0;

	aud_length = strlen(buff);
	rx_pr("length = %d\n", aud_length);

	for (i = 0; i < aud_length; i += 6) {
		tmp[6] = '\0';
		memcpy(tmp, buff + i, 6);
		/* ret = sscanf(tmp, "%x", &aud_data); */
		ret = kstrtoint(tmp, 0, &aud_data);
		if (ret != 1)
			rx_pr("kstrtoint failed\n");
		if (aud_data >> 19 == AUDIO_FORMAT_DDP)
			break;
	}
	if (i < aud_length && (aud_data & 0x1) == 0x1) {
		if (need_support_atmos_bit != 1) {
			need_support_atmos_bit = 1;
			hdmi_rx_top_edid_update();
			if (rx_info.main_port_open && rx_info.main_port != rx_info.arc_port) {
				if (atmos_edid_update_hpd_en)
					rx_send_hpd_pulse(rx_info.main_port);
				rx_pr("*update edid-atmos*\n");
			} else {
				pre_port = 0xff;
				rx_pr("update atmos later, in arc port:%s\n",
				      rx_info.main_port == rx_info.arc_port ? "Y" : "N");
			}
		}
	} else {
		if (need_support_atmos_bit) {
			need_support_atmos_bit = 0;
			hdmi_rx_top_edid_update();
			if (rx_info.main_port_open && rx_info.main_port != rx_info.arc_port) {
				if (atmos_edid_update_hpd_en)
					rx_send_hpd_pulse(rx_info.main_port);
				rx_pr("*update edid-no atmos*\n");
			} else {
				pre_port = 0xff;
				rx_pr("update atmos later, in arc port:%s\n",
				      rx_info.main_port == rx_info.arc_port ? "Y" : "N");
			}
		}
	}

	return 0;
}

/*
 * audio parse the atmos info and inform hdmirx via a flag
 */
void rx_set_atmos_flag(bool en)
{
}
EXPORT_SYMBOL(rx_set_atmos_flag);

bool rx_get_atmos_flag(void)
{
	return false;
}
EXPORT_SYMBOL(rx_get_atmos_flag);

unsigned char get_atmos_offset(unsigned char *p_edid)
{
	unsigned char max_offset = 0;
	unsigned char tag_offset = 0;
	unsigned char tag_data = 0;
	unsigned char aud_length = 0;
	unsigned char i = 0;
	unsigned char ret = 0;

	max_offset = p_edid[130] + 128;
	tag_offset = 132;
	do {
		tag_data = p_edid[tag_offset];
		if ((tag_data & 0xE0) == 0x20) {
			if (log_level & EDID_LOG)
				rx_pr("audio_");
			aud_length = tag_data & 0x1F;
			break;
		}
		tag_offset += (tag_data & 0x1F) + 1;
	} while (tag_offset < max_offset);

	for (i = 0; i < aud_length; i += 3) {
		if (p_edid[tag_offset + 1 + i] >> 3 == 10) {
			ret = tag_offset + 1 + i + 2;
			break;
		}
	}
	return ret;
}

u_char rx_edid_get_aud_sad(u_char *sad_data)
{
	u8 port = rx_info.main_port; //todo

	if (rx_info.chip_id == CHIP_ID_NONE)
		return 0;
	u_char *pedid = rx_get_cur_used_edid(port);
	u_char offset = rx_get_cea_tag_offset(pedid, AUDIO_TAG);
	u_char len = 0;

	if (!sad_data || !offset)
		return 0;
	len = BLK_LENGTH(pedid[offset]);
	memcpy(sad_data, pedid + offset + 1, len);
	return len;
}
EXPORT_SYMBOL(rx_edid_get_aud_sad);

/* audio module parse the short audio descriptors
 * info from ARC/eARC and inform hdmirx
 * if len = 0, revert to original edid audio blk
 */
bool rx_edid_set_aud_sad(u_char *sad, u_char len)
{
	if (rx_info.chip_id == CHIP_ID_NONE)
		return false;
	if (len > 30 || len % 3 != 0) {
		if (log_level & AUDIO_LOG)
			rx_pr("err sad length: %d\n", len);
		return false;
	}
	memset(tmp_sad, 0, sizeof(tmp_sad));
	if (sad)
		memcpy(tmp_sad, sad, len);
	if (tmp_sad_len != len) {
		tmp_sad_len = len;
	} else {
		if (!len)
			return false;
		tmp_sad_len = len;
	}
	hdmi_rx_top_edid_update();
	if (rx_info.main_port_open && rx_info.main_port != rx_info.arc_port) {
		if (atmos_edid_update_hpd_en)
			rx_send_hpd_pulse(rx_info.main_port);
		if (log_level & AUDIO_LOG)
			rx_pr("*update aud SAD len: %d*\n", len);
	} else {
		pre_port = 0xff;
		if (log_level & AUDIO_LOG)
			rx_pr("update aud SAD later, in arc port:%s\n",
			      rx_info.main_port == rx_info.arc_port ? "Y" : "N");
	}
	return true;
}
EXPORT_SYMBOL(rx_edid_set_aud_sad);

void rx_earc_hpd_cntl(void)
{
	if (rx_info.chip_id == CHIP_ID_NONE)
		return;
	if (rx_info.main_port_open && rx_info.main_port == rx_info.arc_port) {
		rx_send_hpd_pulse(rx_info.main_port);
		if (log_level & AUDIO_LOG)
			rx_pr("earc_hpd_cntl\n");
	} else {
		schedule_work(&earc_hpd_dwork);
		pre_port = 0xff;
	}
}
EXPORT_SYMBOL(rx_earc_hpd_cntl);

bool rx_edid_update_aud_blk(u_char *pedid,
			    u_char *sad_data,
			    u_char len)
{
	u_char tmp_aud_blk[128];

	if (!pedid)
		return false;
	/* if len = 0, revert to original edid */
	if (len == 0 || len > 30 || len % 3 != 0) {
		if (log_level & AUDIO_LOG)
			rx_pr("err SAD length: %d\n", len);
		return false;
	}
	tmp_aud_blk[0] = (0x1 << 5) | len;
	memcpy(tmp_aud_blk + 1, sad_data, len);
	edid_rm_db_by_tag(pedid, AUDIO_TAG);
	/* place aud data blk to blk index = 0x1 */
	splice_data_blk_to_edid(pedid, tmp_aud_blk, 0x1);
	return true;
}

/* update atmos bit
 * it will also update audio data blk(if required from Audio)
 */
unsigned char rx_edid_update_sad(unsigned char *p_edid)
{
	unsigned char offset = 0;

	if (need_support_atmos_bit != 0xff)	{
		offset = get_atmos_offset(p_edid);
		if (offset == 0) {
			if (log_level & EDID_LOG)
				rx_pr("can not find atmos info\n");
		} else {
			if (need_support_atmos_bit)
				p_edid[offset] = 1;
			else
				p_edid[offset] = 0;
			if (log_level & EDID_LOG)
				rx_pr("offset = %d\n", offset);
		}
	}
	rx_edid_update_aud_blk(p_edid, tmp_sad, tmp_sad_len);
	return 0;
}

static void rx_edid_update_vsvdb(u_char *pedid,
				 u_char *add_data, u_char len)
{
	/* if len = 0xFF, revert to original edid, no change */
	if (!pedid || !add_data || len == 0xFF)
		return;

	if (hdmirx_repeat_support())
		return;

	/* if len = 0, means disable.
	 * now only consider VSVDB of DV, not HDR10+
	 */
	if (len == 0)
		edid_rm_db_by_tag(pedid, VSVDB_DV_TAG);
	else
		splice_tag_db_to_edid(pedid, add_data,
				      VSVDB_DV_TAG);
}

bool need_update_edid(u8 port)
{
	bool ret = false;

	/* for TM2 with independent edid on each port,
	 * no need to update edid unless edid change.
	 * but for chips <= TL1, or
	 * edid version is set to auto, still need to
	 * update edid on port open
	 */
	if (rx_info.chip_id <= CHIP_ID_TL1)
		ret = true;

	return ret;
}

void rx_edid_print_vic_fmt(unsigned char i,
			   unsigned char hdmi_vic)
{
	/* CTA-861-G: Table-18 */
	/* SVD = 128, 254, 255 are reserved */
	if (hdmi_vic >= 1 && hdmi_vic <= 64) {
		rx_pr("\tSVD#%2d: vic(%3d), format: %s\n",
		      i + 1, hdmi_vic, hdmi_fmt[hdmi_vic]);
	} else if (hdmi_vic >= 65 && hdmi_vic <= 107) {
		/* from first new set */
		rx_pr("\tSVD#%2d: vic(%3d), format: %s\n",
		      i + 1, hdmi_vic, hdmi_fmt[hdmi_vic]);
	} else if (hdmi_vic >= 108 && hdmi_vic <= 127) {
		/* from first new set: 8bit VIC */
	} else if (hdmi_vic >= 129 && hdmi_vic <= 192) {
		hdmi_vic &= 0x7F;
		rx_pr("\tSVD#%2d: vic(%3d), native format: %s\n",
		      i + 1, hdmi_vic, hdmi_fmt[hdmi_vic]);
	} else if (hdmi_vic >= 193 && hdmi_vic <= 253) {
		/* from second new set: 8bit VIC */
	}
}

/* manufacturer name
 * offset 0x8~0x9
 */
static void get_edid_manufacturer_name(unsigned char *buff,
				       unsigned char start,
				       struct edid_info_s *edid_info)
{
	int i;
	unsigned char uppercase[26] = { 0 };
	unsigned char brand[3];

	if (!edid_info || !buff)
		return;
	/* Fill array uppercase with 'A' to 'Z' */
	for (i = 0; i < 26; i++)
		uppercase[i] = 'A' + i;
	/* three 5-byte AscII code, such as 'A' = 00001, 'C' = 00011,*/
	brand[0] = buff[start] >> 2;
	brand[1] = ((buff[start] & 0x3) << 3) + (buff[start + 1] >> 5);
	brand[2] = buff[start + 1] & 0x1f;
	for (i = 0; i < 3; i++)
		edid_info->manufacturer_name[i] = uppercase[brand[i] - 1];
}

/* product code
 * offset 0xA~0xB
 */
static void get_edid_product_code(unsigned char *buff,
				  unsigned char start,
				  struct edid_info_s *edid_info)
{
	if (!edid_info || !buff)
		return;
	edid_info->product_code = buff[start + 1] << 8 | buff[start];
}

/* serial number
 * offset 0xC~0xF
 */
static void get_edid_serial_number(unsigned char *buff,
				   unsigned char start,
				   struct edid_info_s *edid_info)
{
	if (!edid_info || !buff)
		return;
	edid_info->serial_number = buff[start + 3] << 24 |
				   buff[start + 2] << 16 |
				   buff[start + 1] << 8 |
				   buff[start];
}

/* manufacture date
 * offset 0x10~0x11
 */
static void get_edid_manufacture_date(unsigned char *buff,
				      unsigned char start,
				      struct edid_info_s *edid_info)
{
	if (!edid_info || !buff)
		return;
	/* week of manufacture:
	 * 0: not specified
	 * 0x1~0x35: valid week
	 * 0x36~0xfe: reserved
	 * 0xff: model year is specified
	 */
	if (buff[start] == 0 ||
	    (buff[start] >= 0x36 && buff[start] <= 0xfe))
		edid_info->product_week = 0;
	else
		edid_info->product_week = buff[start];
	/* year of manufacture,
	 * or model year (if specified by week=0xff)
	 */
	edid_info->product_year = buff[start + 1] + 1990;
}

/* The EDID version in base block
 * offset 0x12 and 0x13
 *static void get_edid_version(unsigned char *buff,
 *	unsigned char start, struct edid_info_s *edid_info)
 *{
 *	if (!edid_info || !buff)
 *		return;
 *	edid_info->edid_version = buff[start];
 *	edid_info->edid_revision = buff[start+1];
 *}
 */

/* Basic Display Parameters and Features
 * offset 0x14~0x18
 */
static void get_edid_display_parameters(unsigned char *buff,
					unsigned char start,
					struct edid_info_s *edid_info)
{}

/* Color Characteristics. Color Characteristics provides information about
 * the display device's chromaticity and color temperature parameters
 * (white temperature in degrees Kelvin)
 * offset 0x19~0x22
 */
static void get_edid_color_characteristics(unsigned char *buff,
					   unsigned char start,
					   struct edid_info_s *edid_info)
{}

/* established timings are computer display timings recognized by VESA.
 * offset 0x23~0x25
 */
static void get_edid_established_timings(unsigned char *buff,
					 unsigned char start,
					 struct edid_info_s *edid_info)
{
	if (!edid_info || !buff)
		return;
	rx_pr("established timing:\n");
	/* each bit for an established timing */
	if (buff[start] & (1 << 5))
		rx_pr("\t640*480p60hz\n");
	if (buff[start] & (1 << 0))
		rx_pr("\t800*600p60hz\n");
	if (buff[start + 1] & (1 << 3))
		rx_pr("\t1024*768p60hz\n");
}

/* Standard timings are those either recognized by VESA
 * through the VESA Discrete Monitor Timing or Generalized
 * Timing Formula standards. Each timing is two bytes.
 * offset 0x26~0x35
 */
static void get_edid_standard_timing(unsigned char *buff,
				     unsigned char start,
				     unsigned int length,
				     struct edid_info_s *edid_info)
{
	unsigned char  i, img_aspect_ratio;
	int hactive_pixel, vactive_pixel, refresh_rate;
	int asp_ratio[] = {
		80 * 10 / 16,
		80 * 3 / 4,
		80 * 4 / 5,
		80 * 9 / 16,
	};/*multiple 80 first*/
	if (!edid_info || !buff)
		return;
	rx_pr("standard timing:\n");
	for (i = 0; i < length; i = i + 2) {
		if (buff[start + i] != 0x01 && buff[start + i + 1] != 0x01) {
			hactive_pixel = (int)((buff[start + i] + 31) * 8);
			/* image aspect ratio:
			 * 0 -> 16:10
			 * 1 -> 4:3
			 * 2 -> 5:4
			 * 3 -> 16:9
			 */
			img_aspect_ratio = (buff[start + i + 1] >> 6) & 0x3;
			vactive_pixel =
				hactive_pixel * asp_ratio[img_aspect_ratio] / 80;
			refresh_rate = (int)(buff[start + i + 1] & 0x3F) + 60;
			rx_pr("\t%d*%dP%dHz\n", hactive_pixel,
			      vactive_pixel, refresh_rate);
		}
	}
}

static void get_edid_monitor_name(unsigned char *p_edid,
				  unsigned char start,
				  struct edid_info_s *edid_info)
{
	unsigned char i, j;

	if (!p_edid || !edid_info)
		return;
	/* CEA861-F Table83 */
	for (i = 0; i < 4; i++) {
		/* 0xFC denotes that last 13 bytes of this
		 * descriptor block contain Monitor name
		 */
		if (p_edid[start + i * 18 + 3] == 0xFC)
			break;
	}
	/* if name < 13 bytes, terminate name with 0x0A
	 * and fill remainder of 13 bytes with 0x20
	 */
	for (j = 0; j < 13; j++) {
		if (p_edid[start + i * 18 + 5 + j] == 0x0A)
			break;
		edid_info->monitor_name[j] = p_edid[0x36 + i * 18 + 5 + j];
	}
}

static void get_edid_range_limits(unsigned char *p_edid,
				  unsigned char start,
				  struct edid_info_s *edid_info)
{
	unsigned char i;

	for (i = 0; i < 4; i++) {
		/* 0xFD denotes that last 13 bytes of this
		 * descriptor block contain monitor range limits
		 */
		if (p_edid[start + i * 18 + 3] == 0xFD)
			break;
	}
	/*maximum supported pixel clock*/
	edid_info->max_sup_pixel_clk = p_edid[0x36 + i * 18 + 9] * 10;
}

/* edid parse
 * Descriptor data
 * 0xff monitor S/N
 * 0xfe ASCII data string
 * 0xfd monitor range limits
 * 0xfc monitor name
 * 0xfb color point
 * 0xfa standard timing ID
 */
void edid_parse_block0(u8 *p_edid, struct edid_info_s *edid_info)
{

	if (!p_edid || !edid_info)
		return;
	/* manufacturer name offset 8~9 */
	get_edid_manufacturer_name(p_edid, 8, edid_info);
	/* product code offset 10~11 */
	get_edid_product_code(p_edid, 10, edid_info);
	/* serial number offset 12~15 */
	get_edid_serial_number(p_edid, 12, edid_info);

	/* product date offset 0x10~0x11 */
	get_edid_manufacture_date(p_edid, 0x10, edid_info);
	/* EDID version offset 0x12~0x13*/
	/* get_edid_version(p_edid, 0x12, edid_info); */
	/* Basic Display Parameters and Features offset 0x14~0x18 */
	get_edid_display_parameters(p_edid, 0x14, edid_info);
	/* Color Characteristics offset 0x19~0x22 */
	get_edid_color_characteristics(p_edid, 0x19, edid_info);
	/* established timing offset 0x23~0x25 */
	get_edid_established_timings(p_edid, 0x23, edid_info);
	/* standard timing offset 0x26~0x35*/
	get_edid_standard_timing(p_edid, 0x26, 16, edid_info);
	/* best resolution: hactive*vactive*/
	edid_info->descriptor1.hactive =
		p_edid[0x38] + (((p_edid[0x3A] >> 4) & 0xF) << 8);
	edid_info->descriptor1.vactive =
		p_edid[0x3B] + (((p_edid[0x3D] >> 4) & 0xF) << 8);
	edid_info->descriptor1.pixel_clk =
		(p_edid[0x37] << 8) + p_edid[0x36];
	/* monitor name */
	get_edid_monitor_name(p_edid, 0x36, edid_info);
	get_edid_range_limits(p_edid, 0x36, edid_info);
	edid_info->extension_flag = p_edid[0x7e];
	edid_info->block0_chk_sum = p_edid[0x7f];
}

static void get_edid_video_data(unsigned char *buff,
				unsigned char start,
				unsigned char len,
				struct cta_blk_parse_info *edid_info)
{
	unsigned char i;

	if (!buff || !edid_info)
		return;
	edid_info->video_db.svd_num = len;
	for (i = 0; i < len; i++) {
		edid_info->video_db.hdmi_vic[i] =
			buff[i + start];
	}
}

static void get_edid_audio_data(unsigned char *buff,
				unsigned char start,
				unsigned char len,
				struct cta_blk_parse_info *edid_info)
{
	enum edid_audio_format_e fmt;
	int i = start;
	struct pcm_t *pcm;

	if (!buff || !edid_info)
		return;
	do {
		fmt = (buff[i] & 0x78) >> 3;/* bit6~3 */
		edid_info->audio_db.aud_fmt_sup[fmt] = 1;
		/* CEA-861F page 82: audio data block*/
		switch (fmt) {
		case AUDIO_FORMAT_LPCM:
			pcm = &edid_info->audio_db.sad[fmt].bit_rate.pcm;
			edid_info->audio_db.sad[fmt].max_channel =
				(buff[i] & 0x7);
			if (buff[i + 1] & 0x40)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_192khz = 1;
			if (buff[i + 1] & 0x20)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_176_4khz = 1;
			if (buff[i + 1] & 0x10)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_96khz = 1;
			if (buff[i + 1] & 0x08)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_88_2khz = 1;
			if (buff[i + 1] & 0x04)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_48khz = 1;
			if (buff[i + 1] & 0x02)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_44_1khz = 1;
			if (buff[i + 1] & 0x01)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_32khz = 1;
			if (buff[i + 2] & 0x04)
				pcm->size_24bit = 1;
			if (buff[i + 2] & 0x02)
				pcm->size_20bit = 1;
			if (buff[i + 2] & 0x01)
				pcm->size_16bit = 1;
			break;
		/* CEA861F table50 fmt2~8 byte 3:
		 * Maximum bit rate divided by 8 kHz
		 */
		case AUDIO_FORMAT_AC3:
		case AUDIO_FORMAT_MPEG1:
		case AUDIO_FORMAT_MP3:
		case AUDIO_FORMAT_MPEG2:
		case AUDIO_FORMAT_AAC:
		case AUDIO_FORMAT_DTS:
		case AUDIO_FORMAT_ATRAC:
			edid_info->audio_db.sad[fmt].max_channel =
				(buff[i] & 0x07);
			if (buff[i + 1] & 0x40)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_192khz = 1;
			if (buff[i + 1] & 0x20)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_176_4khz = 1;
			if (buff[i + 1] & 0x10)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_96khz = 1;
			if (buff[i + 1] & 0x08)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_88_2khz = 1;
			if (buff[i + 1] & 0x04)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_48khz = 1;
			if (buff[i + 1] & 0x02)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_44_1khz = 1;
			if (buff[i + 1] & 0x01)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_32khz = 1;
			edid_info->audio_db.sad[fmt].bit_rate.others =
				buff[i + 2];
			break;
		/* for audio format code 9~13:
		 * byte3 is dependent on Audio Format Code
		 */
		case AUDIO_FORMAT_OBA:
		case AUDIO_FORMAT_DDP:
		case AUDIO_FORMAT_DTSHD:
		case AUDIO_FORMAT_MAT:
		case AUDIO_FORMAT_DST:
			edid_info->audio_db.sad[fmt].max_channel =
				(buff[i] & 0x07);
			if (buff[i + 1] & 0x40)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_192khz = 1;
			if (buff[i + 1] & 0x20)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_176_4khz = 1;
			if (buff[i + 1] & 0x10)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_96khz = 1;
			if (buff[i + 1] & 0x08)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_88_2khz = 1;
			if (buff[i + 1] & 0x04)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_48khz = 1;
			if (buff[i + 1] & 0x02)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_44_1khz = 1;
			if (buff[i + 1] & 0x01)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_32khz = 1;
			edid_info->audio_db.sad[fmt].bit_rate.others = buff[i + 2];
			break;
		/* audio format code 14:
		 * last 3 bits of byte3: profile
		 */
		case AUDIO_FORMAT_WMAPRO:
			edid_info->audio_db.sad[fmt].max_channel =
				(buff[i] & 0x07);
			if (buff[i + 1] & 0x40)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_192khz = 1;
			if (buff[i + 1] & 0x20)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_176_4khz = 1;
			if (buff[i + 1] & 0x10)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_96khz = 1;
			if (buff[i + 1] & 0x08)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_88_2khz = 1;
			if (buff[i + 1] & 0x04)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_48khz = 1;
			if (buff[i + 1] & 0x02)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_44_1khz = 1;
			if (buff[i + 1] & 0x01)
				edid_info->audio_db.sad[fmt].usr.ssr.freq_32khz = 1;
			edid_info->audio_db.sad[fmt].bit_rate.others = buff[i + 2];
			break;
		/* case 15: audio coding extension coding type */
		default:
			break;
		}
		i += 3; /*next audio fmt*/
	} while (i < (start + len));
}

static void get_edid_speaker_data(unsigned char *buff,
				  unsigned char start,
				  unsigned char len,
				  struct cta_blk_parse_info *edid_info)
{
	int i;

	if (!buff || !edid_info)
		return;
	/* speaker allocation is 3 bytes long*/
	if (len != 3) {
		rx_pr("invalid length for speaker allocation data block: %d\n",
		      len);
		return;
	}
	for (i = 1; i <= 0x80; i = i << 1) {
		switch (buff[start] & i) {
		case 0x80:
			edid_info->speaker_alloc.flw_frw = 1;
			break;
		case 0x40:
			edid_info->speaker_alloc.rlc_rrc = 1;
			break;
		case 0x20:
			edid_info->speaker_alloc.flc_frc = 1;
			break;
		case 0x10:
			edid_info->speaker_alloc.rc = 1;
			break;
		case 0x08:
			edid_info->speaker_alloc.rl_rr = 1;
			break;
		case 0x04:
			edid_info->speaker_alloc.fc = 1;
			break;
		case 0x02:
			edid_info->speaker_alloc.lfe = 1;
			break;
		case 0x01:
			edid_info->speaker_alloc.fl_fr = 1;
			break;
		default:
			break;
		}
	}
	for (i = 1; i <= 0x4; i = i << 1) {
		switch (buff[start + 1] & i) {
		case 0x4:
			edid_info->speaker_alloc.fch = 1;
			break;
		case 0x2:
			edid_info->speaker_alloc.tc = 1;
			break;
		case 0x1:
			edid_info->speaker_alloc.flh_frh = 1;
			break;
		default:
			break;
		}
	}
}

/* if is valid vsdb, then return 1
 * if is valid hf-vsdb, then return 2
 * if is invalid db, then return 0
 */
static int get_edid_vsdb(unsigned char *buff,
			 unsigned char start,
			 unsigned char len,
			 struct cta_blk_parse_info *edid_info)
{
	unsigned char _3d_present_offset;
	unsigned char hdmi_vic_len;
	unsigned char hdmi_vic_offset;
	unsigned char i, j;
	unsigned int ieee_oui;
	unsigned char _3d_struct_all_offset;
	unsigned char hdmi_3d_len;
	unsigned char _3d_struct;
	unsigned char _2d_vic_order_offset;
	unsigned char temp_3d_len;
	int ret = 0;

	if (!buff || !edid_info)
		return 0;
	/* basic 5 bytes; others: extension fields */
	//coverity fixed
	//if (len < 5) {
		//rx_pr("invalid VSDB length: %d!\n", len);
		//return 0;
	//}
	ieee_oui = (buff[start + 2] << 16) |
		   (buff[start + 1] << 8) |
		   buff[start];
	if (ieee_oui != 0x000C03 &&
	    ieee_oui != 0xC45DD8) {
		rx_pr("invalid IEEE OUI\n");
		return 0;
	} else if (ieee_oui == 0xC45DD8) {
		ret = 2;
		goto hf_vsdb;
	}
	ret = 1;
	edid_info->vsdb.ieee_first = ieee_oui & 0xff;
	edid_info->vsdb.ieee_second = ieee_oui >> 8 & 0xff;
	edid_info->vsdb.ieee_third = ieee_oui >> 16 & 0xff;
	/* phy addr: 2 bytes*/
	edid_info->vsdb.a = (buff[start + 3] >> 4) & 0xf;
	edid_info->vsdb.b = buff[start + 3] & 0xf;
	edid_info->vsdb.c = (buff[start + 4] >> 4) & 0xf;
	edid_info->vsdb.d = buff[start + 4] & 0xf;
	/* after first 5 bytes: vsdb1.4 extension fields */
	if (len > 5) {
		edid_info->vsdb.support_ai = (buff[start + 5] >> 7) & 0x1;
		edid_info->vsdb.dc_48bit = (buff[start + 5] >> 6) & 0x1;
		edid_info->vsdb.dc_36bit = (buff[start + 5] >> 5) & 0x1;
		edid_info->vsdb.dc_30bit = (buff[start + 5] >> 4) & 0x1;
		edid_info->vsdb.dc_y444 = (buff[start + 5] >> 3) & 0x1;
		edid_info->vsdb.dvi_dual = buff[start + 5] & 0x1;
	}
	if (len > 6)
		edid_info->vsdb.max_tmds_clk = buff[start + 6];
	if (len > 7) {
		edid_info->vsdb.latency_fields_present =
			(buff[start + 7] >> 7) & 0x1;
		edid_info->vsdb.i_latency_fields_present =
			(buff[start + 7] >> 6) & 0x1;
		edid_info->vsdb.hdmi_video_present =
			(buff[start + 7] >> 5) & 0x1;
		edid_info->vsdb.cnc3 = (buff[start + 7] >> 3) & 0x1;
		edid_info->vsdb.cnc2 = (buff[start + 7] >> 2) & 0x1;
		edid_info->vsdb.cnc1 = (buff[start + 7] >> 1) & 0x1;
		edid_info->vsdb.cnc0 = buff[start + 7] & 0x1;
	}
	if (edid_info->vsdb.latency_fields_present) {
		if (len < 10) {
			rx_pr("invalid vsdb len for latency: %d\n", len);
			return 0;
		}
		edid_info->vsdb.video_latency = buff[start + 8];
		edid_info->vsdb.audio_latency = buff[start + 9];
		_3d_present_offset = 10;
	} else {
		rx_pr("latency fields not present\n");
		_3d_present_offset = 8;
	}
	if (edid_info->vsdb.i_latency_fields_present) {
		/* I_Latency_Fields_Present shall be zero
		 * if Latency_Fields_Present is zero.
		 */
		if (edid_info->vsdb.latency_fields_present) {
			rx_pr("i_latency_fields should not be set\n");
		} else if (len < 12) {
			rx_pr("invalid vsdb len for i_latency: %d\n", len);
			return 0;
		}
		edid_info->vsdb.interlaced_video_latency = buff[start + 10];
		edid_info->vsdb.interlaced_audio_latency = buff[start + 11];
		_3d_present_offset = 12;
	}
	/* HDMI_Video_present: If set then additional video format capabilities
	 * are described by using the fields starting after the Latency
	 * area. This consists of 4 parts with the order described below:
	 * 1 byte containing the 3D_present flag and other flags
	 * 1 byte with length fields HDMI_VIC_LEN and HDMI_3D_LEN
	 * zero or more bytes for information about HDMI_VIC formats supported
	 * (length of this field is indicated by HDMI_VIC_LEN).
	 * zero or more bytes for information about 3D formats supported
	 * (length of this field is indicated by HDMI_3D_LEN)
	 */
	if (edid_info->vsdb.hdmi_video_present) {
		/* if hdmi video present,
		 * 2 additional bytes at least will present
		 * 1 byte containing the 3D_present flag and other flags
		 * 1 byte with length fields HDMI_VIC_LEN and HDMI_3D_LEN
		 * 0 or more bytes for info about HDMI_VIC formats supported
		 * 0 or more bytes for info about 3D formats supported
		 */
		if (len < _3d_present_offset + 2) {
			rx_pr("invalid vsdb length for hdmi video: %d\n", len);
			return 0;
		}
		edid_info->vsdb._3d_present =
			(buff[start + _3d_present_offset] >> 7) & 0x1;
		edid_info->vsdb._3d_multi_present =
			(buff[start + _3d_present_offset] >> 5) & 0x3;
		edid_info->vsdb.image_size =
			(buff[start + _3d_present_offset] >> 3) & 0x3;
		edid_info->vsdb.hdmi_vic_len =
			(buff[start + _3d_present_offset + 1] >> 5) & 0x7;
		edid_info->vsdb.hdmi_3d_len =
			(buff[start + _3d_present_offset + 1]) & 0x1f;
		/* parse 4k2k video format, 4 4k2k format maximum*/
		hdmi_vic_offset = _3d_present_offset + 2;
		hdmi_vic_len = edid_info->vsdb.hdmi_vic_len;
		if (hdmi_vic_len > 4) {
			rx_pr("invalid hdmi vic len: %d\n",
			      edid_info->vsdb.hdmi_vic_len);
			return 0;
		}

		/* HDMI_VIC_LEN may be 0 */
		if (len < hdmi_vic_offset + hdmi_vic_len) {
			rx_pr("invalid length for 4k2k: %d\n", len);
			return 0;
		}
		for (i = 0; i < hdmi_vic_len; i++) {
			if (buff[start + hdmi_vic_offset + i] == 1)
				edid_info->vsdb.hdmi_4k2k_30hz_sup = 1;
			else if (buff[start + hdmi_vic_offset + i] == 2)
				edid_info->vsdb.hdmi_4k2k_25hz_sup = 1;
			else if (buff[start + hdmi_vic_offset + i] == 3)
				edid_info->vsdb.hdmi_4k2k_24hz_sup = 1;
			else if (buff[start + hdmi_vic_offset + i] == 4)
				edid_info->vsdb.hdmi_smpte_sup = 1;
		}

		/* 3D info parse */
		_3d_struct_all_offset =
			hdmi_vic_offset + hdmi_vic_len;
		hdmi_3d_len = edid_info->vsdb.hdmi_3d_len;
		/* there may be additional 0 present after 3D info  */
		if (len < _3d_struct_all_offset + hdmi_3d_len) {
			rx_pr("invalid vsdb length for 3d: %d\n", len);
			return 0;
		}
		/* 3d_multi_present: hdmi1.4 spec page155:
		 * 0: neither structure or mask present,
		 * 1: only 3D_Structure_ALL_15_0 is present
		 *    and assigns 3D formats to all of the
		 *    VICs listed in the first 16 entries
		 *    in the EDID
		 * 2: 3D_Structure_ALL_15_0 and 3D_MASK_15_0
		 *    are present and assign 3D formats to
		 *    some of the VICs listed in the first
		 *    16 entries in the EDID.
		 * 3: neither structure or mask present,
		 *    Reserved for future use.
		 */
		if (edid_info->vsdb._3d_multi_present == 1) {
			edid_info->vsdb._3d_struct_all =
				(buff[start + _3d_struct_all_offset] << 8) +
				buff[start + _3d_struct_all_offset + 1];
			_2d_vic_order_offset =
				_3d_struct_all_offset + 2;
			temp_3d_len = 2;
		} else if (edid_info->vsdb._3d_multi_present == 2) {
			edid_info->vsdb._3d_struct_all =
				(buff[start + _3d_struct_all_offset] << 8) +
				 buff[start + _3d_struct_all_offset + 1];
			edid_info->vsdb._3d_mask_15_0 =
				(buff[start + _3d_struct_all_offset + 2] << 8) +
				 buff[start + _3d_struct_all_offset + 3];
			_2d_vic_order_offset =
				_3d_struct_all_offset + 4;
			temp_3d_len = 4;
		} else {
			_2d_vic_order_offset =
				_3d_struct_all_offset;
			temp_3d_len = 0;
		}
		i = _2d_vic_order_offset;
		for (j = 0; (temp_3d_len < hdmi_3d_len) &&
		     (j < 16); j++) {
			edid_info->vsdb._2d_vic[j]._2d_vic_order =
				(buff[start + i] >> 4) & 0xF;
			edid_info->vsdb._2d_vic[j]._3d_struct =
				buff[start + i] & 0xF;
			_3d_struct =
				edid_info->vsdb._2d_vic[j]._3d_struct;
			/* hdmi1.4 spec page156
			 * if 3D_Structure_X is 0000~0111,
			 * 3D_Detail_X shall not be present,
			 * otherwise shall be present
			 */
			if (_3d_struct >= 0x8 && _3d_struct <= 0xF) {
				edid_info->vsdb._2d_vic[j]._3d_detail =
					(buff[start + i + 1] >> 4) & 0xF;
				i += 2;
				temp_3d_len += 2;
				if (temp_3d_len > hdmi_3d_len) {
					rx_pr("invalid len for 3d_detail: %d\n",
					      len);
					ret = 0;
					break;
				}
			} else {
				i++;
				temp_3d_len++;
			}
		}
		edid_info->vsdb._2d_vic_num = j;
		return ret;
	}
	return ret;
hf_vsdb:
	/* hdmi spec2.0 Table10-6 */
	//if (len < 4) {
		//rx_pr("invalid HF_VSDB length: %d!\n", len);
		//return 0;
	//}
	edid_info->contain_hf_vsdb = true;
	edid_info->hf_vsdb.ieee_first = ieee_oui & 0xff;
	edid_info->hf_vsdb.ieee_second = ieee_oui >> 8 & 0xff;
	edid_info->hf_vsdb.ieee_third = ieee_oui >> 16 & 0xff;
	edid_info->hf_vsdb.version =
		buff[start + 3];
	edid_info->hf_vsdb.max_tmds_rate =
		buff[start + 4];
	//pb3
	edid_info->hf_vsdb.scdc_present =
		(buff[start + 5] >> 7) & 0x1;
	edid_info->hf_vsdb.rr_cap =
		(buff[start + 5] >> 6) & 0x1;
	edid_info->hf_vsdb.cable_status =
		(buff[start + 5] >> 5) & 0x1;
	edid_info->hf_vsdb.ccbpci =
		(buff[start + 5] >> 4) & 0x1;
	edid_info->hf_vsdb.lte_340m_scramble =
		(buff[start + 5] >> 3) & 0x1;
	edid_info->hf_vsdb.independ_view =
		(buff[start + 5] >> 2) & 0x1;
	edid_info->hf_vsdb.dual_view =
		(buff[start + 5] >> 1) & 0x1;
	edid_info->hf_vsdb._3d_osd_disparity =
		buff[start + 5] & 0x1;
	//pb4
	edid_info->hf_vsdb.max_frl_rate =
		(buff[start + 6] >> 4) & 0x0f;
	edid_info->hf_vsdb.uhd_vic =
		(buff[start + 6] >> 3) & 0x1;
	edid_info->hf_vsdb.dc_48bit_420 =
		(buff[start + 6] >> 2) & 0x1;
	edid_info->hf_vsdb.dc_36bit_420 =
		(buff[start + 6] >> 1) & 0x1;
	edid_info->hf_vsdb.dc_30bit_420 =
		buff[start + 6] & 0x1;
	//pb5
	if (len <= 8)
		return ret;
	edid_info->hf_vsdb.qms_tfr_max =
		(buff[start + 7] >> 7) & 0x1;
	edid_info->hf_vsdb.qms =
		(buff[start + 7] >> 6) & 0x1;
	edid_info->hf_vsdb.m_delta =
		(buff[start + 7] >> 5) & 0x1;
	edid_info->hf_vsdb.qms_tfr_min =
		(buff[start + 7] >> 4) & 0x1;
	edid_info->hf_vsdb.neg_mvrr =
		(buff[start + 7] >> 3) & 0x1;
	edid_info->hf_vsdb.fva =
		(buff[start + 7] >> 2) & 0x1;
	edid_info->hf_vsdb.allm =
		(buff[start + 7] >> 1) & 0x1;
	edid_info->hf_vsdb.fapa_start_location =
		buff[start + 7] & 0x1;
	//pb6
	if (len <= 9)
		return ret;
	edid_info->hf_vsdb.vrr_max_hi =
		(buff[start + 8] >> 6) & 0x03;
	edid_info->hf_vsdb.vrr_min =
		(buff[start + 8]) & 0x3f;
	//pb7
	edid_info->hf_vsdb.vrr_max_lo =
		buff[start + 9];
	//pb8
	if (len <= 11)
		return ret;
	edid_info->hf_vsdb.dsc_1p2 =
		(buff[start + 10] >> 7) & 0x1;
	edid_info->hf_vsdb.dsc_native_420 =
		(buff[start + 10] >> 6) & 0x1;
	edid_info->hf_vsdb.fapa_end_extended =
		(buff[start + 10] >> 5) & 0x1;
	edid_info->hf_vsdb.rsv_0 =
		(buff[start + 10] >> 4) & 0x1;
	edid_info->hf_vsdb.dsc_all_bpp =
		(buff[start + 10] >> 3) & 0x1;
	edid_info->hf_vsdb.dsc_16bpc =
		(buff[start + 10] >> 2) & 0x1;
	edid_info->hf_vsdb.dsc_12bpc =
		(buff[start + 10] >> 1) & 0x1;
	edid_info->hf_vsdb.dsc_10bpc =
		buff[start + 10] & 0x1;
	//pb9
	edid_info->hf_vsdb.dsc_max_frl_rate =
		(buff[start + 11] >> 4) & 0xf;
	edid_info->hf_vsdb.dsc_max_slices =
		(buff[start + 11]) & 0xf;
	//pb10
	edid_info->hf_vsdb.dsc_total_chunk_kbytes =
		(buff[start + 11]) & 0x3f;
	return ret;
}

static void get_edid_vcdb(unsigned char *buff,
			  unsigned char start,
			  unsigned char len,
			  struct cta_blk_parse_info *edid_info)
{
	if (!buff || !edid_info)
		return;
	/* vcdb only contain 3 bytes data block. the source should
	 * ignore additional bytes (when present) and continue to
	 * parse the single byte as defined in CEA861-F Table 59.
	 */
	if (len != 2 - 1) {
		rx_pr("invalid length for video cap data block: %d!\n", len);
		/* return; */
	}
	edid_info->contain_vcdb = true;
	edid_info->vcdb.quanti_range_ycc = (buff[start] >> 7) & 0x1;
	edid_info->vcdb.quanti_range_rgb = (buff[start] >> 6) & 0x1;
	edid_info->vcdb.s_PT = (buff[start] >> 4) & 0x3;
	edid_info->vcdb.s_IT = (buff[start] >> 2) & 0x3;
	edid_info->vcdb.s_CE = buff[start] & 0x3;
}

static void get_edid_dv_data(unsigned char *buff,
			     unsigned char start,
			     unsigned char len,
			     struct cta_blk_parse_info *edid_info)
{
	unsigned int ieee_oui;

	if (!buff || !edid_info)
		return;
	if (len != 0xE - 1 &&
	    len != 0x19 - 1 &&
	    len != 0xB - 1) {
		rx_pr("invalid length for dolby vision vsvdb:%d\n",
		      len);
		return;
	}
	ieee_oui = buff[start + 2] << 16 |
		   buff[start + 1] << 8 |
		   buff[start];
	if (ieee_oui != 0x00D046) {
		rx_pr("invalid dolby vision ieee oui\n");
		return;
	}
	edid_info->contain_vsvdb = true;
	edid_info->dv_vsvdb.ieee_oui = ieee_oui;
	edid_info->dv_vsvdb.version = buff[start + 3] >> 5;
	if (edid_info->dv_vsvdb.version == 0x0) {
		/* length except extend code */
		if (len != 0x19 - 1) {
			rx_pr("invalid length for dolby vision ver0\n");
			return;
		}
		edid_info->dv_vsvdb.sup_global_dimming =
			(buff[start + 3] >> 2) & 0x1;
		edid_info->dv_vsvdb.sup_2160p60hz =
			(buff[start + 3] >> 1) & 0x1;
		edid_info->dv_vsvdb.sup_yuv422_12bit =
			buff[start + 3] & 0x1;
		edid_info->dv_vsvdb.Rx =
			((buff[start + 4] >> 4) & 0xF) + (buff[start + 5] << 4);
		edid_info->dv_vsvdb.Ry =
			(buff[start + 4] & 0xF) + (buff[start + 6] << 4);
		edid_info->dv_vsvdb.Gx =
			((buff[start + 7] >> 4) & 0xF) + (buff[start + 8] << 4);
		edid_info->dv_vsvdb.Gy =
			(buff[start + 7] & 0xF) + (buff[start + 9] << 4);
		edid_info->dv_vsvdb.Bx =
			((buff[start + 10] >> 4) & 0xF) + (buff[start + 11] << 4);
		edid_info->dv_vsvdb.By =
			(buff[start + 10] & 0xF) + (buff[start + 12] << 4);
		edid_info->dv_vsvdb.Wx =
			((buff[start + 13] >> 4) & 0xF) + (buff[start + 14] << 4);
		edid_info->dv_vsvdb.Wy =
			(buff[start + 13] & 0xF) + (buff[start + 15] << 4);
		edid_info->dv_vsvdb.tminPQ =
			((buff[start + 16] >> 4) & 0xF) + (buff[start + 17] << 4);
		edid_info->dv_vsvdb.tmaxPQ =
			(buff[start + 16] & 0xF) + (buff[start + 18] << 4);
		edid_info->dv_vsvdb.dm_major_ver =
			(buff[start + 19] >> 4) & 0xF;
		edid_info->dv_vsvdb.dm_minor_ver =
			buff[start + 19] & 0xF;
	} else if (edid_info->dv_vsvdb.version == 0x1) {
		/*length except extend code*/
		if (len == 0xB - 1) {
			/* todo: vsvdb v1-12 */
			rx_pr("vsvdb v1-12 todo\n");
			return;
		} else if (len != 0xE - 1) {
			rx_pr("invalid length for dolby vision ver1\n");
			return;
		}
		edid_info->dv_vsvdb.DM_version =
			(buff[start + 3] >> 2) & 0x7;
		edid_info->dv_vsvdb.sup_2160p60hz =
			(buff[start + 3] >> 1) & 0x1;
		edid_info->dv_vsvdb.sup_yuv422_12bit =
			buff[start + 3] & 1;
		edid_info->dv_vsvdb.target_max_lum =
			(buff[start + 4] >> 1) & 0x7F;
		edid_info->dv_vsvdb.sup_global_dimming =
			buff[start + 4] & 0x1;
		edid_info->dv_vsvdb.target_min_lum =
			(buff[start + 5] >> 1) & 0x7F;
		edid_info->dv_vsvdb.colorimetry =
			buff[start + 5] & 0x1;
		edid_info->dv_vsvdb.Rx = buff[start + 7];
		edid_info->dv_vsvdb.Ry = buff[start + 8];
		edid_info->dv_vsvdb.Gx = buff[start + 9];
		edid_info->dv_vsvdb.Gy = buff[start + 10];
		edid_info->dv_vsvdb.Bx = buff[start + 11];
		edid_info->dv_vsvdb.By = buff[start + 12];
	} else if (edid_info->dv_vsvdb.version == 0x2) {
		/* todo: vdvdb v2 */
		rx_pr("vsvdb v2 todo\n");
	}
}

static void get_edid_colorimetry_data(unsigned char *buff,
				      unsigned char start,
				      unsigned char len,
				      struct cta_blk_parse_info *edid_info)
{
	if (!buff || !edid_info)
		return;
	/* colorimetry DB only contain 3 bytes data block */
	if (len != 3 - 1) {
		rx_pr("invalid length for colorimetry data block:%d\n",
		      len);
		return;
	}
	edid_info->contain_cdb = true;
	edid_info->color_db.BT2020_RGB = (buff[start] >> 7) & 0x1;
	edid_info->color_db.BT2020_YCC = (buff[start] >> 6) & 0x1;
	edid_info->color_db.bt2020_cycc = (buff[start] >> 5) & 0x1;
	edid_info->color_db.adobe_rgb = (buff[start] >> 4) & 0x1;
	edid_info->color_db.adobe_ycc601 = (buff[start] >> 3) & 0x1;
	edid_info->color_db.sycc601 = (buff[start] >> 2) & 0x1;
	edid_info->color_db.xvycc709 = (buff[start] >> 1) & 0x1;
	edid_info->color_db.xvycc601 = buff[start] & 0x1;

	edid_info->color_db.MD3 = (buff[start + 1] >> 3) & 0x1;
	edid_info->color_db.MD2 = (buff[start + 1] >> 2) & 0x1;
	edid_info->color_db.MD1 = (buff[start + 1] >> 1) & 0x1;
	edid_info->color_db.MD0 = buff[start + 1] & 0x1;
}

static void get_hdr_data(unsigned char *buff,
			 unsigned char start,
			 unsigned char len,
			 struct cta_blk_parse_info *edid_info)
{
	if (!buff || !edid_info)
		return;
	/* Bytes 5-7 are optional to declare. 3 bytes payload at least */
	if (len < 3 - 1) {
		rx_pr("invalid hdr length: %d!\n", len);
		return;
	}
	edid_info->contain_hdr_db = true;
	edid_info->hdr_db.eotf_hlg = (buff[start] >> 3) & 0x1;
	edid_info->hdr_db.eotf_smpte_st_2084 = (buff[start] >> 2) & 0x1;
	edid_info->hdr_db.eotf_hdr = (buff[start] >> 1) & 0x1;
	edid_info->hdr_db.eotf_sdr = buff[start] & 0x1;
	edid_info->hdr_db.hdr_SMD_type1 = buff[start + 1] & 0x1;
	if (len > 2)
		edid_info->hdr_db.hdr_lum_max = buff[start + 2];
	if (len > 3)
		edid_info->hdr_db.hdr_lum_avg = buff[start + 3];
	if (len > 4)
		edid_info->hdr_db.hdr_lum_min = buff[start + 4];
}

static void get_edid_y420_vid_data(unsigned char *buff,
				   unsigned char start,
				   unsigned char len,
				   struct cta_blk_parse_info *edid_info)
{
	int i;

	if (!buff || !edid_info)
		return;
	if (len > 6) {
		rx_pr("y420vdb support only 4K50/60hz, smpte50/60hz, len:%d\n",
		      len);
		return;
	}
	edid_info->contain_y420_vdb = true;
	edid_info->y420_vic_len = len;
	for (i = 0; i < len; i++)
		edid_info->y420_vdb_vic[i] = buff[start + i];
}

static void get_edid_y420_cap_map_data(unsigned char *buff,
				       unsigned char start,
				       unsigned char len,
				       struct cta_blk_parse_info *edid_info)
{
	unsigned int i, bit_map = 0;

	if (!buff || !edid_info)
		return;
	/* 31 SVD maximum, 4 bytes needed */
	if (len > 4) {
		rx_pr("31 SVD maximum, all-zero data not needed\n");
		len = 4;
	}
	edid_info->contain_y420_cmdb = true;
	/* When the Length field is set to L==1, the Y420_CMDB does not
	 * include a YCBCR 4:2:0 Capability Bit Map and all the SVDs in
	 * the regular Video Data Block support YCBCR 4:2:0 sampling mode.
	 */
	if (len == 0) {
		edid_info->y420_all_vic = 1;
		return;
	}
	/* bit0: first SVD, bit 1:the second SVD, and so on */
	for (i = 0; i < len; i++)
		bit_map |= (buff[start + i] << (i * 8));
	/* '1' bit in the bit map indicate corresponding SVD support Y420 */
	for (i = 0; (i < len * 8) && (i < 31); i++) {
		if ((bit_map >> i) & 0x1) {
			edid_info->y420_cmdb_vic[i] =
				edid_info->video_db.hdmi_vic[i];
		}
	}
}

static void get_edid_vsadb(unsigned char *buff,
			   unsigned char start,
			   unsigned char len,
			   struct cta_blk_parse_info *edid_info)
{
	int i;

	if (!buff || !edid_info)
		return;
	if (len > 27 - 2) {
		/* CTA-861-G 7.5.8 */
		rx_pr("over VSADB max size(27 bytes), len:%d\n", len);
		return;
	} else if (len < 3) {
		rx_pr("no valid IEEE OUI, len:%d\n", len);
		return;
	}
	edid_info->contain_vsadb = true;
	edid_info->vsadb_ieee =
		buff[start] |
		(buff[start + 1] << 8) |
		(buff[start + 2] << 16);
	for (i = 0; i < len - 3; i++)
		edid_info->vsadb_payload[i] = buff[start + 3 + i];
}

static void get_edid_rcdb(unsigned char *buff,
			  unsigned char start,
			  unsigned char len,
			  struct cta_blk_parse_info *edid_info)
{
}

static void get_edid_sldb(unsigned char *buff,
			  unsigned char start,
			  unsigned char len,
			  struct cta_blk_parse_info *edid_info)
{
}

/* parse multi block data buff */
void parse_cta_data_block(u8 *p_blk_buff,
			  u8 buf_len,
			  struct cta_blk_parse_info *blk_parse_info)
{
	unsigned char tag_data, tag_code, extend_tag_code;
	unsigned char data_blk_len;
	unsigned char index = 0;
	unsigned char i = 0;
	int ret;

	if (!p_blk_buff || !blk_parse_info || buf_len == 0)
		return;

	while (index < buf_len && i < DATA_BLK_MAX_NUM) {
		tag_data = p_blk_buff[index];
		tag_code = (tag_data & 0xE0) >> 5;
		/* data block payload length */
		data_blk_len = tag_data & 0x1F;
		/* length beyond max offset, force to break */
		if ((index + data_blk_len + 1) > buf_len) {
			rx_pr("unexpected data blk len\n");
			break;
		}
		blk_parse_info->db_info[i].cta_blk_index = i;
		blk_parse_info->db_info[i].tag_code = tag_code;
		blk_parse_info->db_info[i].offset = index;
		/* length including header */
		blk_parse_info->db_info[i].blk_len = data_blk_len + 1;
		blk_parse_info->data_blk_num = i + 1;
		switch (tag_code) {
		/* data block payload offset: index+1
		 * length: payload length
		 */
		case VIDEO_TAG:
			get_edid_video_data(p_blk_buff,
					    index + 1,
					    data_blk_len,
					    blk_parse_info);
			break;
		case AUDIO_TAG:
			get_edid_audio_data(p_blk_buff,
					    index + 1,
					    data_blk_len,
					    blk_parse_info);
			break;
		case SPEAKER_TAG:
			get_edid_speaker_data(p_blk_buff,
					      index + 1,
					      data_blk_len,
					      blk_parse_info);
			break;
		case VENDOR_TAG:
			ret = get_edid_vsdb(p_blk_buff,
					    index + 1,
					    data_blk_len,
					    blk_parse_info);
			if (ret == 2)
				blk_parse_info->db_info[i].tag_code =
					tag_code + HF_VSDB_OFFSET;
			else if (ret == 0)
				rx_pr("illegal VSDB\n");
			break;
		case VDTCDB_TAG:
			break;
		case USE_EXTENDED_TAG:
			extend_tag_code = p_blk_buff[index + 1];
			blk_parse_info->db_info[i].tag_code =
				(USE_EXTENDED_TAG << 8) | extend_tag_code;
			switch (extend_tag_code) {
			/* offset: start after extended tag code
			 * length: payload length except extend tag
			 */
			case VCDB_TAG:
				get_edid_vcdb(p_blk_buff,
					      index + 2,
					      data_blk_len - 1,
					      blk_parse_info);
				break;
			case VSVDB_TAG:
				get_edid_dv_data(p_blk_buff,
						 index + 2,
						 data_blk_len - 1,
						 blk_parse_info);
				break;
			case CDB_TAG:
				get_edid_colorimetry_data(p_blk_buff,
							  index + 2,
							  data_blk_len - 1,
							  blk_parse_info);
				break;
			case HDR_STATIC_TAG:
				get_hdr_data(p_blk_buff,
					     index + 2,
					     data_blk_len - 1,
					     blk_parse_info);
				break;
			case Y420VDB_TAG:
				get_edid_y420_vid_data(p_blk_buff, index + 2,
						       data_blk_len - 1,
						       blk_parse_info);
				break;
			case Y420CMDB_TAG:
				get_edid_y420_cap_map_data(p_blk_buff,
							   index + 2,
							   data_blk_len - 1,
							   blk_parse_info);
				break;
			case VSADB_TAG:
				get_edid_vsadb(p_blk_buff, index + 2,
					       data_blk_len - 1, blk_parse_info);
				break;
			case RCDB_TAG:
				get_edid_rcdb(p_blk_buff, index + 2,
					      data_blk_len - 1, blk_parse_info);
				break;
			case SLDB_TAG:
				get_edid_sldb(p_blk_buff, index + 2,
					      data_blk_len - 1, blk_parse_info);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		/* next tag offset */
		index += (data_blk_len + 1);
		i++;
	}
}

/* parse CEA extension block */
void edid_parse_cea_ext_block(u8 *p_edid,
			      struct cea_ext_parse_info *edid_info)
{
	unsigned int max_offset;
	unsigned int blk_start_offset;
	unsigned char data_blk_total_len;
	unsigned char i;

	if (!p_edid || !edid_info)
		return;
	if (p_edid[0] != 0x02) {
		rx_pr("not a valid CEA block!\n");
		return;
	}
	edid_info->cea_tag = p_edid[0];
	edid_info->cea_revision = p_edid[1];
	max_offset = p_edid[2];
	edid_info->dtd_offset = max_offset;
	edid_info->underscan_sup = (p_edid[3] >> 7) & 0x1;
	edid_info->basic_aud_sup = (p_edid[3] >> 6) & 0x1;
	edid_info->ycc444_sup = (p_edid[3] >> 5) & 0x1;
	edid_info->ycc422_sup = (p_edid[3] >> 4) & 0x1;
	edid_info->native_dtd_num = p_edid[3] & 0xF;

	blk_start_offset = 4;
	data_blk_total_len = max_offset - blk_start_offset;
	parse_cta_data_block(&p_edid[blk_start_offset],
			     data_blk_total_len,
			     &edid_info->blk_parse_info);

	for (i = 0; i < edid_info->blk_parse_info.data_blk_num; i++)
		edid_info->blk_parse_info.db_info[i].offset += blk_start_offset;
}

void rx_edid_parse(u8 *p_edid, struct edid_info_s *edid_info)
{
	if (!p_edid || !edid_info)
		return;
	edid_parse_block0(p_edid, edid_info);
	edid_parse_cea_ext_block(p_edid + EDID_EXT_BLK_OFF,
				 &edid_info->cea_ext_info);

	edid_info->free_size =
		rx_edid_free_size(p_edid, 256);
	edid_info->total_free_size =
		rx_edid_total_free_size(p_edid, 256);
	edid_info->dtd_size =
		rx_get_cea_dtd_size(p_edid, 256);
}

void rx_parse_blk0_print(struct edid_info_s *edid_info)
{
	if (!edid_info)
		return;
	rx_pr("****EDID Basic Block****\n");
	rx_pr("manufacturer_name: %s\n", edid_info->manufacturer_name);
	rx_pr("product code: 0x%04x\n", edid_info->product_code);
	rx_pr("serial_number: 0x%08x\n", edid_info->serial_number);
	rx_pr("product week: %d\n", edid_info->product_week);
	rx_pr("product year: %d\n", edid_info->product_year);
	rx_pr("descriptor1.hactive: %d\n", edid_info->descriptor1.hactive);
	rx_pr("descriptor1.vactive: %d\n", edid_info->descriptor1.vactive);
	rx_pr("descriptor1.pix clk: %dKHz\n",
	      edid_info->descriptor1.pixel_clk * 10);
	rx_pr("monitor name: %s\n", edid_info->monitor_name);
	rx_pr("max support pixel clock: %dMhz\n",
	      edid_info->max_sup_pixel_clk);
	rx_pr("extension_flag: %d\n", edid_info->extension_flag);
	rx_pr("block0_chk_sum: 0x%x\n", edid_info->block0_chk_sum);
}

void rx_parse_print_vdb(struct video_db_s *video_db)
{
	unsigned char i;
	unsigned char hdmi_vic;

	if (!video_db)
		return;
	rx_pr("****Video Data Block****\n");
	rx_pr("support SVD list:\n");
	for (i = 0; i < video_db->svd_num; i++) {
		hdmi_vic = video_db->hdmi_vic[i];
		rx_edid_print_vic_fmt(i, hdmi_vic);
	}
}

void rx_parse_print_adb(struct audio_db_s *audio_db)
{
	enum edid_audio_format_e fmt;
	union bit_rate_u *bit_rate;

	if (!audio_db)
		return;
	rx_pr("****Audio Data Block****\n");
	for (fmt = AUDIO_FORMAT_LPCM; fmt <= AUDIO_FORMAT_WMAPRO; fmt++) {
		if (audio_db->aud_fmt_sup[fmt]) {
			rx_pr("audio fmt: %s\n", aud_fmt[fmt]);
			rx_pr("\tmax channel: %d\n",
			      audio_db->sad[fmt].max_channel + 1);
			if (audio_db->sad[fmt].usr.ssr.freq_192khz)
				rx_pr("\tfreq_192khz\n");
			if (audio_db->sad[fmt].usr.ssr.freq_176_4khz)
				rx_pr("\tfreq_176.4khz\n");
			if (audio_db->sad[fmt].usr.ssr.freq_96khz)
				rx_pr("\tfreq_96khz\n");
			if (audio_db->sad[fmt].usr.ssr.freq_88_2khz)
				rx_pr("\tfreq_88.2khz\n");
			if (audio_db->sad[fmt].usr.ssr.freq_48khz)
				rx_pr("\tfreq_48khz\n");
			if (audio_db->sad[fmt].usr.ssr.freq_44_1khz)
				rx_pr("\tfreq_44.1khz\n");
			if (audio_db->sad[fmt].usr.ssr.freq_32khz)
				rx_pr("\tfreq_32khz\n");
			bit_rate = &audio_db->sad[fmt].bit_rate;
			if (fmt == AUDIO_FORMAT_LPCM) {
				rx_pr("sample size:\n");
				if (bit_rate->pcm.size_16bit)
					rx_pr("\t16bit\n");
				if (bit_rate->pcm.size_20bit)
					rx_pr("\t20bit\n");
				if (bit_rate->pcm.size_24bit)
					rx_pr("\t24bit\n");
			} else if ((fmt >= AUDIO_FORMAT_AC3) &&
				(fmt <= AUDIO_FORMAT_ATRAC)) {
				rx_pr("\tmax bit rate: %dkHz\n",
				      bit_rate->others * 8);
			} else {
				rx_pr("\tformat dependent value: 0x%x\n",
				      bit_rate->others);
			}
		}
	}
}

/* may need extend spker alloc */
void rx_parse_print_spk_alloc(struct speaker_alloc_db_s *spk_alloc)
{
	if (!spk_alloc)
		return;
	rx_pr("****Speaker Allocation Data Block****\n");
	if (spk_alloc->flw_frw)
		rx_pr("FLW/FRW\n");
	if (spk_alloc->rlc_rrc)
		rx_pr("RLC/RRC\n");
	if (spk_alloc->flc_frc)
		rx_pr("FLC/FRC\n");
	if (spk_alloc->rc)
		rx_pr("RC\n");
	if (spk_alloc->rl_rr)
		rx_pr("RL/RR\n");
	if (spk_alloc->fc)
		rx_pr("FC\n");
	if (spk_alloc->lfe)
		rx_pr("LFE\n");
	if (spk_alloc->fl_fr)
		rx_pr("FL/FR\n");
	if (spk_alloc->fch)
		rx_pr("FCH\n");
	if (spk_alloc->tc)
		rx_pr("TC\n");
	if (spk_alloc->flh_frh)
		rx_pr("FLH_FRH\n");
}

void rx_parse_print_vsdb(struct cta_blk_parse_info *edid_info)
{
	unsigned char i;
	unsigned char hdmi_vic;
	unsigned char svd_num;
	unsigned char _2d_vic_order;
	unsigned char _3d_struct;
	unsigned char _3d_detail;

	if (!edid_info)
		return;

	svd_num = edid_info->video_db.svd_num;
	rx_pr("****Vender Specific Data Block****\n");
	rx_pr("IEEE OUI: %06X%x%xma\n",
	      edid_info->vsdb.ieee_third,
	      edid_info->vsdb.ieee_second,
	      edid_info->vsdb.ieee_first);
	rx_pr("phy addr: %d.%d.%d.%d\n",
	      edid_info->vsdb.a, edid_info->vsdb.b,
	      edid_info->vsdb.c, edid_info->vsdb.d);
	if (edid_info->vsdb.support_ai)
		rx_pr("support AI\n");
	rx_pr("support deep clor:\n");
	if (edid_info->vsdb.dc_48bit)
		rx_pr("\t16bit\n");
	if (edid_info->vsdb.dc_36bit)
		rx_pr("\t12bit\n");
	if (edid_info->vsdb.dc_30bit)
		rx_pr("\t10bit\n");
	if (edid_info->vsdb.dvi_dual)
		rx_pr("support dvi dual channel\n");
	if (edid_info->vsdb.max_tmds_clk > 0)
		rx_pr("max tmds clk supported: %dMHz\n",
		      edid_info->vsdb.max_tmds_clk * 5);
	rx_pr("hdmi_video_present: %d\n",
	      edid_info->vsdb.hdmi_video_present);
	rx_pr("Content types:\n");
	if (edid_info->vsdb.cnc3)
		rx_pr("\tcnc3: Game\n");
	if (edid_info->vsdb.cnc2)
		rx_pr("\tcnc2: Cinema\n");
	if (edid_info->vsdb.cnc1)
		rx_pr("\tcnc1: Photo\n");
	if (edid_info->vsdb.cnc0)
		rx_pr("\tcnc0: Graphics(text)\n");
	if (edid_info->vsdb.hdmi_vic_len > 0)
		rx_pr("supported 4k2k format:\n");
	if (edid_info->vsdb.hdmi_4k2k_30hz_sup)
		rx_pr("\thdmi vic1: 4k30hz\n");
	if (edid_info->vsdb.hdmi_4k2k_25hz_sup)
		rx_pr("\thdmi vic2: 4k25hz\n");
	if (edid_info->vsdb.hdmi_4k2k_24hz_sup)
		rx_pr("\thdmi vic3: 4k24hz\n");
	if (edid_info->vsdb.hdmi_smpte_sup)
		rx_pr("\thdmi vic4: smpte\n");
	/* Mandatory 3D format: HDMI1.4 spec page157 */
	if (edid_info->vsdb._3d_present) {
		rx_pr("Basic(Mandatory) 3D formats supported\n");
		rx_pr("Image Size:\n");
		switch (edid_info->vsdb.image_size) {
		case 0:
			rx_pr("\tNo additional information\n");
			break;
		case 1:
			rx_pr("\tOnly indicate correct aspect ratio\n");
			break;
		case 2:
			rx_pr("\tCorrect size: Accurate to 1(cm)\n");
			break;
		case 3:
			rx_pr("\tCorrect size: multiply by 5(cm)\n");
			break;
		default:
			break;
		}
	} else {
		rx_pr("No 3D support\n");
	}
	if (edid_info->vsdb._3d_multi_present == 1) {
		/* For each bit in _3d_struct which is set (=1),
		 * Sink supports the corresponding 3D_Structure
		 * for all of the VICs listed in the first 16
		 * entries in the EDID.
		 */
		rx_pr("General 3D format, on the first 16 SVDs:\n");
		for (i = 0; i < 16; i++) {
			if ((edid_info->vsdb._3d_struct_all >> i) & 0x1)
				rx_pr("\t%s\n",	_3d_structure[i]);
		}
	} else if (edid_info->vsdb._3d_multi_present == 2) {
		/* Where a bit is set (=1), for the corresponding
		 * VIC within the first 16 entries in the EDID,
		 * the Sink indicates 3D support as designate
		 * by the 3D_Structure_ALL_15.0 field.
		 */
		rx_pr("General 3D format, on the SVDs below:\n");
		for (i = 0; i < 16; i++) {
			if ((edid_info->vsdb._3d_struct_all >> i) & 0x1)
				rx_pr("\t%s\n",	_3d_structure[i]);
		}
		rx_pr("For SVDs:\n");
		for (i = 0; (i < svd_num) && (i < 16); i++) {
			hdmi_vic = edid_info->video_db.hdmi_vic[i];
			if ((edid_info->vsdb._3d_mask_15_0 >> i) & 0x1)
				rx_edid_print_vic_fmt(i, hdmi_vic);
		}
	}

	if (edid_info->vsdb._2d_vic_num > 0)
		rx_pr("Specific VIC 3D information:\n");
	for (i = 0; (i < edid_info->vsdb._2d_vic_num) &&
	     (i < svd_num) && (i < 16); i++) {
		_2d_vic_order =
			edid_info->vsdb._2d_vic[i]._2d_vic_order;
		_3d_struct =
			edid_info->vsdb._2d_vic[i]._3d_struct;
		hdmi_vic =
			edid_info->video_db.hdmi_vic[_2d_vic_order];
		rx_edid_print_vic_fmt(_2d_vic_order, hdmi_vic);
		rx_pr("\t\t3d format: %s\n", _3d_structure[_3d_struct]);
		if (_3d_struct >= 0x8 && _3d_struct <= 0xF) {
			_3d_detail =
				edid_info->vsdb._2d_vic[i]._3d_detail;
			rx_pr("\t\t3d_detail: %s\n",
			      _3d_detail_x[_3d_detail]);
		}
	}
}

void rx_parse_print_hf_vsdb(struct hf_vsdb_s *hf_vsdb)
{
	if (!hf_vsdb)
		return;
	rx_pr("****HF-VSDB****\n");
	rx_pr("IEEE OUI: %06X%X%X\n",
	      hf_vsdb->ieee_first,
	      hf_vsdb->ieee_second,
	      hf_vsdb->ieee_third);
	rx_pr("hf-vsdb version: %d\n",
	      hf_vsdb->version);
	rx_pr("max_tmds_rate: %dMHz\n",
	      hf_vsdb->max_tmds_rate * 5);
	rx_pr("scdc_present: %d\n",
	      hf_vsdb->scdc_present);
	rx_pr("rr_cap: %d\n",
	      hf_vsdb->rr_cap);
	rx_pr("cable_status: %d\n",
	      hf_vsdb->cable_status);
	rx_pr("ccbpci: %d\n",
	      hf_vsdb->ccbpci);
	rx_pr("lte_340m_scramble: %d\n",
	      hf_vsdb->lte_340m_scramble);
	rx_pr("independ_view: %d\n",
	      hf_vsdb->independ_view);
	rx_pr("dual_view: %d\n",
	      hf_vsdb->dual_view);
	rx_pr("_3d_osd_disparity: %d\n",
	      hf_vsdb->_3d_osd_disparity);
	//pb4
	rx_pr("max_frl_rate: %d\n",
	      hf_vsdb->max_frl_rate);
	rx_pr("uhd_vic: %d\n",
	      hf_vsdb->uhd_vic);
	rx_pr("48bit 420 endode: %d\n",
	      hf_vsdb->dc_48bit_420);
	rx_pr("36bit 420 endode: %d\n",
	      hf_vsdb->dc_36bit_420);
	rx_pr("30bit 420 endode: %d\n",
	      hf_vsdb->dc_30bit_420);
	//pb5
	rx_pr("qms_tfr_max: %d\n",
	      hf_vsdb->qms_tfr_max);
	rx_pr("qms: %d\n",
	      hf_vsdb->qms);
	rx_pr("m_delta: %d\n",
	      hf_vsdb->m_delta);
	rx_pr("qms_tfr_min: %d\n",
	      hf_vsdb->qms_tfr_min);
	rx_pr("neg_mvrr: %d\n",
	      hf_vsdb->neg_mvrr);
	rx_pr("fva: %d\n",
	      hf_vsdb->fva);
	rx_pr("allm: %d\n",
	      hf_vsdb->allm);
	rx_pr("fapa_start_location: %d\n",
	      hf_vsdb->fapa_start_location);
	//pb6
	rx_pr("vrr_max_hi: %d\n",
	      hf_vsdb->vrr_max_hi);
	rx_pr("vrr_min: %d\n",
	      hf_vsdb->vrr_min);
	//pb7
	rx_pr("vrr_max_lo: %d\n",
	      hf_vsdb->vrr_max_lo);
	//pb8
	rx_pr("dsc_1p2: %d\n",
	      hf_vsdb->dsc_1p2);
	rx_pr("dsc_native_420: %d\n",
	      hf_vsdb->dsc_native_420);
	rx_pr("fapa_end_extended: %d\n",
	      hf_vsdb->fapa_end_extended);
	rx_pr("dsc_all_bpp: %d\n",
	      hf_vsdb->dsc_all_bpp);
	rx_pr("dsc_16bpc: %d\n",
	      hf_vsdb->dsc_16bpc);
	rx_pr("dsc_12bpc: %d\n",
	      hf_vsdb->dsc_12bpc);
	rx_pr("dsc_10bpc: %d\n",
	      hf_vsdb->dsc_10bpc);
	//pb9
	rx_pr("dsc_max_frl_rate: %d\n",
	      hf_vsdb->dsc_max_frl_rate);
	rx_pr("dsc_max_slices: %d\n",
	      hf_vsdb->dsc_max_slices);
	//pb10
	rx_pr("dsc_total_chunk_kbytes: %d\n",
	      hf_vsdb->dsc_total_chunk_kbytes);
}

void rx_parse_print_vcdb(struct video_cap_db_s *vcdb)
{
	if (!vcdb)
		return;
	rx_pr("****Video Cap Data Block****\n");
	rx_pr("YCC Quant Range:\n");
	if (vcdb->quanti_range_ycc)
		rx_pr("\tSelectable(via AVI YQ)\n");
	else
		rx_pr("\tNo Data\n");

	rx_pr("RGB Quant Range:\n");
	if (vcdb->quanti_range_rgb)
		rx_pr("\tSelectable(via AVI Q)\n");
	else
		rx_pr("\tNo Data\n");

	rx_pr("PT Scan behavior:\n");
	switch (vcdb->s_PT) {
	case 0:
		rx_pr("\t transfer to CE/IT fields\n");
		break;
	case 1:
		rx_pr("\t Always Overscanned\n");
		break;
	case 2:
		rx_pr("\t Always Underscanned\n");
		break;
	case 3:
		rx_pr("\t Support both over and underscan\n");
		break;
	default:
		break;
	}
	rx_pr("IT Scan behavior:\n");
	switch (vcdb->s_IT) {
	case 0:
		rx_pr("\tIT video format not support\n");
		break;
	case 1:
		rx_pr("\tAlways Overscanned\n");
		break;
	case 2:
		rx_pr("\tAlways Underscanned\n");
		break;
	case 3:
		rx_pr("\tSupport both over and underscan\n");
		break;
	default:
		break;
	}
	rx_pr("CE Scan behavior:\n");
	switch (vcdb->s_CE) {
	case 0:
		rx_pr("\tCE video format not support\n");
		break;
	case 1:
		rx_pr("\tAlways Overscanned\n");
		break;
	case 2:
		rx_pr("\tAlways Underscanned\n");
		break;
	case 3:
		rx_pr("\tSupport both over and underscan\n");
		break;
	default:
		break;
	}
}

void rx_parse_print_vsvdb(struct dv_vsvdb_s *dv_vsvdb)
{
	if (!dv_vsvdb)
		return;
	rx_pr("****VSVDB(dolby vision)****\n");
	rx_pr("IEEE_OUI: %06X\n",
	      dv_vsvdb->ieee_oui);
	rx_pr("vsvdb version: %d\n",
	      dv_vsvdb->version);
	rx_pr("sup_global_dimming: %d\n",
	      dv_vsvdb->sup_global_dimming);
	rx_pr("sup_2160p60hz: %d\n",
	      dv_vsvdb->sup_2160p60hz);
	rx_pr("sup_yuv422_12bit: %d\n",
	      dv_vsvdb->sup_yuv422_12bit);
	rx_pr("Rx: 0x%x\n", dv_vsvdb->Rx);
	rx_pr("Ry: 0x%x\n", dv_vsvdb->Ry);
	rx_pr("Gx: 0x%x\n", dv_vsvdb->Gx);
	rx_pr("Gy: 0x%x\n", dv_vsvdb->Gy);
	rx_pr("Bx: 0x%x\n", dv_vsvdb->Bx);
	rx_pr("By: 0x%x\n", dv_vsvdb->By);
	if (dv_vsvdb->version == 0) {
		rx_pr("target max pq: 0x%x\n",
		      dv_vsvdb->tmaxPQ);
		rx_pr("target min pq: 0x%x\n",
		      dv_vsvdb->tminPQ);
		rx_pr("dm_major_ver: 0x%x\n",
		      dv_vsvdb->dm_major_ver);
		rx_pr("dm_minor_ver: 0x%x\n",
		      dv_vsvdb->dm_minor_ver);
	} else if (dv_vsvdb->version == 1) {
		rx_pr("DM_version: 0x%x\n",
		      dv_vsvdb->DM_version);
		rx_pr("target_max_lum: 0x%x\n",
		      dv_vsvdb->target_max_lum);
		rx_pr("target_min_lum: 0x%x\n",
		      dv_vsvdb->target_min_lum);
		rx_pr("colorimetry: 0x%x\n",
		      dv_vsvdb->colorimetry);
	}
}

void rx_parse_print_cdb(struct colorimetry_db_s *color_db)
{
	if (!color_db)
		return;
	rx_pr("****Colorimetry Data Block****\n");
	rx_pr("supported colorimetry:\n");
	if (color_db->BT2020_RGB)
		rx_pr("\tBT2020_RGB\n");
	if (color_db->BT2020_YCC)
		rx_pr("\tBT2020_YCC\n");
	if (color_db->bt2020_cycc)
		rx_pr("\tbt2020_cycc\n");
	if (color_db->adobe_rgb)
		rx_pr("\tadobe_rgb\n");
	if (color_db->adobe_ycc601)
		rx_pr("\tadobe_ycc601\n");
	if (color_db->sycc601)
		rx_pr("\tsycc601\n");
	if (color_db->xvycc709)
		rx_pr("\txvycc709\n");
	if (color_db->xvycc601)
		rx_pr("\txvycc601\n");

	rx_pr("supported colorimetry metadata:\n");
	if (color_db->MD3)
		rx_pr("\tMD3\n");
	if (color_db->MD2)
		rx_pr("\tMD2\n");
	if (color_db->MD1)
		rx_pr("\tMD1\n");
	if (color_db->MD0)
		rx_pr("\tMD0\n");
}

void rx_parse_print_hdr_static(struct hdr_db_s *hdr_db)
{
	if (!hdr_db)
		return;
	rx_pr("****HDR Static Metadata Data Block****\n");
	rx_pr("eotf_hlg: %d\n",
	      hdr_db->eotf_hlg);
	rx_pr("eotf_smpte_st_2084: %d\n",
	      hdr_db->eotf_smpte_st_2084);
	rx_pr("eotf_hdr: %d\n",
	      hdr_db->eotf_hdr);
	rx_pr("eotf_sdr: %d\n",
	      hdr_db->eotf_sdr);
	rx_pr("hdr_SMD_type1: %d\n",
	      hdr_db->hdr_SMD_type1);
	if (hdr_db->hdr_lum_max)
		rx_pr("Desired Content Max Luminance: 0x%x\n",
		      hdr_db->hdr_lum_max);
	if (hdr_db->hdr_lum_avg)
		rx_pr("Desired Content Max Frame-avg Luminance: 0x%x\n",
		      hdr_db->hdr_lum_avg);
	if (hdr_db->hdr_lum_min)
		rx_pr("Desired Content Min Luminance: 0x%x\n",
		      hdr_db->hdr_lum_min);
}

void rx_parse_print_y420vdb(struct cta_blk_parse_info *edid_info)
{
	unsigned char i;

	if (!edid_info)
		return;
	rx_pr("****Y420 Video Data Block****\n");
	for (i = 0; i < edid_info->y420_vic_len; i++)
		rx_edid_print_vic_fmt(i, edid_info->y420_vdb_vic[i]);
}

void rx_parse_print_y420cmdb(struct cta_blk_parse_info *edid_info)
{
	unsigned char i;
	unsigned char hdmi_vic;

	if (!edid_info)
		return;
	rx_pr("****Yc420 capability map Data Block****\n");
	if (edid_info->y420_all_vic) {
		rx_pr("all vic support y420\n");
	} else {
		for (i = 0; i < 31; i++) {
			hdmi_vic = edid_info->y420_cmdb_vic[i];
			if (hdmi_vic)
				rx_edid_print_vic_fmt(i, hdmi_vic);
		}
	}
}

void rx_cea_ext_parse_print(struct cea_ext_parse_info *cea_ext_info)
{
	if (!cea_ext_info)
		return;
	rx_pr("****CEA Extension Block Header****\n");
	rx_pr("cea_tag: 0x%x\n", cea_ext_info->cea_tag);
	rx_pr("cea_revision: 0x%x\n", cea_ext_info->cea_revision);
	rx_pr("dtd offset: %d\n", cea_ext_info->dtd_offset);
	rx_pr("underscan_sup: %d\n", cea_ext_info->underscan_sup);
	rx_pr("basic_aud_sup: %d\n", cea_ext_info->basic_aud_sup);
	rx_pr("ycc444_sup: %d\n", cea_ext_info->ycc444_sup);
	rx_pr("ycc422_sup: %d\n", cea_ext_info->ycc422_sup);
	rx_pr("native_dtd_num: %d\n", cea_ext_info->native_dtd_num);

	rx_parse_print_vdb(&cea_ext_info->blk_parse_info.video_db);
	rx_parse_print_adb(&cea_ext_info->blk_parse_info.audio_db);
	rx_parse_print_spk_alloc(&cea_ext_info->blk_parse_info.speaker_alloc);
	rx_parse_print_vsdb(&cea_ext_info->blk_parse_info);
	if (cea_ext_info->blk_parse_info.contain_hf_vsdb)
		rx_parse_print_hf_vsdb(&cea_ext_info->blk_parse_info.hf_vsdb);
	if (cea_ext_info->blk_parse_info.contain_vcdb)
		rx_parse_print_vcdb(&cea_ext_info->blk_parse_info.vcdb);
	if (cea_ext_info->blk_parse_info.contain_cdb)
		rx_parse_print_cdb(&cea_ext_info->blk_parse_info.color_db);
	if (cea_ext_info->blk_parse_info.contain_vsvdb)
		rx_parse_print_vsvdb(&cea_ext_info->blk_parse_info.dv_vsvdb);
	if (cea_ext_info->blk_parse_info.contain_hdr_db)
		rx_parse_print_hdr_static(&cea_ext_info->blk_parse_info.hdr_db);
	if (cea_ext_info->blk_parse_info.contain_y420_vdb)
		rx_parse_print_y420vdb(&cea_ext_info->blk_parse_info);
	if (cea_ext_info->blk_parse_info.contain_y420_cmdb)
		rx_parse_print_y420cmdb(&cea_ext_info->blk_parse_info);
}

void rx_edid_parse_print(struct edid_info_s *edid_info)
{
	if (!edid_info)
		return;
	rx_parse_blk0_print(edid_info);
	rx_cea_ext_parse_print(&edid_info->cea_ext_info);

	rx_pr("CEA ext blk free size: %d\n", edid_info->free_size);
	rx_pr("CEA ext blk total free size(include dtd size): %d\n",
	      edid_info->total_free_size);
	rx_pr("CEA ext blk dtd size: %d\n", edid_info->dtd_size);
}

void rx_data_blk_index_print(struct cta_data_blk_info *db_info)
{
	if (!db_info)
		return;
	rx_pr("%-7d\t 0x%-5X\t %-30s\t 0x%-8X\t %d\n",
	      db_info->cta_blk_index,
	      db_info->tag_code,
	      rx_get_cta_blk_name(db_info->tag_code),
	      db_info->offset,
	      db_info->blk_len);
}

void rx_blk_index_print(struct cta_blk_parse_info *blk_info)
{
	int i;

	if (!blk_info)
		return;
	rx_pr("****CTA Data Block Index****\n");
	rx_pr("%-7s\t %-7s\t %-30s\t %-10s\t %s\n",
	      "blk_idx", "blk_tag", "blk_name",
	      "blk_offset", "blk_len");
	for (i = 0; i < blk_info->data_blk_num; i++)
		rx_data_blk_index_print(&blk_info->db_info[i]);
}

#ifdef CONFIG_AMLOGIC_HDMITX
void rx_edid_physical_addr(int a, int b, int c, int d)
{
	//tx_hpd_event = E_RCV;
	//if (edid_from_tx & 2) {
		//rx_pr("whole edid from tx. no need to update physical addr");
		//return;
	//}
	up_phy_addr = ((d & 0xf) << 12) |
		   ((c & 0xf) <<  8) |
		   ((b & 0xf) <<  4) |
		   ((a & 0xf) <<  0);

	/* if (log_level & EDID_LOG) */
	rx_pr("\nup_phy_addr = %x\n", up_phy_addr);
}
EXPORT_SYMBOL(rx_edid_physical_addr);
#endif

unsigned char rx_get_cea_dtd_size(unsigned char *cur_edid, unsigned int size)
{
	unsigned char dtd_block_offset;
	unsigned char dtd_size = 0;

	if (!cur_edid)
		return 0;
	/* get description offset */
	dtd_block_offset =
		cur_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET];
	dtd_block_offset += EDID_BLOCK1_OFFSET;
	if (log_level & EDID_LOG)
		rx_pr("%s dtd offset start:%d\n", __func__, dtd_block_offset);
	/* dtd first two bytes are pixel clk != 0 */
	while ((dtd_block_offset + 1 < size - 1) &&
	       (cur_edid[dtd_block_offset] ||
		cur_edid[dtd_block_offset + 1])) {
		dtd_block_offset += DETAILED_TIMING_LEN;
		if (dtd_block_offset > size - 1)
			break;
		dtd_size += DETAILED_TIMING_LEN;
	}
	if (log_level & EDID_LOG)
		rx_pr("%s block_start end:%d\n", __func__, dtd_block_offset);

	return dtd_size;
}

/* rx_get_total_free_size
 * get total free size including dtd
 */
unsigned char rx_edid_total_free_size(unsigned char *cur_edid,
				      unsigned int size)
{
	unsigned char dtd_block_offset;

	if (!cur_edid)
		return 0;
	/* get description offset */
	dtd_block_offset =
		cur_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET];
	dtd_block_offset += EDID_BLOCK1_OFFSET;
	if (log_level & EDID_LOG)
		rx_pr("%s total free size: %d\n", __func__,
		      size - dtd_block_offset - 1);
	/* free size except checksum */
	return (size - dtd_block_offset - 1);
}

bool rx_set_earc_cap_ds(unsigned char *data, unsigned int len)
{
	if (rx_info.chip_id == CHIP_ID_NONE)
		return false;
	new_earc_cap_ds = true;
	memset(recv_earc_cap_ds, 0, sizeof(recv_earc_cap_ds));
	if (!data || len > EARC_CAP_DS_MAX_LENGTH)
		return false;

	memcpy(recv_earc_cap_ds, data, len);
	earc_cap_ds_len = len;

	rx_pr("*update earc cap_ds to edid*\n");
	hdmi_rx_top_edid_update();
	/* if currently in arc port, don't reset hpd */
	if (rx_info.main_port_open && rx_info.main_port != rx_info.arc_port) {
		if (earc_cap_ds_update_hpd_en)
			rx_send_hpd_pulse(rx_info.main_port);
	} else {
		pre_port = 0xff;
		rx_pr("update cap_ds later, in ARC port:%s\n",
		      rx_info.main_port == rx_info.arc_port ? "Y" : "N");
	}
	return true;
}
EXPORT_SYMBOL(rx_set_earc_cap_ds);

/* now only consider VSVDB_DV. vsvdb length:
 * v0:26bytes, v1-15:15bytes, v1-12/v2 12bytes
 */
bool rx_set_vsvdb(unsigned char *data, unsigned int len)
{
	/* len = 0, disable/remove VSVDB in edid
	 * len = 0xFF, revert to original VSVDB in edid
	 * len = 12/15/26, replace VSVDB with param[data]
	 */
	if (rx_info.chip_id == CHIP_ID_NONE)
		return false;
	if (len > VSVDB_LEN && len != 0xFF) {
		rx_pr("err: invalid vsvdb length:%d\n", len);
		return false;
	}
	memset(recv_vsvdb, 0, sizeof(recv_vsvdb));
	if (data && len <= VSVDB_LEN)
		memcpy(recv_vsvdb, data, len);
	recv_vsvdb_len = len;
	rx_pr("*update vsvdb by DV, len=%d*\n", len);
	hdmi_rx_top_edid_update();
	if (rx_info.main_port_open) {
		if (vsvdb_update_hpd_en)
			rx_send_hpd_pulse(rx_info.main_port);
	} else {
		pre_port = 0xff;
		rx_pr("update vsvdb later\n");
	}
	return true;
}
EXPORT_SYMBOL(rx_set_vsvdb);

#ifdef CONFIG_AMLOGIC_HDMITX
bool rx_update_tx_edid_with_audio_block(unsigned char *edid_data, unsigned char *audio_block)
{
	if (rx_info.chip_id == CHIP_ID_NONE)
		return false;
	if (!edid_data) {
		memset(edid_tx, 0, EDID_SIZE);
		return false;
	}

	memcpy(edid_tx, edid_data, EDID_SIZE);
	return true;
}
EXPORT_SYMBOL(rx_update_tx_edid_with_audio_block);
#endif

/* extract data block with certain tag from block buf */
u8 *data_blk_extract(u8 *p_buf, u16 tagid)
{
	unsigned int index = 0;
	u8 tag_length;
	u16 tag_code;

	if (!p_buf)
		return NULL;
	//TODO: for 512byte edid
	while (index < EDID_BLK_SIZE) {
		/* Get the tag and length */
		tag_code = rx_get_tag_code(p_buf + index);
		tag_length = BLK_LENGTH(p_buf[index]);
		if (tagid == tag_code)
			return &p_buf[index];
		index += tag_length + 1;
	}
	return NULL;
}

bool get_edid_support(u8 port, enum edid_support_e func)
{
	u32 hf_vsdb_start = 0;
	u8 tag_len = 0;
	u_char *edid_buf = NULL;

	edid_buf = rx_get_cur_used_edid(port);
	if (!edid_buf)
		return false;
	if (func < HF_DB) {
		hf_vsdb_start = rx_get_cea_tag_offset(edid_buf, HF_VENDOR_DB_TAG);
		if (!hf_vsdb_start)
			return false;
		tag_len = edid_buf[hf_vsdb_start] & 0x1f;
	}
	switch (func) {
	case HF_VRR:
		if (tag_len < 9)
			return false;
		else
			return true;
		break;
	case HF_ALLM:
		if (tag_len < 8)
			return false;
		if (edid_buf[hf_vsdb_start  + 8] & 0x2)
			return true;
		break;
	case HF_DB:
		if (data_blk_extract(edid_buf + EDID_BLK_SIZE + 4, HF_VENDOR_DB_TAG))
			return true;
		break;
	case DV_DB:
		if (data_blk_extract(edid_buf + EDID_BLK_SIZE + 4, VSVDB_DV_TAG))
			return true;
		break;
	case HDR10P_DB:
		if (data_blk_extract(edid_buf + EDID_BLK_SIZE + 4, VSVDB_HDR10P_TAG))
			return true;
		break;
	case HDR_STATIC_DB:
		if (data_blk_extract(edid_buf + EDID_BLK_SIZE + 4, HDR_STATIC_TAG))
			return true;
		break;
	case HDR_DYNAMIC_DB:
		if (data_blk_extract(edid_buf + EDID_BLK_SIZE + 4, HDR_DYNAMIC_TAG))
			return true;
		break;
	case FREESYNC_DB:
		if (data_blk_extract(edid_buf + EDID_BLK_SIZE + 4, VSDB_FREESYNC_TAG))
			return true;
		break;
	default:
		break;
	}
	return false;
}

/* combine short audio descriptors,
 * see FigureH-3 of HDMI2.1 SPEC
 */
unsigned char *compose_audio_db(u8 *aud_db, u8 *add_buf)
{
	u8 aud_db_len;
	u8 add_buf_len;
	u8 payload_len;
	u8 tmp_aud[DB_LEN_MAX - 1] = {0};
	u8 *tmp_buf = add_buf;

	u8 i, j;
	u8 idx = 1;
	enum edid_audio_format_e aud_fmt;
	enum edid_audio_format_e tmp_fmt;

	if (!aud_db || !add_buf)
		return NULL;
	memset(com_aud, 0, sizeof(com_aud));
	aud_db_len = BLK_LENGTH(aud_db[0]) + 1;
	add_buf_len = BLK_LENGTH(add_buf[0]) + 1;

	for (i = 1; (i < aud_db_len) && (idx < DB_LEN_MAX - 1); i += SAD_LEN) {
		aud_fmt = (aud_db[i] & 0x78) >> 3;
		for (j = 1; j < add_buf_len; j += SAD_LEN) {
			tmp_fmt = (tmp_buf[j] & 0x78) >> 3;
			if (aud_fmt == tmp_fmt)
				break;
		}
		/* copy this audio data to payload */
		if (j == add_buf_len) {
			/* not find this fmt in add_buf */
			memcpy(com_aud + idx, aud_db + i, SAD_LEN);
		} else {
			/* find this fmt in add_buf */
			memcpy(com_aud + idx, tmp_buf + j, SAD_LEN);
			/* delete this fmt from add_buf */
			memcpy(tmp_aud + 1, tmp_buf + 1, j - 1);
			memcpy(tmp_aud + j, tmp_buf + j + SAD_LEN,
			       add_buf_len - j - SAD_LEN);
			add_buf_len -= SAD_LEN;
			tmp_buf = tmp_aud;
		}
		idx += SAD_LEN;
	}
	/* copy remain Short Audio Descriptors
	 * in add_buf, except blk header
	 */
	if (idx < sizeof(com_aud))
		if (idx + add_buf_len - 1 <= sizeof(com_aud))
			memcpy(com_aud + idx, tmp_buf + 1, add_buf_len - 1);
	payload_len = (idx - 1) + (add_buf_len - 1);
	/* data blk header */
	com_aud[0] = (AUDIO_TAG << 5) | payload_len;

	if (log_level & EDID_DATA_LOG) {
		rx_pr("++++after compose, audio data blk:\n");
		for (i = 0; i < payload_len + 1; i++)
			rx_pr("%02x", com_aud[i]);
		rx_pr("\n");
	}
	return com_aud;
}

/* param[add_buf]: contain only one data blk.
 * param[blk_idx]: sequence number of the data blk
 * 1.if the data blk is not present in edid, then
 * add this data blk according to add_blk_idx,
 * otherwise compose and replace this data blk.
 * 2.if blk_idx is 0xFF, then add this data blk
 * after the last data blk, otherwise insert it
 * in the blk_idx place.
 */
void splice_data_blk_to_edid(u_char *p_edid, u_char *add_buf,
			     u_char blk_idx)
{
	/* u8 *tag_data_blk = NULL; */
	u8 *add_data_blk = NULL;
	u8 tag_db_len = 0;
	u8 add_db_len = 0;
	u8 diff_len = 0;
	u16 tag_code = 0;
	u8 tag_offset = 0;

	int free_size = 0;
	u8 total_free_size = 0;
	u8 dtd_size = 0;
	u8 free_space_off;
	u32 i = 0;
	struct cea_ext_parse_info cea_ext;
	u8 num;

	if (!p_edid || !add_buf)
		return;

	free_size = rx_edid_free_size(p_edid, 256);
	if (free_size < 0) {
		rx_pr("wrong edid\n");
		return;
	}
	total_free_size =
		rx_edid_total_free_size(p_edid, 256);
	dtd_size = rx_get_cea_dtd_size(p_edid, 256);

	tag_code = (add_buf[0] >> 5) & 0x7;
	if (tag_code == USE_EXTENDED_TAG)
		tag_code = (tag_code << 8) | add_buf[1];
	/* tag_data_blk = edid_tag_extract(p_edid, tag_code); */
	tag_offset = rx_get_ceadata_offset(p_edid, add_buf);

	/* if (!tag_data_blk) { */
	if (tag_offset == 0) {
		/* not find the data blk, add it to edid */
		add_db_len = BLK_LENGTH(add_buf[0]) + 1;
		edid_parse_cea_ext_block(p_edid + EDID_EXT_BLK_OFF,
					 &cea_ext);
		/* decide to add db after which data blk */
		num = cea_ext.blk_parse_info.data_blk_num;

		if (blk_idx >= num)
			/* add after the last data blk */
			tag_offset =
				cea_ext.blk_parse_info.db_info[num - 1].offset +
				cea_ext.blk_parse_info.db_info[num - 1].blk_len;
		else
			/* insert in blk_idx */
			tag_offset =
				cea_ext.blk_parse_info.db_info[blk_idx].offset;
		tag_offset += EDID_BLOCK1_OFFSET;
		if (add_db_len <= free_size) {
			/* move data behind added data block, except checksum */
			for (i = 0; i < 255 - tag_offset - add_db_len; i++)
				p_edid[255 - i - 1] =
					p_edid[255 - i - 1 - add_db_len];
		} else if (en_take_dtd_space) {
			if (add_db_len > total_free_size) {
				rx_pr("no enough space for splicing3, abort\n");
				return;
			}
			/* actually, total free size = free_size + dtd_size,
			 * add_db_len won't excess 32bytes, so may excess
			 * one dtd length, but mustn't excess 2 dtd length.
			 * need clear the replaced dtd to 0
			 */
			if (add_db_len <= free_size + DETAILED_TIMING_LEN) {
				free_space_off =
					255 - free_size - DETAILED_TIMING_LEN;
				memset(&p_edid[free_space_off], 0,
				       DETAILED_TIMING_LEN);
			} else {
				free_space_off =
					255 - free_size - 2 * DETAILED_TIMING_LEN;
				memset(&p_edid[free_space_off], 0,
				       2 * DETAILED_TIMING_LEN);
			}
			/* move data behind current data
			 * block, except checksum
			 */
			for (i = 0; i < 255 - tag_offset - add_db_len; i++)
				p_edid[255 - i - 1] =
					p_edid[255 - i - 1 - add_db_len];
		} else {
			rx_pr("no enough space for splicing4, abort\n");
			return;
		}
		/* dtd offset modify */
		p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] +=
			add_db_len;
		/* copy added data block */
		memcpy(p_edid + tag_offset, add_buf, add_db_len);
	} else {
		/* tag_db_len = BLK_LENGTH(tag_data_blk[0])+1; */
		tag_db_len = BLK_LENGTH(p_edid[tag_offset]) + 1;
		add_data_blk = add_buf;
		/* compose and replace data blk */
		if (tag_code == AUDIO_TAG) {
			/* compose audio data blk */
			add_data_blk =
				compose_audio_db(&p_edid[tag_offset], add_buf);
				/* compose_audio_db(tag_data_blk, add_buf); */
		}
		/* replace data blk */
		if (!add_data_blk)
			return;
		add_db_len = BLK_LENGTH(add_data_blk[0]) + 1;
		if (tag_db_len >= add_db_len) {
			/* move data behind current data
			 * block, except checksum
			 */
			for (i = 0; i < 255 - tag_offset - tag_db_len; i++)
				p_edid[tag_offset + i + add_db_len] =
					p_edid[tag_offset + i + tag_db_len];
			/* need clear the new free space to 0 */
			free_size += (tag_db_len - add_db_len);
			free_space_off = 255 - free_size;
			memset(&p_edid[free_space_off], 0, free_size);
		} else if (add_db_len - tag_db_len <= free_size) {
			/* move data behind current data
			 * block, except checksum
			 */
			for (i = 0; i < 255 - tag_offset - add_db_len; i++)
				p_edid[255 - i - 1] =
					p_edid[255 - i - 1 - (add_db_len - tag_db_len)];
		} else if (en_take_dtd_space) {
			diff_len = add_db_len - tag_db_len;
			if (diff_len > total_free_size) {
				rx_pr("no enough space for splicing, abort\n");
				return;
			}
			/* actually, total free size = free_size + dtd_size,
			 * diff_len won't excess 31bytes, so may excess
			 * one dtd length, but mustn't excess 2 dtd length.
			 * need clear the replaced dtd to 0
			 */
			if (diff_len <= free_size + DETAILED_TIMING_LEN) {
				free_space_off =
					255 - free_size - DETAILED_TIMING_LEN;
				memset(&p_edid[free_space_off], 0,
				       DETAILED_TIMING_LEN);
			} else {
				free_space_off =
					255 - free_size - 2 * DETAILED_TIMING_LEN;
				memset(&p_edid[free_space_off], 0,
				       2 * DETAILED_TIMING_LEN);
			}
			/* move data behind current data
			 * block, except checksum
			 */
			for (i = 0; i < 255 - tag_offset - add_db_len; i++)
				p_edid[255 - i - 1] =
					p_edid[255 - i - 1 - (add_db_len - tag_db_len)];
		} else {
			rx_pr("no enough space for splicing2, abort\n");
			return;
		}
		/* dtd offset modify */
		p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] +=
			(add_db_len - tag_db_len);
		/* copy added data block */
		memcpy(p_edid + tag_offset, add_data_blk, add_db_len);
	}
	if (log_level & EDID_DATA_LOG) {
		rx_pr("++++edid data after splice:\n");
		for (i = 128; i < 256; i++)
			pr_cont("%02x", p_edid[i]);
		rx_pr("\n");
	}
}

/* param[add_buf] may contain multi data blk,
 * search the blk filtered by tag, and then
 * splice it with edid
 */
void splice_tag_db_to_edid(u8 *p_edid, u8 *add_buf,
			   u16 tagid)
{
	u8 *tag_data_blk = NULL;
	unsigned int i;

	if (!p_edid || !add_buf)
		return;
	tag_data_blk = data_blk_extract(add_buf, tagid);
	if (!tag_data_blk) {
		rx_pr("no this data blk in add buf\n");
		return;
	}
	if (log_level & EDID_LOG) {
		rx_pr("++++extracted data blk(tag=0x%x):\n", tagid);
		for (i = 0; i < BLK_LENGTH(tag_data_blk[0]) + 1; i++)
			pr_cont("%02x", tag_data_blk[i]);
		rx_pr("\n");
	}
	/* if db not exist in edid, then add it to the end */
	splice_data_blk_to_edid(p_edid, tag_data_blk, 0xFF);
}

/* remove cta data blk which tag = tagid */
void edid_rm_db_by_tag(u8 *p_edid, u16 tagid)
{
	int tag_offset;
	unsigned char tag_len;
	unsigned int i;

	if (!p_edid)
		return;
	tag_offset = rx_get_cea_tag_offset(p_edid, tagid);
	if (!tag_offset) {
		rx_pr("no this data blk in edid\n");
		return;
	}
	tag_len = BLK_LENGTH(p_edid[tag_offset]) + 1;
	for (i = tag_offset; i < 255 - tag_len; i++)
		p_edid[i] = p_edid[i + tag_len];
	/* clear new free space to zero */
	memset(&p_edid[i + 1], 0, tag_len);
	/* dtd offset modify */
	p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -= tag_len;
	if (log_level & EDID_DATA_LOG) {
		rx_pr("++++edid data after rm db by tag:\n");
		for (i = 128; i < 256; i++)
			pr_cont("%02x", p_edid[i]);
		rx_pr("\n");
	}
}

/* param[blk_idx]: start from 0
 * the sequence index of the data blk to be removed
 */
void edid_rm_db_by_idx(u8 *p_edid, u8 blk_idx)
{
	struct cea_ext_parse_info ext_blk_info;
	int tag_offset;
	unsigned char tag_len;
	unsigned int i;

	if (!p_edid)
		return;
	edid_parse_cea_ext_block(p_edid + EDID_EXT_BLK_OFF,
				 &ext_blk_info);
	if (blk_idx >= ext_blk_info.blk_parse_info.data_blk_num) {
		rx_pr("no this index data blk in edid\n");
		return;
	}
	tag_offset = EDID_EXT_BLK_OFF +
		ext_blk_info.blk_parse_info.db_info[blk_idx].offset;
	tag_len =
		ext_blk_info.blk_parse_info.db_info[blk_idx].blk_len;
	/* move data behind the removed data block
	 * forward, except checksum & free size
	 */
	for (i = tag_offset; i < 255 - tag_len; i++)
		p_edid[i] = p_edid[i + tag_len];
	/* clear new free space to zero */
	memset(&p_edid[i + 1], 0, tag_len);
	/* dtd offset modify */
	p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -= tag_len;
	if (log_level & EDID_DATA_LOG) {
		rx_pr("++++edid data after rm db by idx:\n");
		for (i = 128; i < 256; i++)
			pr_cont("%02x", p_edid[i]);
		rx_pr("\n");
	}
}

#ifdef CONFIG_AMLOGIC_HDMITX
static void rpt_edid_extension_num_extraction(unsigned char *p_edid)
{
	u_int i;
	u_int checksum = 0;

	if (!p_edid)
		return;
	if (log_level & EDID_LOG)
		rx_pr("extension_block_num = %d\n", p_edid[126]);
	/* currently we only handle 2 EDID blocks, ignore > 2 blocks */
	if (p_edid[0x7e] > 1) {
		p_edid[0x7e] = 1;
		for (i = 0; i <= 0x7F; i++) {
			if (i < 0x7F) {
				checksum += p_edid[i];
				checksum &= 0xFF;
			} else {
				checksum = (0x100 - checksum) & 0xFF;
			}
		}
		p_edid[0x7f] = checksum & 0xFF;
	}
}

static void rpt_edid_rm_hf_eeodb(unsigned char *p_edid)
{
	/* HF-EEODB occupy byte 4~6 of block1, fixed location */
	edid_rm_db_by_tag(p_edid, EXTENDED_HF_EEODB);
}

void rpt_edid_hf_vs_db_extraction(unsigned char *p_edid)
{
	u8 hf_vsdb_start = 0;
	u8 tx_hf_vsdb_start = 0;
	struct hf_vsdb_s hf_vsdb_tx, hf_vsdb_def, hf_vsdb_dest;
	u8 tag_len_tx, tag_len_def, i;

	if (!p_edid)
		return;

	hf_vsdb_start = rx_get_cea_tag_offset(p_edid, HF_VENDOR_DB_TAG);
	if (!hf_vsdb_start) {
		rx_pr("unsupported hf-vsdb!!!\n");
		return;
	}
	tag_len_def = p_edid[hf_vsdb_start] & 0x1f;
	memcpy(&hf_vsdb_def, p_edid + hf_vsdb_start, tag_len_def + 1);

	tx_hf_vsdb_start = rx_get_cea_tag_offset(edid_tx, HF_VENDOR_DB_TAG);
	if (!tx_hf_vsdb_start) {
		if (log_level & EDID_LOG)
			rx_pr("no hf-vsdb\n");
		edid_rm_db_by_tag(p_edid, HF_VENDOR_DB_TAG);
		return;
	}
	tag_len_tx = edid_tx[tx_hf_vsdb_start] & 0x1f;
	memcpy(&hf_vsdb_tx, edid_tx + tx_hf_vsdb_start, tag_len_tx + 1);

	hf_vsdb_dest.tag = hf_vsdb_tx.tag;
	hf_vsdb_dest.version = hf_vsdb_tx.version;
	hf_vsdb_dest.len = tag_len_tx > tag_len_def ? tag_len_def : tag_len_tx;
	if (hf_vsdb_dest.len < 7)
		rx_pr("hfvsdb size err\n");
	hf_vsdb_dest.ieee_first = hf_vsdb_tx.ieee_first;
	hf_vsdb_dest.ieee_second = hf_vsdb_tx.ieee_second;
	hf_vsdb_dest.ieee_third = hf_vsdb_tx.ieee_third;
	if (hf_vsdb_def.max_tmds_rate > hf_vsdb_tx.max_tmds_rate)
		hf_vsdb_dest.max_tmds_rate = hf_vsdb_tx.max_tmds_rate;
	else
		hf_vsdb_dest.max_tmds_rate = hf_vsdb_def.max_tmds_rate;

	hf_vsdb_dest.scdc_present = hf_vsdb_tx.scdc_present & hf_vsdb_def.scdc_present;
	hf_vsdb_dest.rr_cap = hf_vsdb_tx.rr_cap & hf_vsdb_def.rr_cap;
	hf_vsdb_dest.cable_status = hf_vsdb_tx.cable_status & hf_vsdb_def.cable_status;
	hf_vsdb_dest.ccbpci = hf_vsdb_tx.ccbpci & hf_vsdb_def.ccbpci;
	hf_vsdb_dest.lte_340m_scramble =
		hf_vsdb_tx.lte_340m_scramble & hf_vsdb_def.lte_340m_scramble;
	hf_vsdb_dest.independ_view =
		hf_vsdb_tx.independ_view & hf_vsdb_def.independ_view;
	hf_vsdb_dest.dual_view = hf_vsdb_tx.dual_view & hf_vsdb_def.dual_view;
	hf_vsdb_dest._3d_osd_disparity =
		hf_vsdb_tx._3d_osd_disparity & hf_vsdb_def._3d_osd_disparity;
	if (hf_vsdb_def.max_frl_rate > hf_vsdb_tx.max_frl_rate)
		hf_vsdb_dest.max_frl_rate = hf_vsdb_tx.max_frl_rate;
	else
		hf_vsdb_dest.max_frl_rate = hf_vsdb_def.max_frl_rate;
	hf_vsdb_dest.uhd_vic = hf_vsdb_tx.uhd_vic & hf_vsdb_def.uhd_vic;
	hf_vsdb_dest.dc_48bit_420 =
		hf_vsdb_tx.dc_48bit_420 & hf_vsdb_def.dc_48bit_420;
	hf_vsdb_dest.dc_36bit_420 =
		hf_vsdb_tx.dc_36bit_420 & hf_vsdb_def.dc_36bit_420;
	hf_vsdb_dest.dc_30bit_420 =
		hf_vsdb_tx.dc_30bit_420 & hf_vsdb_def.dc_30bit_420;
	hf_vsdb_dest.qms_tfr_max = hf_vsdb_tx.qms_tfr_max & hf_vsdb_def.qms_tfr_max;
	hf_vsdb_dest.qms = hf_vsdb_tx.qms & hf_vsdb_def.qms;
	hf_vsdb_dest.m_delta = hf_vsdb_tx.m_delta & hf_vsdb_def.m_delta;
	hf_vsdb_dest.qms_tfr_min = hf_vsdb_tx.qms_tfr_min & hf_vsdb_def.qms_tfr_min;
	hf_vsdb_dest.neg_mvrr = hf_vsdb_tx.neg_mvrr & hf_vsdb_def.neg_mvrr;
	hf_vsdb_dest.fva = hf_vsdb_tx.fva & hf_vsdb_def.fva;
	hf_vsdb_dest.allm = hf_vsdb_tx.allm & hf_vsdb_def.allm;
	hf_vsdb_dest.fapa_start_location =
		hf_vsdb_tx.fapa_start_location & hf_vsdb_def.fapa_start_location;
	if (hf_vsdb_dest.len < 10)
		goto _end;
	if (hf_vsdb_def.vrr_min > hf_vsdb_tx.vrr_min)
		hf_vsdb_dest.vrr_min = hf_vsdb_tx.vrr_min;
	else
		hf_vsdb_dest.vrr_min = hf_vsdb_def.vrr_min;
	if (((hf_vsdb_def.vrr_max_lo + hf_vsdb_def.vrr_max_hi) << 8) >
		((hf_vsdb_tx.vrr_max_lo + hf_vsdb_tx.vrr_max_hi) << 8)) {
		hf_vsdb_dest.vrr_max_lo = hf_vsdb_tx.vrr_max_lo;
		hf_vsdb_dest.vrr_max_hi = hf_vsdb_tx.vrr_max_hi;
	} else {
		hf_vsdb_dest.vrr_max_lo = hf_vsdb_def.vrr_max_lo;
		hf_vsdb_dest.vrr_max_hi = hf_vsdb_def.vrr_max_hi;
	}
	if (hf_vsdb_dest.len < 13)
		goto _end;
	hf_vsdb_dest.dsc_1p2 = hf_vsdb_tx.dsc_1p2 & hf_vsdb_def.dsc_1p2;
	hf_vsdb_dest.dsc_native_420 =
		hf_vsdb_tx.dsc_native_420 & hf_vsdb_def.dsc_native_420;
	hf_vsdb_dest.dsc_all_bpp = hf_vsdb_tx.dsc_all_bpp & hf_vsdb_def.dsc_all_bpp;
	hf_vsdb_dest.dsc_16bpc = hf_vsdb_tx.dsc_16bpc & hf_vsdb_def.dsc_16bpc;
	hf_vsdb_dest.dsc_12bpc = hf_vsdb_tx.dsc_12bpc & hf_vsdb_def.dsc_12bpc;
	hf_vsdb_dest.dsc_10bpc = hf_vsdb_tx.dsc_10bpc & hf_vsdb_def.dsc_10bpc;

	if (hf_vsdb_def.dsc_max_frl_rate > hf_vsdb_tx.dsc_max_frl_rate)
		hf_vsdb_dest.dsc_max_frl_rate = hf_vsdb_tx.dsc_max_frl_rate;
	else
		hf_vsdb_dest.dsc_max_frl_rate = hf_vsdb_def.dsc_max_frl_rate;
	if (hf_vsdb_def.dsc_max_slices > hf_vsdb_tx.dsc_max_slices)
		hf_vsdb_dest.dsc_max_slices = hf_vsdb_tx.dsc_max_slices;
	else
		hf_vsdb_dest.dsc_max_slices = hf_vsdb_def.dsc_max_slices;

	if (hf_vsdb_def.dsc_total_chunk_kbytes > hf_vsdb_tx.dsc_total_chunk_kbytes)
		hf_vsdb_dest.dsc_total_chunk_kbytes = hf_vsdb_tx.dsc_total_chunk_kbytes;
	else
		hf_vsdb_dest.dsc_total_chunk_kbytes = hf_vsdb_def.dsc_total_chunk_kbytes;

	hf_vsdb_dest.fapa_end_extended =
		hf_vsdb_tx.fapa_end_extended & hf_vsdb_def.fapa_end_extended;

_end:
	//p_edid[hf_vsdb_start] = 0x60 + hf_vsdb_dest.len;
	memcpy(p_edid + hf_vsdb_start, &hf_vsdb_dest, hf_vsdb_dest.len + 1);
	i = hf_vsdb_start + hf_vsdb_dest.len + 1;
	for (; i < 255 - tag_len_def + hf_vsdb_dest.len; i++)
		p_edid[i] = p_edid[i + tag_len_def - hf_vsdb_dest.len];
	for (; i < 255; i++)
		p_edid[i] = 0;
	/* dtd offset modify */
	p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -= tag_len_def - hf_vsdb_dest.len;
}

void rpt_edid_14_vs_db_extraction(unsigned char *p_edid)
{
	u8 vsdb_start = 0;
	u8 tx_vsdb_start = 0;
	struct vsdb_s vsdb_tx, vsdb_def, vsdb_dest;
	u8 tag_len_tx, tag_len_def, i, j, k;
	u8 tx_hdmi_vic[4] = {0};
	u8 def_hdmi_vic[4] = {0};

	memset(&vsdb_tx, 0, sizeof(struct vsdb_s));
	memset(&vsdb_def, 0, sizeof(struct vsdb_s));
	memset(&vsdb_dest, 0, sizeof(struct vsdb_s));
	if (!p_edid)
		return;

	vsdb_start = rx_get_cea_tag_offset(p_edid, VENDOR_TAG);
	if (!vsdb_start) {
		//abnormal condition, only for errordebug
		rx_pr("unsupported vsdb!!\n");
		return;
	}
	tag_len_def = p_edid[vsdb_start] & 0x1f;
	memcpy(&vsdb_def, p_edid + vsdb_start, tag_len_def + 1);
	//Default edid parsing
	i = 8;
	if (tag_len_def <= i)
		goto _end1;
	if (vsdb_def.latency_fields_present) {
		i++;
		vsdb_def.video_latency = p_edid[vsdb_start + i];
		i++;
		vsdb_def.video_latency = p_edid[vsdb_start + i];
	}
	if (vsdb_def.i_latency_fields_present) {
		i++;
		vsdb_def.interlaced_video_latency = p_edid[vsdb_start + i];
		i++;
		vsdb_def.interlaced_audio_latency = p_edid[vsdb_start + i];
	}
	if (tag_len_def <= i)
		goto _end1;
	if (vsdb_def.hdmi_video_present) {
		i++;
		vsdb_def.image_size = (p_edid[vsdb_start + i] >> 3) & 3;
		vsdb_def._3d_multi_present = (p_edid[vsdb_start + i] >> 5) & 3;
		vsdb_def._3d_present = (p_edid[vsdb_start + i] >> 7) & 1;
		i++;
		vsdb_def.hdmi_3d_len = p_edid[vsdb_start + i] & 0x1f;
		vsdb_def.hdmi_vic_len = (p_edid[vsdb_start + i] >> 5) & 0x07;
	}
	if (vsdb_def.hdmi_vic_len) {
		i++;
		for (j = 0; j < vsdb_def.hdmi_vic_len; j++)
			def_hdmi_vic[j] = (p_edid[vsdb_start + i + j]);
	}
	if (vsdb_def.hdmi_3d_len) {
		//todo
		//unsupport 3d format by default
	}
_end1:
	tx_vsdb_start = rx_get_cea_tag_offset(edid_tx, VENDOR_TAG);
	if (!tx_vsdb_start) {
		if (log_level & EDID_LOG)
			rx_pr("no vsdb\n");
		edid_rm_db_by_tag(p_edid, VENDOR_TAG);
		return;
	}
	tag_len_tx = edid_tx[tx_vsdb_start] & 0x1f;
	memcpy(&vsdb_tx, edid_tx + tx_vsdb_start, tag_len_tx + 1);
	//TX edid parsing
	i = 5;
	if (tag_len_tx <= i) {
		if (log_level & EDID_LOG)
			rx_pr("vsdb only physcal address info\n");
		return;
	}
	i = 8;
	if (tag_len_tx <= i)
		goto _end2;
	if (vsdb_tx.latency_fields_present) {
		i++;
		vsdb_tx.video_latency = edid_tx[tx_vsdb_start + i];
		i++;
		vsdb_tx.audio_latency = edid_tx[tx_vsdb_start + i];
	}
	if (vsdb_tx.i_latency_fields_present) {
		i++;
		vsdb_tx.interlaced_video_latency = edid_tx[tx_vsdb_start + i];
		i++;
		vsdb_tx.interlaced_audio_latency = edid_tx[tx_vsdb_start + i];
	}
	if (tag_len_tx <= i)
		goto _end2;
	if (vsdb_tx.hdmi_video_present) {
		i++;
		vsdb_tx.image_size = (edid_tx[tx_vsdb_start + i] >> 3) & 3;
		vsdb_tx._3d_multi_present = (edid_tx[tx_vsdb_start + i] >> 5) & 3;
		vsdb_tx._3d_present = (edid_tx[tx_vsdb_start + i] >> 7) & 1;
		i++;
		vsdb_tx.hdmi_3d_len = edid_tx[tx_vsdb_start + i] & 0x1f;
		vsdb_tx.hdmi_vic_len = (edid_tx[tx_vsdb_start + i] >> 5) & 0x07;
	}
	if (vsdb_tx.hdmi_vic_len) {
		i++;
		for (j = 0; j < vsdb_tx.hdmi_vic_len; j++)
			tx_hdmi_vic[j] = (edid_tx[tx_vsdb_start + i + j]);
	}
	//if (vsdb_tx.hdmi_3d_len)
		//todo
_end2:
	vsdb_dest.len = tag_len_tx > tag_len_def ? tag_len_def : tag_len_tx;
	if (log_level & EDID_LOG)
		rx_pr("len=%d-%d-%d\n", vsdb_dest.len, tag_len_tx, tag_len_def);
	vsdb_dest.tag = vsdb_tx.tag;
	vsdb_dest.ieee_first = vsdb_tx.ieee_first;
	vsdb_dest.ieee_second = vsdb_tx.ieee_second;
	vsdb_dest.ieee_third = vsdb_tx.ieee_third;
	vsdb_dest.b = vsdb_tx.b;
	vsdb_dest.a = vsdb_tx.a;
	vsdb_dest.d = vsdb_tx.d;
	vsdb_dest.c = vsdb_tx.c;
	if (vsdb_tx.a == 0) {
		vsdb_dest.b = vsdb_def.b;
		vsdb_dest.a = vsdb_def.a;
		vsdb_dest.d = vsdb_def.d;
		vsdb_dest.c = vsdb_def.c;
		if (log_level & EDID_LOG)
			rx_pr("err, use def err %x%x%x%x",
				  vsdb_def.a, vsdb_def.b, vsdb_def.c, vsdb_def.d);
	} else if (vsdb_tx.b == 0) {
		vsdb_dest.b = vsdb_def.a;
		vsdb_dest.a = vsdb_tx.a;
		vsdb_dest.d = 0;
		vsdb_dest.c = 0;
		if (log_level & EDID_LOG)
			rx_pr("dest physical address:%x%x%x%x",
				  vsdb_tx.a, vsdb_tx.b, vsdb_tx.c, vsdb_tx.d);
	} else if (vsdb_tx.c == 0) {
		vsdb_dest.b = vsdb_tx.b;
		vsdb_dest.a = vsdb_tx.a;
		vsdb_dest.d = 0;
		vsdb_dest.c = vsdb_def.a;
		if (log_level & EDID_LOG)
			rx_pr("dest physical address:%x%x%x%x",
				  vsdb_tx.a, vsdb_tx.b, vsdb_tx.c, vsdb_tx.d);
	} else if (vsdb_tx.d == 0) {
		vsdb_dest.b = vsdb_tx.b;
		vsdb_dest.a = vsdb_tx.a;
		vsdb_dest.d = vsdb_def.a;
		vsdb_dest.c = vsdb_tx.c;
		if (log_level & EDID_LOG)
			rx_pr("dest physical address:%x%x%x%x",
				  vsdb_tx.a, vsdb_tx.b, vsdb_tx.c, vsdb_tx.d);
	} else {
		vsdb_dest.b = vsdb_tx.b;
		vsdb_dest.a = vsdb_tx.a;
		vsdb_dest.d = vsdb_tx.d;
		vsdb_dest.c = vsdb_tx.c;
		if (log_level & EDID_LOG)
			rx_pr("err, use tx address:%x%x%x%x",
				  vsdb_def.a, vsdb_def.b, vsdb_def.c, vsdb_def.d);
	}
	if (vsdb_dest.len <= 5)
		return;
	vsdb_dest.dvi_dual = vsdb_tx.dvi_dual & vsdb_def.dvi_dual;
	vsdb_dest.dc_y444 = vsdb_tx.dc_y444 & vsdb_def.dc_y444;
	vsdb_dest.dc_30bit = vsdb_tx.dc_30bit & vsdb_def.dc_30bit;
	vsdb_dest.dc_36bit = vsdb_tx.dc_36bit & vsdb_def.dc_36bit;
	vsdb_dest.dc_48bit = vsdb_tx.dc_48bit & vsdb_def.dc_48bit;
	vsdb_dest.support_ai = vsdb_tx.support_ai & vsdb_def.support_ai;
	if (vsdb_dest.len <= 6)
		goto _end3;
	if (vsdb_def.max_tmds_clk > vsdb_tx.max_tmds_clk)
		vsdb_dest.max_tmds_clk = vsdb_tx.max_tmds_clk;
	else
		vsdb_dest.max_tmds_clk = vsdb_def.max_tmds_clk;
	//pb5
	if (vsdb_dest.len <= 7)
		goto _end3;
	vsdb_dest.cnc0 = vsdb_tx.cnc0 & vsdb_def.cnc0;
	vsdb_dest.cnc1 = vsdb_tx.cnc1 & vsdb_def.cnc1;
	vsdb_dest.cnc2 = vsdb_tx.cnc2 & vsdb_def.cnc2;
	vsdb_dest.cnc3 = vsdb_tx.cnc3 & vsdb_def.cnc3;
	vsdb_dest.hdmi_video_present =
		vsdb_tx.hdmi_video_present & vsdb_def.hdmi_video_present;
	vsdb_dest.i_latency_fields_present =
		vsdb_tx.i_latency_fields_present & vsdb_def.i_latency_fields_present;
	vsdb_dest.latency_fields_present =
		vsdb_tx.latency_fields_present & vsdb_def.latency_fields_present;
	if (vsdb_dest.len <= 8)
		goto _end3;
	//pb6
	if (vsdb_dest.latency_fields_present) {
		if (LATENCY_MAX - vsdb_tx.video_latency > vsdb_def.video_latency)
			vsdb_dest.video_latency = vsdb_tx.video_latency + vsdb_def.video_latency;
		else
			vsdb_dest.video_latency = LATENCY_MAX;
		if (LATENCY_MAX - vsdb_tx.audio_latency > vsdb_def.audio_latency)
			vsdb_dest.audio_latency = vsdb_tx.audio_latency + vsdb_def.audio_latency;
		else
			vsdb_dest.audio_latency = LATENCY_MAX;
	}
	if (vsdb_dest.i_latency_fields_present) {
		if (LATENCY_MAX - vsdb_tx.interlaced_video_latency >
			vsdb_def.interlaced_video_latency)
			vsdb_dest.interlaced_video_latency =
				vsdb_tx.interlaced_video_latency +
				vsdb_def.interlaced_video_latency;
		else
			vsdb_dest.interlaced_video_latency = LATENCY_MAX;
		if (LATENCY_MAX - vsdb_tx.interlaced_audio_latency >
			vsdb_def.interlaced_audio_latency)
			vsdb_dest.interlaced_audio_latency =
			vsdb_tx.interlaced_audio_latency + vsdb_def.interlaced_audio_latency;
		else
			vsdb_dest.interlaced_audio_latency = LATENCY_MAX;
	}
	if (vsdb_dest.hdmi_video_present) {
		if (vsdb_def.image_size > vsdb_tx.image_size)
			vsdb_dest.image_size = vsdb_tx.image_size;
		else
			vsdb_dest.image_size = vsdb_def.image_size;
	}
	if (vsdb_def.hdmi_vic_len >= vsdb_tx.hdmi_vic_len)
		vsdb_dest.hdmi_vic_len = vsdb_tx.hdmi_vic_len;
	else
		vsdb_dest.hdmi_vic_len = vsdb_def.hdmi_vic_len;
_end3:
	i = 5;
	//memcpy(p_edid + vsdb_start, &vsdb_dest, i + 1);
	if (vsdb_dest.dvi_dual ||
		vsdb_dest.dc_y444 ||
		vsdb_dest.dc_30bit ||
		vsdb_dest.dc_36bit ||
		vsdb_dest.dc_48bit ||
		vsdb_dest.support_ai)
		i = 6;
	if (vsdb_dest.max_tmds_clk)
		i = 7;
	memcpy(p_edid + vsdb_start, &vsdb_dest, i + 1);
	if (vsdb_dest.cnc0 || vsdb_dest.cnc1 || vsdb_dest.cnc2 || vsdb_dest.cnc3 ||
		vsdb_dest.hdmi_video_present ||
		vsdb_dest.i_latency_fields_present ||
		vsdb_dest.latency_fields_present) {
		i++;
		p_edid[vsdb_start + i] =
			vsdb_dest.cnc0 +
			(vsdb_dest.cnc1 << 1) +
			(vsdb_dest.cnc2 << 2) +
			(vsdb_dest.cnc3 << 3) +
			(vsdb_dest.hdmi_video_present << 5) +
			(vsdb_dest.i_latency_fields_present << 6) +
			(vsdb_dest.latency_fields_present << 7);
		//memcpy(p_edid + vsdb_start + i, &vsdb_dest + i, 1);
	}
	if (vsdb_dest.latency_fields_present) {
		i++;
		p_edid[vsdb_start + i] = vsdb_dest.video_latency;
		i++;
		p_edid[vsdb_start + i] = vsdb_dest.audio_latency;
	}
	if (vsdb_dest.i_latency_fields_present) {
		i++;
		p_edid[vsdb_start + i] = vsdb_dest.interlaced_video_latency;
		i++;
		p_edid[vsdb_start + i] = vsdb_dest.interlaced_audio_latency;
	}
	if (vsdb_dest.hdmi_video_present) {
		i++;
		p_edid[vsdb_start + i] = (vsdb_dest.image_size << 3) +
								(vsdb_dest._3d_multi_present << 5) +
								(vsdb_dest._3d_present << 7);
		i++;
		if (vsdb_dest.hdmi_vic_len) {
			vsdb_dest.hdmi_vic_len = 0;
			for (j = 0; j < vsdb_tx.hdmi_vic_len; j++) {
				for (k = 0; k < vsdb_def.hdmi_vic_len; k++) {
					if (tx_hdmi_vic[j] == def_hdmi_vic[k]) {
						i++;
						vsdb_dest.hdmi_vic_len++;
						p_edid[vsdb_start + i] = tx_hdmi_vic[j];
						break;
					}
				}
			}
			p_edid[vsdb_start + i - vsdb_dest.hdmi_vic_len] =
				(vsdb_dest.hdmi_vic_len << 5) +
				vsdb_dest.hdmi_3d_len;
		}
		//3D related/ todo
	}
	if (i != vsdb_dest.len) {
		if (log_level & EDID_LOG)
			rx_pr("vsdb length warning-%d-%d\n",
				  i, vsdb_dest.len);
		vsdb_dest.len = i > vsdb_dest.len ? vsdb_dest.len : i;
	}
	p_edid[vsdb_start] = (VENDOR_TAG << 5) + vsdb_dest.len;
	for (i = vsdb_start + vsdb_dest.len + 1; i < 255 - tag_len_def + vsdb_dest.len; i++)
		p_edid[i] = p_edid[i + tag_len_def - vsdb_dest.len];
	for (; i < 255; i++)
		p_edid[i] = 0;
	/* dtd offset modify */
	p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -= tag_len_def - vsdb_dest.len;
}

void rpt_edid_video_db_extraction(unsigned char *p_edid)
{
	u8 vdb_start = 0;
	u8 tx_vdb_start = 0;
	u8 vdb_dest[31];
	u8 tx_native_list[31];
	u8 tag_len_tx, tag_len_def;
	u8 i, j, tag_len_dest = 0;
	u8 native_cnt = 0;

	if (!p_edid)
		return;

	vdb_start = rx_get_cea_tag_offset(p_edid, VIDEO_TAG);
	if (!vdb_start) {
		//abnormal condition, only for errordebug
		rx_pr("unsupported vdb!!\n");
		return;
	}
	tag_len_def = p_edid[vdb_start] & 0x1f;
	memset(def_vic_list, 0, 31);
	memcpy(def_vic_list, p_edid + vdb_start + 1, tag_len_def);
	for (i = 0; i < tag_len_def; i++) {
		if (def_vic_list[i] >= 129 && def_vic_list[i] <= 192)
			def_vic_list[i] = def_vic_list[i] & 0x7f;
	}
	tx_vdb_start = rx_get_cea_tag_offset(edid_tx, VIDEO_TAG);
	if (!tx_vdb_start) {
		if (log_level & EDID_LOG)
			rx_pr("no vdb\n");
		return;
	}
	tag_len_tx = edid_tx[tx_vdb_start] & 0x1f;
	memset(tx_vic_list, 0, 31);
	memcpy(tx_vic_list, edid_tx + tx_vdb_start + 1, tag_len_tx);
	for (i = 0; i < tag_len_tx; i++) {
		if (tx_vic_list[i] >= 129 && tx_vic_list[i] <= 192) {
			tx_vic_list[i] = tx_vic_list[i] & 0x7f;
			tx_native_list[native_cnt] = tx_vic_list[i];
			native_cnt++;
		}
	}
	for (j = 0; j < tag_len_tx; j++) {
		for (i = 0; i < tag_len_def; i++) {
			if (tx_vic_list[j] == def_vic_list[i]) {
				vdb_dest[tag_len_dest] = tx_vic_list[j];
				tag_len_dest++;
				break;
			}
		}
	}
	for (j = 0; j < tag_len_dest; j++) {
		for (i = 0; i < native_cnt; i++) {
			if (vdb_dest[j] == tx_native_list[i])
				vdb_dest[j] = vdb_dest[j] | 0x80;
		}
	}
	if (log_level & EDID_LOG)
		rx_pr("vdb_size = %d\n", tag_len_dest);
	p_edid[vdb_start] = (VIDEO_TAG << 5) + tag_len_dest;
	memcpy(p_edid + vdb_start + 1, vdb_dest, tag_len_dest);
	for (i = vdb_start + tag_len_dest + 1; i < 255 - tag_len_def + tag_len_dest; i++)
		p_edid[i] = p_edid[i + tag_len_def - tag_len_dest];
	for (; i < 255; i++)
		p_edid[i] = 0;
	/* dtd offset modify */
	p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -= tag_len_def - tag_len_dest;
}

void rpt_edid_audio_db_extraction(unsigned char *p_edid)
{
	u8 adb_start = 0;
	u8 tx_adb_start = 0;
	struct edid_audio_block_t adb_tx[10], adb_def[10], adb_dest[10];
	u8 tag_len_tx, tag_len_def;
	u8 i, j, tag_len_dest = 0;
	//some device such as arc, it's edid include several same audio format
	//can not judge which format is the best format when extract rx edid
	//when found tx edid has duplicat audio format,just use rx audio format
	int tx_duplicate_audio_format = 0;
	int tx_audio_format = 0;

	if (!p_edid)
		return;

	adb_start = rx_get_cea_tag_offset(p_edid, AUDIO_TAG);
	if (!adb_start) {
		//abnormal condition, only for errordebug
		rx_pr("unsupported adb!!\n");
		return;
	}
	tag_len_def = p_edid[adb_start] & 0x1f;
	if (tag_len_def % 3) {
		if (log_level & EDID_LOG)
			rx_pr("aud db err\n");
		return;
	}
	memcpy((u8 *)adb_def, p_edid + adb_start + 1, tag_len_def);
	tx_adb_start = rx_get_cea_tag_offset(edid_tx, AUDIO_TAG);
	if (!tx_adb_start) {
		if (log_level & EDID_LOG)
			rx_pr("no adb\n");
		return;
	}
	tag_len_tx = edid_tx[tx_adb_start] & 0x1f;
	if (tag_len_tx % 3) {
		if (log_level & EDID_LOG)
			rx_pr("aud db err\n");
		return;
	}
	memcpy((u8 *)adb_tx, edid_tx + tx_adb_start + 1, tag_len_tx);
	for (i = 0; i < tag_len_def / 3; i++) {
		for (j = 0; j < tag_len_tx / 3; j++) {
			if (adb_tx[j].format_code == adb_def[i].format_code) {
				if (tx_audio_format & (1 << adb_tx[j].format_code)) {
					if (log_level & EDID_LOG)
						rx_pr("skip same audio format %d",
							adb_tx[j].format_code);
					tx_duplicate_audio_format |= (1 << adb_tx[j].format_code);
					break;
				}
				adb_dest[tag_len_dest / 3].format_code = adb_tx[j].format_code;
				tx_audio_format |= (1 << adb_tx[j].format_code);
				//max channel
				if (adb_def[i].max_channel >
						adb_tx[j].max_channel)
					adb_dest[tag_len_dest / 3].max_channel =
						adb_tx[j].max_channel;
				else
					adb_dest[tag_len_dest / 3].max_channel =
						adb_def[i].max_channel;

				//frequency list
				adb_dest[tag_len_dest / 3].usr.freq_list =
					adb_tx[j].usr.freq_list &
					adb_def[i].usr.freq_list;
				//PCM bit rate
				//2-8 max bit rate
				//9-13 dependent value
				//14~ todo
				if (adb_dest[tag_len_dest / 3].format_code == AUDIO_FORMAT_LPCM) {
					adb_dest[tag_len_dest / 3].bit_rate.others =
						adb_tx[j].bit_rate.others &
						adb_def[i].bit_rate.others;
				} else if ((adb_dest[tag_len_dest / 3].format_code >=
						AUDIO_FORMAT_AC3) &&
						(adb_dest[tag_len_dest / 3].format_code <=
						AUDIO_FORMAT_ATRAC)) {
					if (adb_tx[j].bit_rate.others >
							adb_def[i].bit_rate.others)
						adb_dest[tag_len_dest / 3].bit_rate.others =
							adb_def[i].bit_rate.others;
					else
						adb_dest[tag_len_dest / 3].bit_rate.others =
							adb_tx[j].bit_rate.others;
				} else if ((adb_dest[tag_len_dest / 3].format_code >=
							AUDIO_FORMAT_OBA) &&
						(adb_dest[tag_len_dest / 3].format_code <=
							AUDIO_FORMAT_DST)) {
					adb_dest[tag_len_dest / 3].bit_rate.others =
						adb_tx[j].bit_rate.others &
						adb_def[i].bit_rate.others;
				} else {
					//todo
				}
				tag_len_dest += 3;
			}
		}
	}
	//recover duplicate audio format to rx format
	for (i = 0; i < tag_len_dest / 3; i++) {
		if (tx_duplicate_audio_format & (1 << adb_dest[i].format_code)) {
			for (j = 0; j < tag_len_def / 3; j++) {
				if (adb_dest[i].format_code == adb_def[j].format_code) {
					memcpy(&adb_dest[i], &adb_def[j],
							sizeof(struct edid_audio_block_t));
					break;
				}
			}
		}
	}
	if (log_level & EDID_LOG)
		rx_pr("adb_size = %d %d %d\n", tag_len_dest, tag_len_def, tag_len_tx);
	p_edid[adb_start] = (AUDIO_TAG << 5) + tag_len_dest;
	memcpy(p_edid + adb_start + 1, adb_dest, tag_len_dest);
	for (i = adb_start + tag_len_dest + 1; i < 255 - tag_len_def + tag_len_dest; i++)
		p_edid[i] = p_edid[i + tag_len_def - tag_len_dest];
	for (; i < 255; i++)
		p_edid[i] = 0;
		/* dtd offset modify */
	p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -= tag_len_def - tag_len_dest;
}

void rpt_edid_colorimetry_db_extraction(unsigned char *p_edid)
{
	u8 db_start = 0;
	u8 tx_db_start = 0;
	u8 colorimetry_tx[2], colorimetry_def[2], colorimetry_dest[2];
	u8 def_tag_len = 0;
	u8 i = 0;

	if (!p_edid)
		return;

	db_start = rx_get_cea_tag_offset(p_edid, EXTENDED_CDB_TAG);
	if (!db_start) {
		//abnormal condition, only for errordebug
		if (log_level & EDID_LOG)
			rx_pr("unsupported colorimetry!!\n");
		return;
	} else {
		def_tag_len = 3;
		memcpy(colorimetry_def, p_edid + db_start + 2, 2);
	}
	tx_db_start = rx_get_cea_tag_offset(edid_tx, EXTENDED_CDB_TAG);
	if (!tx_db_start) {
		if (log_level & EDID_LOG)
			rx_pr("no colorimetry\n");
		edid_rm_db_by_tag(p_edid, EXTENDED_CDB_TAG);
		return;
	}
	memcpy(colorimetry_tx, edid_tx + tx_db_start + 2, 2);
	for (i = 0; i < 2; i++)
		colorimetry_dest[i] = colorimetry_tx[i] & colorimetry_def[i];
	memcpy(p_edid + db_start + 2, colorimetry_dest, 2);
}

void rpt_edid_420_vdb_extraction(unsigned char *p_edid)
{
	u8 _420_vdb_start = 0;
	u8 _420_cmdb_start = 0;
	u8 tx_420_vdb_start = 0;
	u8 tx_420_cmdb_start = 0;
	u8 vdb_start = 0;
	u8 tx_420vic_list[31] = { 0 };
	u8 def_420vic_list[31] = { 0 };
	u8 dest_420vic_list[31] = { 0 };
	u8 new_vic_list[31] = { 0 };
	u8 new_420vdb_list[31] = { 0 };
	u8 def_vdb_len = 0;
	u8 tx_420_vdb_len = 0;
	u8 tx_cmdb_len = 0;
	u8 def_420_vdb_len = 0;
	u8 def_cmdb_len = 0;
	u8 i = 0;
	u8 j = 0;
	u8 k = 0;
	u8 l = 0;
	u32 cmdb_map = 0;

	if (!p_edid)
		return;

	//420cmdb
	_420_cmdb_start = rx_get_cea_tag_offset(p_edid, EXTENDED_Y420CMDB_TAG);
	if (_420_cmdb_start) {
		def_cmdb_len = (p_edid[_420_cmdb_start]) & 0x1f;
		if (def_cmdb_len > 5) {
			if (log_level & EDID_LOG)
				rx_pr("cmdb len error:%d\n", def_cmdb_len);
			def_cmdb_len = 5;
		}
		for (i = 0; i < def_cmdb_len - 1; i++)
			cmdb_map += p_edid[_420_cmdb_start + 2 + i] << (i * 8);
		for (i = 0; i < 31; i++) {
			if ((cmdb_map >> i) & 1) {
				def_420vic_list[j] = def_vic_list[i];
				if (def_420vic_list[j])
					j++;
			}
		}
	}
	//420vdb
	_420_vdb_start = rx_get_cea_tag_offset(p_edid, EXTENDED_Y420VDB_TAG);
	if (_420_vdb_start) {
		def_420_vdb_len = (p_edid[_420_vdb_start]) & 0x1f;
		if (j + def_420_vdb_len - 1 > 31) {
			rx_pr("420DB error\n");
			return;
		}
		memcpy(def_420vic_list + j, p_edid + _420_vdb_start + 2, def_420_vdb_len - 1);
	}

	if (def_420_vdb_len == 0 && def_cmdb_len == 0) {
		if (log_level & EDID_LOG)
			rx_pr("unsupported 420\n");
		return;
	}
	//TX EDID parsing
	//420cmdb
	j = 0;
	tx_420_cmdb_start = rx_get_cea_tag_offset(edid_tx, EXTENDED_Y420CMDB_TAG);
	if (tx_420_cmdb_start) {
		tx_cmdb_len = (edid_tx[tx_420_cmdb_start]) & 0x1f;
		if (tx_cmdb_len > 5) {
			if (log_level & EDID_LOG)
				rx_pr("cmdb len error:%d\n", tx_cmdb_len);
			tx_cmdb_len = 5;
		}
		cmdb_map = 0;
		for (i = 0; i <  tx_cmdb_len - 1; i++)
			cmdb_map += edid_tx[tx_420_cmdb_start + 2 + i] << (i * 8);
		j = 0;
		for (i = 0; i < 31; i++) {
			if ((cmdb_map >> i) & 1) {
				tx_420vic_list[j] = tx_vic_list[i];
				if (tx_420vic_list[j])
					j++;
			}
		}
	}
	//420vdb
	tx_420_vdb_start = rx_get_cea_tag_offset(edid_tx, EXTENDED_Y420VDB_TAG);
	if (tx_420_vdb_start) {
		tx_420_vdb_len = (edid_tx[tx_420_vdb_start]) & 0x1f;
		if (j + tx_420_vdb_len - 1 > 31) {
			rx_pr("420DB error\n");
			return;
		}
		memcpy(tx_420vic_list + j, edid_tx + tx_420_vdb_start + 2, tx_420_vdb_len - 1);
	}
	if (tx_420_vdb_len == 0 && tx_cmdb_len == 0) {
		if (log_level & EDID_LOG)
			rx_pr("no 420 block\n");
		edid_rm_db_by_tag(p_edid, EXTENDED_Y420CMDB_TAG);
		edid_rm_db_by_tag(p_edid, EXTENDED_Y420VDB_TAG);
		return;
	}
	k = 0;
	for (j = 0; tx_420vic_list[j]; j++)
		for (i = 0; def_420vic_list[i]; i++)
			if (def_420vic_list[i] == tx_420vic_list[j] && tx_420vic_list[j] != 0) {
				dest_420vic_list[k] = tx_420vic_list[j];
				k++;
				break;
			}
	if (def_cmdb_len) {
		vdb_start = rx_get_cea_tag_offset(p_edid, VIDEO_TAG);
		if (!vdb_start && (log_level & EDID_LOG))
			rx_pr("cannot find vdb\n");
		def_vdb_len = p_edid[vdb_start] & 0x1f;
		memset(def_vic_list, 0, 31);
		memcpy(def_vic_list, p_edid + vdb_start + 1, def_vdb_len);
		k = 0;
		for (i = 0; dest_420vic_list[i]; i++) {
			for (j = 0; j < def_vdb_len; j++) {
				if (dest_420vic_list[i] == def_vic_list[j]) {
					new_vic_list[k] = def_vic_list[j];
					k++;
					break;
				} else if (j == def_vdb_len - 1) {
					new_420vdb_list[l] = dest_420vic_list[i];
					l++;
				}
			}
		}
		if (log_level & EDID_LOG) {
			for (i = 0; i < 10; i++)
				rx_pr("new_tx_vic_list[%d]=%x\n", i, new_vic_list[i]);
			for (i = 0; i < 10; i++)
				rx_pr("new_420vdb_list[%d]=%x\n", i, new_420vdb_list[i]);
		}
		if (k == 0) {
			cmdb_map = 0;
			edid_rm_db_by_tag(p_edid, EXTENDED_Y420CMDB_TAG);
			tx_cmdb_len = 0;
			goto end1;
		} else {
			cmdb_map = (1 << k) - 1;
		}
		for (i = 0; def_vic_list[i]; i++) {
			for (j = 0; j < k; j++) {
				if (new_vic_list[j] == def_vic_list[i])
					break;
			}
			if (j == k) {
				new_vic_list[k] = def_vic_list[i];
				k++;
			}
		}
		j = 0;
		while (cmdb_map & 0xff) {
			p_edid[_420_cmdb_start + 2 + j] = cmdb_map & 0xff;
			cmdb_map = cmdb_map >> 8;
			j++;
		}
		memcpy(p_edid + vdb_start + 1, new_vic_list, def_vdb_len);
		p_edid[_420_cmdb_start] = (USE_EXTENDED_TAG << 5) + j + 1;
		for (i = _420_cmdb_start + j + 2; i < 255 - def_cmdb_len + j + 1; i++)
			p_edid[i] = p_edid[i + def_cmdb_len - j - 1];
		for (; i < 255; i++)
			p_edid[i] = 0;
		/* dtd offset modify */
		p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -= def_cmdb_len - j - 1;
end1:
		if (def_420_vdb_len) {
			if (new_420vdb_list[0] && l) {
				_420_vdb_start =
					rx_get_cea_tag_offset(p_edid,
						EXTENDED_Y420VDB_TAG);
				p_edid[_420_vdb_start] = (USE_EXTENDED_TAG << 5) + l + 1;
				memcpy(p_edid + _420_vdb_start + 2, new_420vdb_list, l);
				for (i = _420_vdb_start + l + 1;
					i < 255 - def_420_vdb_len + l + 1; i++)
					p_edid[i] = p_edid[i + def_420_vdb_len - l - 1];
				for (; i < 255; i++)
					p_edid[i] = 0;
				/* dtd offset modify */
				p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -=
					def_420_vdb_len - l - 1;
			} else {
				edid_rm_db_by_tag(p_edid, EXTENDED_Y420VDB_TAG);
			}
		}
	} else if (def_420_vdb_len) {
		if (dest_420vic_list[0] && k) {
			p_edid[_420_vdb_start] = (USE_EXTENDED_TAG << 5) + k + 1;
			memcpy(p_edid + _420_vdb_start + 2, dest_420vic_list, k);
			for (i = _420_vdb_start + k + 2; i < 255 - def_420_vdb_len + k + 1; i++)
				p_edid[i] = p_edid[i + def_420_vdb_len - k - 1];
			for (; i < 255; i++)
				p_edid[i] = 0;
			/* dtd offset modify */
			p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -=
				def_420_vdb_len - k - 1;
		} else {
			edid_rm_db_by_tag(p_edid, EXTENDED_Y420VDB_TAG);
		}
	}
}

void rpt_edid_hdr_static_db_extraction(unsigned char *p_edid)
{
	u8 db_start = 0;
	u8 tx_db_start = 0;
	u8 tx_db[31], def_db[31];
	u8 tx_len = 0;
	u8 def_len = 0;
	u8 i;

	if (!p_edid)
		return;
	db_start = rx_get_cea_tag_offset(p_edid, EXTENDED_HDR_STATIC_TAG);
	if (!db_start) {
		if (log_level & EDID_LOG)
			rx_pr("unsupported HDR!!\n");
		return;
	} else {
		def_len = (p_edid[db_start]) & 0x1f;
		memcpy(def_db, p_edid + db_start + 2, def_len - 1);
	}
	tx_db_start = rx_get_cea_tag_offset(edid_tx, EXTENDED_HDR_STATIC_TAG);
	if (!tx_db_start) {
		if (log_level & EDID_LOG)
			rx_pr("no HDR!!\n");
		//remove p_edid's block, not edid_tx!!!!!!
		edid_rm_db_by_tag(p_edid, EXTENDED_HDR_STATIC_TAG);
		return;
	} else {
		tx_len = (edid_tx[tx_db_start]) & 0x1f;
		memcpy(tx_db, edid_tx + tx_db_start + 2, tx_len - 1);
	}
	/*
	for (i = 0; i < 2; i++)
		tx_db[i] = tx_db[i] & def_db[i];
	*/
	if (def_len && tx_len) {
		//keep using tx data block, only BYTE 3 && 4 changed.
		for (i = 0; i < 2; i++)
			p_edid[db_start + i + 2] = tx_db[i] & def_db[i];
	} else if (def_len == 0) {
		//remove HDR block
		for (i = db_start; i < 254 - tx_len; i++)
			p_edid[i] = p_edid[i + tx_len];
		for (; i < 255; i++)
			p_edid[i] = 0;
	}
	/* dtd offset modify */
	p_edid[EDID_BLOCK1_OFFSET + EDID_DESCRIP_OFFSET] -= tx_len - def_len;
}

void rpt_edid_vsv_hdr10plus_extraction(unsigned char *p_edid)
{
	u8 db_start = 0;

	if (!p_edid)
		return;

	db_start = rx_get_cea_tag_offset(p_edid, VSVDB_HDR10P_TAG);
	if (!db_start) {
		if (log_level & EDID_LOG)
			rx_pr("rx no vsv hdr10+!!\n");
		return;
	}

	db_start = rx_get_cea_tag_offset(edid_tx, VSVDB_HDR10P_TAG);
	if (!db_start) {
		if (log_level & EDID_LOG)
			rx_pr("tx no vsv hdr10+, need remove rx vsv hdr10+\n");
		edid_rm_db_by_tag(p_edid, VSVDB_HDR10P_TAG);
	}
}

void rpt_edid_vsv_db_extraction(unsigned char *p_edid)
{
	u8 db_start = 0;
	u8 tx_db_start = 0;
	u8 tx_len = 0;
	u8 db_version;

	if (!p_edid)
		return;

	db_start = rx_get_cea_tag_offset(p_edid, VSVDB_DV_TAG);
	if (!db_start) {
		if (log_level & EDID_LOG)
			rx_pr("rx no vsvdb!!\n");
		return;
	}

	tx_len = (p_edid[db_start]) & 0x1f;
	if (tx_len < 11)
		return;

	db_version = (p_edid[db_start + 5] >> 5) & 0x07;
	if (db_version != 2)
		return;

	p_edid[db_start + 7] &= 0xFE; //remove interface 420
	p_edid[db_start + 8] &= 0xFE; //remove 444 12bit
	p_edid[db_start + 9] &= 0xFE; //remove 444 10bit

	tx_db_start = rx_get_cea_tag_offset(edid_tx, VSVDB_DV_TAG);
	if (!tx_db_start) {
		if (log_level & EDID_LOG)
			rx_pr("tx no vsvdb!!,need remove rx vsvdb\n");
		edid_rm_db_by_tag(p_edid, VSVDB_DV_TAG);
	}
}

void rpt_edid_vsg_freesync_extraction(unsigned char *p_edid)
{
	u8 db_start = 0;

	if (!p_edid)
		return;

	db_start = rx_get_cea_tag_offset(p_edid, VSDB_FREESYNC_TAG);
	if (!db_start) {
		if (log_level & EDID_LOG)
			rx_pr("rx no vsg freesync!!\n");
		return;
	}

	db_start = rx_get_cea_tag_offset(edid_tx, VSDB_FREESYNC_TAG);
	if (!db_start) {
		if (log_level & EDID_LOG)
			rx_pr("tx no vsg freesync, need remove rx vsg freesync\n");
		edid_rm_db_by_tag(p_edid, VSDB_FREESYNC_TAG);
	}
}

bool get_dtd_data(u8 *p_edid, struct edid_info_s *edid_info)
{
	unsigned int pixel_clk;
	unsigned int hactive;
	unsigned int vactive;
	unsigned int hblank;
	unsigned int vblank;
	unsigned int htotal;
	unsigned int vtotal;
	bool ret = false;

	if (!p_edid)
		return 0;

	if (p_edid[0x36] == 0 && p_edid[0x37] == 0) {
		rx_pr("descriptor1 is not a DTD!\n");
		goto _dtd2;
	}
	ret = true;
	edid_info->descriptor1.pixel_clk = (p_edid[0x36] + (p_edid[0x37] << 8)) / 100;
	pixel_clk = edid_info->descriptor1.pixel_clk;
	edid_info->descriptor1.hactive = p_edid[0x38] + (((p_edid[0x3A] >> 4) & 0xF) << 8);
	hactive = edid_info->descriptor1.hactive;
	edid_info->descriptor1.vactive = p_edid[0x3B] + (((p_edid[0x3D] >> 4) & 0xF) << 8);
	vactive = edid_info->descriptor1.vactive;
	edid_info->descriptor1.hblank = ((p_edid[0x3A] & 0xF) << 8) + p_edid[0x39];
	hblank = edid_info->descriptor1.hblank;
	edid_info->descriptor1.vblank = ((p_edid[0x3D] & 0xF) << 8) + p_edid[0x3C];
	vblank = edid_info->descriptor1.vblank;
	htotal = hactive + hblank;
	vtotal = vactive + vblank;
	edid_info->descriptor1.framerate = pixel_clk * 1000 / htotal * 1000 / vtotal;

_dtd2:
	if (p_edid[0x48] == 0 && p_edid[0x49] == 0) {
		rx_pr("descriptor2 is not a DTD!\n");
		return ret;
	}
	ret = true;
	edid_info->descriptor2.pixel_clk = (p_edid[0x48] + (p_edid[0x49] << 8)) / 100;
	pixel_clk = edid_info->descriptor2.pixel_clk;
	edid_info->descriptor2.hactive = p_edid[0x4A] + (((p_edid[0x4C] >> 4) & 0xF) << 8);
		hactive = edid_info->descriptor2.hactive;
	edid_info->descriptor2.vactive = p_edid[0x4D] + (((p_edid[0x4F] >> 4) & 0xF) << 8);
	    vactive = edid_info->descriptor2.vactive;

	edid_info->descriptor2.hblank = ((p_edid[0x4C] & 0xF) << 8) + p_edid[0x4B];
	hblank = edid_info->descriptor2.hblank;
	edid_info->descriptor2.vblank = ((p_edid[0x4F] & 0xF) << 8) + p_edid[0x4E];
	vblank = edid_info->descriptor2.vblank;
	htotal = hactive + hblank;
	vtotal = vactive + vblank;
	edid_info->descriptor2.framerate = pixel_clk * 1000 / htotal * 1000 / vtotal;
	return ret;
}

void rpt_edid_update_dtd(u8 *p_edid)
{
	int i;
	bool checksum_update = false;
	struct edid_info_s edid_info;
	//default 1080P dtd
	u8 def_dtd[18] = {0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58,
					  0x2C, 0x45, 0x00, 0x40, 0x84, 0x63, 0x00, 0x00, 0x1E};

	if (!get_dtd_data(p_edid, &edid_info))
		return;
	if (edid_info.descriptor2.pixel_clk > MAX_PIXEL_CLK ||
		edid_info.descriptor2.hactive > MAX_H_ACTIVE ||
		edid_info.descriptor2.vactive > MAX_V_ACTIVE ||
		edid_info.descriptor2.framerate > MAX_FRAME_RATE) {
		//clear to reserved
		edid_info.descriptor2.pixel_clk = 0;
		for (i = 0; i < 18; i++)
			p_edid[0x48 + i] = 0;
		p_edid[0x4b] = 0x11;
		checksum_update = true;
	}
	if (edid_info.descriptor1.pixel_clk > MAX_PIXEL_CLK ||
		edid_info.descriptor1.hactive > MAX_H_ACTIVE ||
		edid_info.descriptor1.vactive > MAX_V_ACTIVE ||
		edid_info.descriptor1.framerate > MAX_FRAME_RATE) {
		if (edid_info.descriptor2.pixel_clk) {
			//first dtd = second dtd
			for (i = 0; i < 18; i++)
				p_edid[0x36 + i] = p_edid[0x48 + i];
		} else {
			//use default setting
			for (i = 0; i < 18; i++)
				p_edid[0x36 + i] = def_dtd[i];
		}
		checksum_update = true;
	}
	if (checksum_update) {
		p_edid[127] = 0;
		for (i = 0; i < 127; i++) {
			p_edid[127] += p_edid[i];
			p_edid[127] &= 0xFF;
		}
		p_edid[127] = (0x100 - p_edid[127]) & 0xFF;
	}
}

void get_edid_standard_timing_info(u8 *p_edid, struct edid_standard_timing *edid_st_info)
{
	int i;

	if (!p_edid)
		return;

	for (i = 0; i <= 7; i++) {
		if (p_edid[0x26 + i * 2] != 0x01 && p_edid[0x27 + i * 2] != 0x01)
			edid_st_info->refresh_rate[i] = (int)(p_edid[0x27 + i * 2] & 0x3F) + 60;
		edid_st_info->h_active[i] = ((int)(p_edid[0x26 + i * 2]) + 31) * 8;
		edid_st_info->aspect_ratio[i]  = (int)((p_edid[0x27 + i * 2] >> 5) & 0x03);
	}
}

void rpt_edid_update_stand_timing(u8 *p_edid)
{
	u8 i;
	struct edid_standard_timing edid_st_info;

	get_edid_standard_timing_info(p_edid, &edid_st_info);
	for (i = 0; i <= 7; i++) {
		if (edid_st_info.refresh_rate[i] > EDID_MAX_REFRESH_RATE) {
			if (log_level & EDID_LOG)
				rx_pr("sti%d is not supported\n", i);
			p_edid[0x26 + i * 2] = 0x01;
			p_edid[0x27 + i * 2] = 0x01;
		}
	}
}

static void edid_sad_passthrough(unsigned char *p_edid_dest, unsigned char *p_edid_src)
{
	u8 adb_start = 0;

	adb_start = rx_get_cea_tag_offset(p_edid_src, AUDIO_TAG);
	if (adb_start) {
		edid_rm_db_by_tag(p_edid_dest, AUDIO_TAG);
		/* place tv aud data blk to blk index = 0x1 */
		splice_data_blk_to_edid(p_edid_dest, &p_edid_src[adb_start], 0x01);
	}
}

static void edid_secondaryphyaddr(unsigned char *p_edid_dest, unsigned char *p_edid_src)
{
	u8 src_vsdb_offset = 0;
	u8 def_vsdb_offset = 0;
	struct vsdb_s vsdb_src, vsdb_def, vsdb_dest;
	u8 *p_vsdb_src = (u8 *)&vsdb_src;
	u8 *p_vsdb_def = (u8 *)&vsdb_def;

	src_vsdb_offset = rx_get_cea_tag_offset(p_edid_src, VENDOR_TAG);
	def_vsdb_offset = rx_get_cea_tag_offset(p_edid_dest, VENDOR_TAG);
	memcpy(p_vsdb_src + 4, &p_edid_src[src_vsdb_offset + 4], 2);
	memcpy(p_vsdb_def + 4, &p_edid_dest[def_vsdb_offset + 4], 2);
	if (vsdb_src.a == 0) {
		vsdb_dest.b = vsdb_def.b;
		vsdb_dest.a = vsdb_def.a;
		vsdb_dest.d = vsdb_def.d;
		vsdb_dest.c = vsdb_def.c;
	} else if (vsdb_src.b == 0) {
		vsdb_dest.b = vsdb_def.a;
		vsdb_dest.a = vsdb_src.a;
		vsdb_dest.d = 0;
		vsdb_dest.c = 0;
	} else if (vsdb_src.c == 0) {
		vsdb_dest.b = vsdb_src.b;
		vsdb_dest.a = vsdb_src.a;
		vsdb_dest.d = 0;
		vsdb_dest.c = vsdb_def.a;
	} else if (vsdb_src.d == 0) {
		vsdb_dest.b = vsdb_src.b;
		vsdb_dest.a = vsdb_src.a;
		vsdb_dest.d = vsdb_def.a;
		vsdb_dest.c = vsdb_src.c;
	} else {
		vsdb_dest.b = vsdb_src.b;
		vsdb_dest.a = vsdb_src.a;
		vsdb_dest.d = vsdb_src.d;
		vsdb_dest.c = vsdb_src.c;
	}
	p_vsdb_def = (u8 *)&vsdb_dest;
	memcpy(&p_edid_dest[def_vsdb_offset + 4], p_vsdb_def + 4, 2);
	if (log_level & EDID_LOG)
		rx_pr("dest addr:0x%x 0x%x",
			p_edid_dest[def_vsdb_offset + 4], p_edid_dest[def_vsdb_offset + 5]);
}

void rpt_edid_extraction(unsigned char *p_edid)
{
	if (!is_valid_edid_data(edid_tx))
		return;
	if (rpt_edid_selection == use_edid_def_secondary_phyaddr) {
		edid_secondaryphyaddr(p_edid, edid_tx);
		return;
	} else if (rpt_edid_selection == use_edid_def_sad_passthrough_secondary_phyaddr) {
		edid_sad_passthrough(p_edid, edid_tx);
		edid_secondaryphyaddr(p_edid, edid_tx);
		return;
	} else if (rpt_edid_selection == use_edid_def) {
		return;
	}

	//block0
	rpt_edid_update_stand_timing(p_edid);
	rpt_edid_update_dtd(p_edid);
	//block1
	rpt_edid_extension_num_extraction(p_edid);
	/* refer to hdmi2.1 spec 10.3.6 Repeaters that include
	 * an HF-EEODB in their E-EDID shall set the EDID Extension
	 * Block Countfield to a non-zero value . Repeaters that do
	 * not have the ability to forward more than two E-EDID blocks
	 * to theupstream Source shall either (1) remove the HF-EEODB
	 * before forwarding the E-EDID to the Source , or (2) set
	 * the EDID Extension Block Count to 1 in the EDID Extension
	 * Block Count. Option (2) is useful for Repeaters that do
	 * not have the ability to forward more than two E-EDID Blocks
	 * to the Source , and do not wish to reorder the existing
	 * Data Blocks in the E-EDID prior to forwarding
	 */
	rpt_edid_rm_hf_eeodb(p_edid);
	rpt_edid_video_db_extraction(p_edid);
	if (rpt_edid_selection == use_edid_repeater_sad_passthrough)
		edid_sad_passthrough(p_edid, edid_tx);
	else
		rpt_edid_audio_db_extraction(p_edid);
	rpt_edid_14_vs_db_extraction(p_edid);
	rpt_edid_hf_vs_db_extraction(p_edid);
	rpt_edid_420_vdb_extraction(p_edid);
	rpt_edid_colorimetry_db_extraction(p_edid);
	rpt_edid_hdr_static_db_extraction(p_edid);
	rpt_edid_vsv_hdr10plus_extraction(p_edid);
	rpt_edid_vsg_freesync_extraction(p_edid);
	rpt_edid_vsv_db_extraction(p_edid);
}
#endif

u_char rx_edid_calc_cksum(u_char *pedid, u8 blk_num)
{
	u_int i;
	u_int checksum = 0;
	u32 start = blk_num * EDID_BLK_SIZE;
	u32 end = END_OF_BLK(blk_num);

	if (!pedid)
		return 0;
	for (i = start; i <= end; i++) {
		if (i < end) {
			checksum += pedid[i];
			checksum &= 0xFF;
		} else {
			checksum = (0x100 - checksum) & 0xFF;
		}
	}
	return checksum;
}

u_char *rx_get_cur_def_edid(u_char port)
{
	u8 offset = 0;
	enum edid_ver_e ver = get_edid_selection(port);

	if (ver == EDID_V20)
		offset = 1;

	switch (port) {
	case 0:
		memcpy(edid_cur, edid_buf1 + EDID_SIZE * offset, EDID_SIZE);
		break;
	case 1:
		memcpy(edid_cur, edid_buf2 + EDID_SIZE * offset, EDID_SIZE);
		break;
	case 2:
		memcpy(edid_cur, edid_buf3 + EDID_SIZE * offset, EDID_SIZE);
		break;
	case 3:
		memcpy(edid_cur, edid_buf4 + EDID_SIZE * offset, EDID_SIZE);
		break;
	default:
		break;
	}
	return edid_cur;
}

u_char *rx_get_cur_used_edid(u_char port)
{
	u32 i = 0;
	u32 addr = 0;

	if (rx_info.chip_id < CHIP_ID_TM2)
		addr = TOP_EDID_OFFSET; //todo
	else
		addr = edid_addr[port];
	for (i = 0; i < 512; i++)
		edid_cur[i] = hdmirx_rd_top(addr + i, port);

	return &edid_cur[0];
}

void rx_print_edid_support(void)
{
	rx_pr("****Capacity info****\n");
	rx_pr("support vrr: %d\n", rx_info.edid_cap.vrr);
	rx_pr("support allm: %d\n", rx_info.edid_cap.allm);
	rx_pr("support HF_DB: %d\n", rx_info.edid_cap.hf_db);
	rx_pr("support DV_DB: %d\n", rx_info.edid_cap.dv_db);
	rx_pr("support HDR10P_DB: %d\n", rx_info.edid_cap.hdr10p_db);
	rx_pr("support HDR_STATIC_DB: %d\n", rx_info.edid_cap.hdr_static_db);
	rx_pr("support HDR_DYNAMIC_DB: %d\n", rx_info.edid_cap.hdr_dynamic_db);
	rx_pr("support FREESYNC_DB: %d\n", rx_info.edid_cap.freesync_db);
}

void rx_get_edid_support(u8 port)
{
	rx_info.edid_cap.vrr |= get_edid_support(port, HF_VRR);
	rx_info.edid_cap.allm |= get_edid_support(port, HF_ALLM);
	rx_info.edid_cap.hf_db |= get_edid_support(port, HF_DB);
	rx_info.edid_cap.dv_db |= get_edid_support(port, DV_DB);
	rx_info.edid_cap.hdr10p_db |= get_edid_support(port, HDR10P_DB);
	rx_info.edid_cap.hdr_static_db |= get_edid_support(port, HDR_STATIC_DB);
	rx_info.edid_cap.hdr_dynamic_db |= get_edid_support(port, HDR_DYNAMIC_DB);
	rx_info.edid_cap.freesync_db |= get_edid_support(port, FREESYNC_DB);
}

bool hdmi_rx_top_edid_update(void)
{
	u_char *pedid = NULL;
	u_int i = 0;
	u_int j = 0;
	u8 ext_blk_num;
	static int edid_reset_cnt;

	rx_edid_module_reset();
	memset(&rx_info.edid_cap, 0, sizeof(struct edid_capacity));
	while (edid_reset_cnt <= edid_reset_max)
		edid_reset_cnt++;
	edid_reset_cnt = 0;
	for (i = 0; i < rx_info.port_num; i++) {
		pedid = rx_get_cur_def_edid(i);
		if (!is_valid_edid_data(pedid)) {
			rx_pr("port-%d edid err!\n", i);
			return false;
		}
		ext_blk_num = pedid[126];
		rx_edid_update_hdr_dv_info(pedid);
		rx_edid_update_sad(pedid);
		rx_edid_update_vsvdb(pedid, recv_vsvdb, recv_vsvdb_len);
		if (vrr_range_dynamic_update_en)
			rx_edid_update_vrr_info(pedid);
		if (allm_update_en)
			rx_edid_update_allm_info(pedid);
#ifdef CONFIG_AMLOGIC_HDMITX
		rpt_edid_extraction(pedid);
#endif
		for (j = 0; j <= ext_blk_num; ++j)
			pedid[END_OF_BLK(j)] = rx_edid_calc_cksum(pedid, j);
		for (j = 0; j < EDID_SIZE; j++)
			hdmirx_wr_top(edid_addr[i] + j, pedid[j], i);
		rx_get_edid_support(i);
	}
	if (log_level & EDID_LOG)
		rx_print_edid_support();
	return true;
}

void rx_clr_edid_type(unsigned char port)
{
	if (port >= rx_info.port_num)
		return;

	rx[port].edid_type.need_update = false;
	if (log_level & EDID_LOG)
		rx_pr("edid_auto_sel:%d, port:%d, cfg:%d\n",
			edid_auto_sel, port, rx[port].edid_type.cfg);
	switch (rx[port].edid_type.cfg) {
	case EDID_V20:
		rx[port].edid_type.edid_ver = EDID_V20;
		break;
	case EDID_AUTO20:
		if (rx[port].tx_type == DEV_HDMI14)
			rx[port].edid_type.edid_ver = EDID_V14;
		else
			rx[port].edid_type.edid_ver = EDID_V20;
		break;
	case EDID_AUTO14:
		if (rx[port].tx_type == DEV_HDMI20)
			rx[port].edid_type.edid_ver = EDID_V20;
		else
			rx[port].edid_type.edid_ver = EDID_V14;
		break;
	case EDID_V14:
	default:
		rx[port].edid_type.edid_ver = EDID_V14;
		break;
	}
}

void edid_type_init(void)
{
	unsigned char i = 0;

	for (i = 0; i < rx_info.port_num; i++)
		rx_clr_edid_type(i);
}

void edid_type_update(u8 port)
{
	switch (rx[port].edid_type.cfg) {
	case EDID_V14:
	case EDID_V20:
		break;
	case EDID_AUTO14:
		if ((edid_auto_sel & rx[port].edid_type.cfg) == 0)
			break;
		if (rx[port].tx_type == DEV_HDMI20 && rx[port].edid_type.edid_ver != EDID_V20) {
			rx[port].edid_type.edid_ver = EDID_V20;
			rx[port].edid_type.need_update = true;
		}
		break;
	case EDID_AUTO20:
		if ((edid_auto_sel & rx[port].edid_type.cfg) == 0)
			break;
		if (rx[port].tx_type == DEV_HDMI14 && rx[port].edid_type.edid_ver != EDID_V14) {
			rx[port].edid_type.edid_ver = EDID_V14;
			rx[port].edid_type.need_update = true;
		} else if (rx[port].tx_type == DEV_ABNORMAL_SCDC) {
			if (port == rx_info.main_port) {
				if (rx[port].edid_type.edid_ver != EDID_V20) {
					rx[port].edid_type.edid_ver = EDID_V20;
					rx[port].edid_type.need_update = true;
				}
				rx[port].tx_type = DEV_HDMI20;
			} else {
				if (rx[port].edid_type.edid_ver != EDID_V14) {
					rx[port].edid_type.edid_ver = EDID_V14;
					rx[port].edid_type.need_update = true;
				}
			}
		} else {
			if (rx[port].edid_type.edid_ver != EDID_V20) {
				rx[port].edid_type.edid_ver = EDID_V20;
				rx[port].edid_type.need_update = true;
			}
		}
		break;
	default:
		break;
	}
	if (rx[port].edid_type.need_update) {
		edid_update_dwork.port = port;
		schedule_work(&edid_update_dwork.work);
		rx[port].edid_type.need_update = false;
	}
}

