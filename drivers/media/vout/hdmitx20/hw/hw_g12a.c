// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/printk.h>
#include <linux/pinctrl/devinfo.h>
#include "common.h"
#include "mach_reg.h"
#include "../../hdmitx_common/hdmitx_compliance.h"

/*
 * NAME		PAD		PINMUX		GPIO
 * HPD		GPIOH_1		reg0[23]	GPIO1[21]
 * SCL		GPIOH_2		reg0[22]	GPIO1[22[
 * SDA		GPIOH_3		reg0[21]	GPIO1[23]
 */

#ifdef P_HHI_HDMI_PLL_CNTL
#undef P_HHI_HDMI_PLL_CNTL
#endif
#ifdef P_HHI_HDMI_PLL_CNTL2
#undef P_HHI_HDMI_PLL_CNTL2
#endif
#ifdef P_HHI_HDMI_PLL_CNTL3
#undef P_HHI_HDMI_PLL_CNTL3
#endif
#ifdef P_HHI_HDMI_PLL_CNTL4
#undef P_HHI_HDMI_PLL_CNTL4
#endif
#ifdef P_HHI_HDMI_PLL_CNTL5
#undef P_HHI_HDMI_PLL_CNTL5
#endif
#ifdef P_HHI_HDMI_PLL_CNTL6
#undef P_HHI_HDMI_PLL_CNTL6
#endif
#ifdef HHI_HDMI_PLL_CNTL_I
#undef HHI_HDMI_PLL_CNTL_I
#endif
#ifdef P_HHI_HDMI_PLL_CNTL_I
#undef P_HHI_HDMI_PLL_CNTL_I
#endif
#ifdef WAIT_FOR_PLL_LOCKED
#undef WAIT_FOR_PLL_LOCKED
#endif

#define WAIT_FOR_PLL_LOCKED(_reg) \
	do { \
		unsigned int st = 0; \
		unsigned int reg = 0; \
		int cnt = 10; \
		reg = _reg; \
		while (cnt--) { \
			usleep_range(40, 50); \
			st = (((hd_read_reg(reg) >> 30) & 0x3) == 3); \
			if (st) \
				break; \
			else { \
				/* reset hpll */ \
				hd_set_reg_bits(reg, 1, 29, 1); \
				hd_set_reg_bits(reg, 0, 29, 1); \
			} \
		} \
		if (cnt < 9) \
			HDMITX_INFO("pll[0x%x] reset %d times\n", reg, 9 - cnt);\
	} while (0)

#define P_HHI_HDMI_PLL_CNTL HHI_REG_ADDR(0xc8)
#define P_HHI_HDMI_PLL_CNTL0 P_HHI_HDMI_PLL_CNTL
#define P_HHI_HDMI_PLL_CNTL1 HHI_REG_ADDR(0xc9)
#define P_HHI_HDMI_PLL_CNTL2 HHI_REG_ADDR(0xca)
#define P_HHI_HDMI_PLL_CNTL3 HHI_REG_ADDR(0xcb)
#define P_HHI_HDMI_PLL_CNTL4 HHI_REG_ADDR(0xcc)
#define P_HHI_HDMI_PLL_CNTL5 HHI_REG_ADDR(0xcd)
#define P_HHI_HDMI_PLL_CNTL6 HHI_REG_ADDR(0xce)
#define P_HHI_HDMI_PLL_STS HHI_REG_ADDR(0xcf)

/*
 * When VCO outputs 6.0 GHz, if VCO unlock with default v1
 * steps, then need reset with v2 or v3
 */
static bool set_hpll_hclk_v1(unsigned int m, unsigned int frac_val)
{
	int ret = 0;
	struct hdmitx_dev *hdev = get_hdmitx_device();
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;

	hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x0b3a0400 | (m & 0xff));
	hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x3, 28, 2);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL1, frac_val);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);

	if (frac_val == 0x8168) {
		if ((para->vic == HDMI_96_3840x2160p50_16x9 ||
		     para->vic == HDMI_97_3840x2160p60_16x9 ||
		     para->vic == HDMI_106_3840x2160p50_64x27 ||
		     para->vic == HDMI_107_3840x2160p60_64x27) &&
		     para->cs != HDMI_COLORSPACE_YUV420) {
			hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x6a685c00);
			hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x11551293);
		} else {
			hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x6a685c00);
			hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x44331290);
		}
	} else {
		if (hdev->tx_hw.chip_data->chip_type == MESON_CPU_ID_SM1 &&
		    hdmitx_find_vendor_6g(hdev->tx_comm.EDID_buf) &&
		    (para->vic == HDMI_96_3840x2160p50_16x9 ||
		    para->vic == HDMI_97_3840x2160p60_16x9 ||
		    para->vic == HDMI_106_3840x2160p50_64x27 ||
		    para->vic == HDMI_107_3840x2160p60_64x27 ||
		    para->vic == HDMI_101_4096x2160p50_256x135 ||
		    para->vic == HDMI_102_4096x2160p60_256x135) &&
		    para->cs != HDMI_COLORSPACE_YUV420) {
			hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x6a685c00);
			hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x11551293);
		} else {
			hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x6a68dc00);
			hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x65771290);
		}
	}
	hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39272000);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x56540000);
	hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
	WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
	HDMITX_INFO("HPLLv1: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));

	ret = (((hd_read_reg(P_HHI_HDMI_PLL_CNTL0) >> 30) & 0x3) == 0x3);
	return ret; /* return hpll locked status */
}

static bool set_hpll_hclk_v2(unsigned int m, unsigned int frac_val)
{
	int ret = 0;

	hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x0b3a0400 | (m & 0xff));
	hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x3, 28, 2);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL1, frac_val);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0xea68dc00);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x65771290);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39272000);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x56540000);
	hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
	WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
	HDMITX_INFO("HPLLv2: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));

	ret = (((hd_read_reg(P_HHI_HDMI_PLL_CNTL0) >> 30) & 0x3) == 0x3);
	return ret; /* return hpll locked status */
}

static bool set_hpll_hclk_v3(unsigned int m, unsigned int frac_val)
{
	int ret = 0;

	hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x0b3a0400 | (m & 0xff));
	hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x3, 28, 2);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL1, frac_val);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0xea68dc00);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x65771290);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39272000);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x55540000);
	hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
	WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
	HDMITX_INFO("HPLLv3: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));

	ret = (((hd_read_reg(P_HHI_HDMI_PLL_CNTL0) >> 30) & 0x3) == 0x3);
	return ret; /* return hpll locked status */
}

static inline int is_dongle_mode(struct hdmitx_dev *hdev)
{
	struct hdmi_format_para *para = &hdev->tx_comm.fmt_para;

	return hdev->tx_hw.dongle_mode &&
		(para->cs == HDMI_COLORSPACE_YUV422 ||
		para->cd == COLORDEPTH_24B) &&
		(hdev->tx_comm.fmt_para.vic == HDMI_19_1280x720p50_16x9 ||
		 hdev->tx_comm.fmt_para.vic == HDMI_4_1280x720p60_16x9 ||
		 hdev->tx_comm.fmt_para.vic == HDMI_5_1920x1080i60_16x9 ||
		 hdev->tx_comm.fmt_para.vic == HDMI_20_1920x1080i50_16x9 ||
		 hdev->tx_comm.fmt_para.vic == HDMI_16_1920x1080p60_16x9 ||
		 hdev->tx_comm.fmt_para.vic == HDMI_31_1920x1080p50_16x9);
}

static void set_hpll_hclk_dongle_5940m(void)
{
	hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x0b3a04f7);
	hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x3, 28, 2);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x10000);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00100140);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x2a295c00);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x65771290);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39272000);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
	HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
	hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
	HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
	WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
	HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
}

static void g12a_auto_set_hpll(u32 clk)
{
	u32 quotient;
	u32 remainder;
	u32 rem_1;
	u32 rem_2;

	if (clk < 3000000 || clk >= 6000000) {
		HDMITX_ERROR("%s[%d] clock should be 3~6G\n", __func__, __LINE__);
		return;
	}

	quotient = clk / 24000;
	remainder = clk - quotient * 24000;
	/* remainder range: 0 ~ 99999, 0x1869f, 17bits */
	/* convert remainder to 0 ~ 2^17 */
	if (remainder) {
		rem_1 = remainder / 16;
		rem_2 = remainder - rem_1 * 16;
		rem_1 *= 1 << 17;
		rem_1 /= 1500;
		rem_2 *= 1 << 13;
		rem_2 /= 1500;
		remainder = rem_1 + rem_2;
	}

	hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b000400 | (quotient & 0x1ff));
	hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x3, 28, 2);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL1, remainder);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x6a685c00);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x3927200a);
	hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x55540000);
	hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
	WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
}

void set_g12a_hpll_clk_out(unsigned int frac_rate, unsigned int clk)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	switch (clk) {
	case 5940000:
		if (is_dongle_mode(hdev)) {
			set_hpll_hclk_dongle_5940m();
			break;
		}
		if (set_hpll_hclk_v1(0xf7, frac_rate ? 0x8168 : 0x10000))
			break;
		if (set_hpll_hclk_v2(0x7b, 0x18000))
			break;
		if (set_hpll_hclk_v3(0xf7, 0x10000))
			break;
		break;
	case 5850000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004f3);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00018000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 5680000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004ec);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00015555);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 5600000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004e9);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x0000aaab);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);/*test*/
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 5405400:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004e1);
		if (frac_rate)
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00000000);
		else
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00007333);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 5200000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004d8);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00015555);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 4897000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004cc);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x0000d560);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x6a685c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x43231290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x29272000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x56540028);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 4830000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004c9);
		if (frac_rate)
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00008000);
		else
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00001910);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x6a685c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x43231290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x29272000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x56540028);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 4455000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004b9);
		if (frac_rate)
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x0000e10e);
		else
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00014000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x6a685c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x43231290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x29272000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x56540028);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 4324320:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004b4);
		if (frac_rate)
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00000000);
		else
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00005c29);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 4320000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004b4);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 4260000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004b1);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00010000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 3712500:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b00049a);
		if (frac_rate)
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x000110e1);
		else
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00016000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x6a685c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x43231290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x29272000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x56540028);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 3450000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b00048f);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00018000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 3420000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b00048e);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00010000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 3243240:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b000487);
		if (frac_rate)
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00000000);
		else
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x0000451f);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 3200000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b000485);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x0000aaab);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 3197500:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b000485);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00007555);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 3180000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b000484);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00010000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 2970000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b00047b);
		if (frac_rate)
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x000140b4);
		else
			hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x00018000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 4115866:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004ab);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x0000fd22);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	case 4028000:
		hd_write_reg(P_HHI_HDMI_PLL_CNTL0, 0x3b0004a7);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL1, 0x0001aa80);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL2, 0x00000000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL3, 0x0a691c00);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL4, 0x33771290);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL5, 0x39270000);
		hd_write_reg(P_HHI_HDMI_PLL_CNTL6, 0x50540000);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_HHI_HDMI_PLL_CNTL0);
		HDMITX_INFO("HPLL: 0x%x\n", hd_read_reg(P_HHI_HDMI_PLL_CNTL0));
		break;
	default:
		HDMITX_INFO("error hpll clk: %d\n", clk);
		g12a_auto_set_hpll(clk);
		break;
	}
}

void set_hpll_od1_g12a(unsigned int div)
{
	switch (div) {
	case 1:
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0, 16, 2);
		break;
	case 2:
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 1, 16, 2);
		break;
	case 4:
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 2, 16, 2);
		break;
	default:
		break;
	}
}

void set_hpll_od2_g12a(unsigned int div)
{
	switch (div) {
	case 1:
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0, 18, 2);
		break;
	case 2:
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 1, 18, 2);
		break;
	case 4:
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 2, 18, 2);
		break;
	default:
		break;
	}
}

void set_hpll_od3_g12a(unsigned int div)
{
	switch (div) {
	case 1:
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 0, 20, 2);
		break;
	case 2:
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 1, 20, 2);
		break;
	case 4:
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL, 2, 20, 2);
		break;
	default:
		HDMITX_INFO("Err %s[%d]\n", __func__, __LINE__);
		break;
	}
}

int hdmitx_hpd_hw_op_g12a(enum hpd_op cmd)
{
	int ret = 0;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (!(hdev->pdev)) {
		HDMITX_INFO("exit for null device of hdmitx!\n");
		return -ENODEV;
	}

	if (!(hdev->pdev->pins)) {
		HDMITX_INFO("exit for null pins of hdmitx device!\n");
		return -ENODEV;
	}

	if (!(hdev->pdev->pins->p)) {
		HDMITX_INFO("exit for null pinctrl of hdmitx device pins!\n");
		return -ENODEV;
	}

	switch (cmd) {
	case HPD_INIT_SET_FILTER:
		hdmitx_wr_reg(HDMITX_TOP_HPD_FILTER,
			      ((0xa << 12) | (0xa0 << 0)));
		break;
	case HPD_IS_HPD_MUXED:
		ret = 1;
		break;
	case HPD_MUX_HPD:
		pinctrl_select_state(hdev->pdev->pins->p,
				     hdev->pinctrl_default);
		break;
	case HPD_UNMUX_HPD:
		pinctrl_select_state(hdev->pdev->pins->p, hdev->pinctrl_i2c);
		break;
	case HPD_READ_HPD_GPIO:
		ret = hdmitx_rd_reg(HDMITX_DWC_PHY_STAT0) & (1 << 1);
		break;
	default:
		break;
	}
	return ret;
}

void set_hpll_sspll_g12a(enum hdmi_vic vic)
{
	struct hdmitx_dev *hdev = get_hdmitx_device();

	switch (vic) {
	case HDMI_16_1920x1080p60_16x9:
	case HDMI_31_1920x1080p50_16x9:
	case HDMI_4_1280x720p60_16x9:
	case HDMI_19_1280x720p50_16x9:
	case HDMI_5_1920x1080i60_16x9:
	case HDMI_20_1920x1080i50_16x9:
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 1, 29, 1);
		/* bit[22:20] hdmi_dpll_fref_sel
		 * bit[8] hdmi_dpll_ssc_en
		 * bit[7:4] hdmi_dpll_ssc_dep_sel
		 */
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 1, 20, 3);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 1, 8, 1);
		/* 2: 1000ppm  1: 500ppm */
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 2, 4, 4);
		if (hdev->tx_hw.dongle_mode)
			hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL2, 4, 4, 4);
		/* bit[15] hdmi_dpll_sdmnc_en */
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL3, 0, 15, 1);
		hd_set_reg_bits(P_HHI_HDMI_PLL_CNTL0, 0, 29, 1);
		break;
	default:
		break;
	}
}

void hdmitx_phy_bandgap_en_g12(void)
{
	unsigned int val = 0;

	val = hd_read_reg(P_HHI_HDMI_PHY_CNTL0);
	if (val == 0)
		hd_write_reg(P_HHI_HDMI_PHY_CNTL0, 0x0b4242);
}

int hdmitx_ddc_hw_op_g12a(enum ddc_op cmd)
{
	int ret = 0;
	struct hdmitx_dev *hdev = get_hdmitx_device();

	if (!(hdev->pdev)) {
		HDMITX_INFO("exit for null device of hdmitx!\n");
		return -ENODEV;
	}

	if (!(hdev->pdev->pins)) {
		HDMITX_INFO("exit for null pins of hdmitx device!\n");
		return -ENODEV;
	}

	if (!(hdev->pdev->pins->p)) {
		HDMITX_INFO("exit for null pinctrl of hdmitx device pins!\n");
		return -ENODEV;
	}

	switch (cmd) {
	case DDC_MUX_DDC:
		pinctrl_select_state(hdev->pdev->pins->p,
				     hdev->pinctrl_default);
		break;
	case DDC_UNMUX_DDC:
		pinctrl_select_state(hdev->pdev->pins->p, hdev->pinctrl_i2c);
		break;
	case DDC_INIT_DISABLE_PULL_UP_DN:
		/* For s4/sc2/g12 chips, the pins of DDC is OD(open drain),
		 * there is no need to configure the internal pull up or down.
		 * Just left blank here
		 */
		break;
	default:
		HDMITX_ERROR("error ddc cmd %d\n", cmd);
	}
	return ret;
}

void set_phy_by_mode_g12(unsigned int mode)
{
	switch (mode) {
	case HDMI_PHYPARA_6G: /* 5.94Gbps */
		hd_write_reg(P_HHI_HDMI_PHY_CNTL5, 0x0000080b);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL0, 0x37eb76d4);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL3, 0x2ab0ff3b);
		break;
	case HDMI_PHYPARA_4p5G: /* 4.5Gbps*/
		hd_write_reg(P_HHI_HDMI_PHY_CNTL5, 0x0000080b);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL0, 0x37eb65d4);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL3, 0x2ab0ff3b);
		break;
	case HDMI_PHYPARA_3p7G: /* 3.7Gbps */
		hd_write_reg(P_HHI_HDMI_PHY_CNTL5, 0x0000080b);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL0, 0x37eb65c4);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL3, 0x2ab0ff3b);
		break;
	case HDMI_PHYPARA_3G: /* 2.97Gbps */
		hd_write_reg(P_HHI_HDMI_PHY_CNTL5, 0x00000003);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL0, 0x33eb6272);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL3, 0x2ab0ff3b);
		break;
	case HDMI_PHYPARA_270M: /* SD format, 480p/576p, 270Mbps */
		hd_write_reg(P_HHI_HDMI_PHY_CNTL5, 0x00000003);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL0, 0x33eb5252);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL3, 0x2ab0ff3b);
		break;
	case HDMI_PHYPARA_DEF: /* less than 2.97G */
	default:
		hd_write_reg(P_HHI_HDMI_PHY_CNTL5, 0x00000003);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL0, 0x33eb4262);
		hd_write_reg(P_HHI_HDMI_PHY_CNTL3, 0x2ab0ff3b);
		break;
	}
}

