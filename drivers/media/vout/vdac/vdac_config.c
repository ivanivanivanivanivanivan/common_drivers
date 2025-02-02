// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/vout/vdac/vdac_config.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/platform_device.h>
#include <linux/of_device.h>
#include "vdac_dev.h"

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static struct meson_vdac_ctrl_s vdac_ctrl_enable_g12ab[] = {
	{HHI_VDAC_CNTL0_G12A, 0, 9, 1},
	{HHI_VDAC_CNTL0_G12A, 2, 0, 3},
	{HHI_VDAC_CNTL0_G12A, 0, 16, 5}, /* vref adj */
	{HHI_VDAC_CNTL0_G12A, 1, 24, 1}, /* clk phase sel */
	{HHI_VDAC_CNTL1_G12A, 0, 0, 3},  /*gsw */
	{HHI_VDAC_CNTL1_G12A, 0, 3, 1},
	{VDAC_REG_MAX, 0, 0, 0},
};

static struct meson_vdac_ctrl_s vdac_ctrl_enable_tl1[] = {
	{HHI_VDAC_CNTL0_G12A, 0, 9, 1},
	{HHI_VDAC_CNTL0_G12A, 2, 0, 3},
	{HHI_VDAC_CNTL1_G12A, 0, 3, 1},
	{HHI_VDAC_CNTL1_G12A, 0, 7, 1}, /* bandgap */
	{HHI_VDAC_CNTL0_G12A, 0, 16, 5}, /* vref adj */
	{HHI_VDAC_CNTL0_G12A, 1, 24, 1}, /* clk phase sel */
	{HHI_VDAC_CNTL1_G12A, 0, 0, 3},  /*gsw */
	{HHI_VDAC_CNTL1_G12A, 1, 6, 1}, /* bypass avout */
	{HHI_VDAC_CNTL1_G12A, 1, 8, 1}, /* bypass avout */
	{VDAC_REG_MAX, 0, 0, 0},
};

static struct meson_vdac_ctrl_s vdac_ctrl_enable_sc2[] = {
	{ANACTRL_VDAC_CTRL0, 0, 9, 1},
	{ANACTRL_VDAC_CTRL0, 2, 0, 3},
	{ANACTRL_VDAC_CTRL0, 0, 16, 5}, /* vref adj */
	{ANACTRL_VDAC_CTRL0, 1, 24, 1}, /* clk phase sel */
	{ANACTRL_VDAC_CTRL1, 0, 0, 3},  /*gsw */
	{ANACTRL_VDAC_CTRL1, 0, 3, 1},
	{VDAC_REG_MAX, 0, 0, 0},
};

static struct meson_vdac_ctrl_s vdac_ctrl_enable_t5[] = {
	{HHI_VDAC_CNTL0_G12A, 0, 9, 1},
	{HHI_VDAC_CNTL0_G12A, 2, 0, 3},
	{HHI_VDAC_CNTL1_G12A, 0x0, 0, 7}, /*gsw t5 no verf*/
	{HHI_VDAC_CNTL1_G12A, 1, 7, 1}, /* cdac_pwd */
	{VDAC_REG_MAX, 0, 0, 0},
};
#endif

static struct meson_vdac_ctrl_s vdac_ctrl_enable_s4[] = {
	/* byp_bias<1:0> */
	{ANACTRL_VDAC_CTRL0, 0, 9, 1},
	/* cdac_ctrl_rsv1[7:0] clk_delay_adj<2:0> */
	{ANACTRL_VDAC_CTRL0, 2, 0, 3},
	/* cdac_pwd, 1: on, 0: off */
	{ANACTRL_VDAC_CTRL1, 1, 7, 1},
	{VDAC_REG_MAX, 0, 0, 0},
};

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static struct meson_vdac_ctrl_s vdac_ctrl_enable_s7[] = {
	/* cdac_ctrl_rsv1[7:0] clk_delay_adj<2:0> */
	{ANACTRL_VDAC_CTRL0, 2, 0, 3},
	/* cdac_pwd, 1: on, 0: off */
	{ANACTRL_VDAC_CTRL1, 1, 7, 1},
	{VDAC_REG_MAX, 0, 0, 0},
};

static struct meson_vdac_ctrl_s vdac_ctrl_enable_s7d[] = {
	/* cdac_ctrl_rsv1[7:0] clk_delay_adj<2:0> */
	{ANACTRL_VDAC_CTRL0, 2, 0, 3},
	/* cdac_pwd, 1: on, 0: off */
	{ANACTRL_VDAC_CTRL1, 1, 7, 1},
	{VDAC_REG_MAX, 0, 0, 0},
};

static struct meson_vdac_ctrl_s vdac_ctrl_enable_t5m[] = {
	{ANACTRL_VDAC_CTRL0, 1, 4, 1}, /* chopper_clk_h */
	{ANACTRL_VDAC_CTRL1, 1, 7, 1}, /* cdac_en */
	{ANACTRL_VDAC_CTRL1, 1, 27, 1}, /* cdac_data_reg_en */
	{ANACTRL_VDAC_CTRL1, 0x230, 16, 10}, /* cdac_data_reg */
	{VDAC_REG_MAX, 0, 0, 0},
};

/* ********************************************************* */
static struct meson_vdac_data meson_g12ab_vdac_data = {
	.cpu_id = VDAC_CPU_G12AB,
	.name = "meson-g12ab-vdac",

	.reg_cntl0 = HHI_VDAC_CNTL0_G12A,
	.reg_cntl1 = HHI_VDAC_CNTL1_G12A,
	.reg_vid_clk_ctrl2 = HHI_VID_CLK_CNTL2,
	.reg_vid2_clk_div = HHI_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_g12ab,
};

static struct meson_vdac_ctrl_s vdac_ctrl_enable_txhd2[] = {
	{HHI_VDAC_CNTL0_G12A, 1, 4, 1}, /* chopper_clk_h */
	{HHI_VDAC_CNTL1_G12A, 1, 7, 1}, /* cdac_en */
	{VDAC_REG_MAX, 0, 0, 0},
};
#endif

#ifndef CONFIG_AMLOGIC_REMOVE_OLD
static struct meson_vdac_data meson_tl1_vdac_data = {
	.cpu_id = VDAC_CPU_TL1,
	.name = "meson-tl1-vdac",

	.reg_cntl0 = HHI_VDAC_CNTL0_G12A,
	.reg_cntl1 = HHI_VDAC_CNTL1_G12A,
	.reg_vid_clk_ctrl2 = HHI_VID_CLK_CNTL2,
	.reg_vid2_clk_div = HHI_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_tl1,
};
#endif

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static struct meson_vdac_data meson_sm1_vdac_data = {
	.cpu_id = VDAC_CPU_SM1,
	.name = "meson-sm1-vdac",

	.reg_cntl0 = HHI_VDAC_CNTL0_G12A,
	.reg_cntl1 = HHI_VDAC_CNTL1_G12A,
	.reg_vid_clk_ctrl2 = HHI_VID_CLK_CNTL2,
	.reg_vid2_clk_div = HHI_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_g12ab,
};

static struct meson_vdac_data meson_tm2_vdac_data = {
	.cpu_id = VDAC_CPU_TM2,
	.name = "meson-tm2-vdac",

	.reg_cntl0 = HHI_VDAC_CNTL0_G12A,
	.reg_cntl1 = HHI_VDAC_CNTL1_G12A,
	.reg_vid_clk_ctrl2 = HHI_VID_CLK_CNTL2,
	.reg_vid2_clk_div = HHI_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_tl1,
};

static struct meson_vdac_data meson_sc2_vdac_data = {
	.cpu_id = VDAC_CPU_SC2,
	.name = "meson-sc2-vdac",

	.reg_cntl0 = ANACTRL_VDAC_CTRL0,
	.reg_cntl1 = ANACTRL_VDAC_CTRL1,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_sc2,
};

static struct meson_vdac_data meson_t5_vdac_data = {
	.cpu_id = VDAC_CPU_T5,
	.name = "meson-t5-vdac",

	.reg_cntl0 = HHI_VDAC_CNTL0_G12A,
	.reg_cntl1 = HHI_VDAC_CNTL1_G12A,
	.reg_vid_clk_ctrl2 = HHI_VID_CLK_CNTL2,
	.reg_vid2_clk_div = HHI_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_t5,
};

static struct meson_vdac_data meson_t5d_vdac_data = {
	.cpu_id = VDAC_CPU_T5D,
	.name = "meson-t5d-vdac",

	.reg_cntl0 = HHI_VDAC_CNTL0_G12A,
	.reg_cntl1 = HHI_VDAC_CNTL1_G12A,
	.reg_vid_clk_ctrl2 = HHI_VID_CLK_CNTL2,
	.reg_vid2_clk_div = HHI_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_t5,
};

static struct meson_vdac_data meson_s4_vdac_data = {
	.cpu_id = VDAC_CPU_S4,
	.name = "meson-s4-vdac",

	.reg_cntl0 = ANACTRL_VDAC_CTRL0,
	.reg_cntl1 = ANACTRL_VDAC_CTRL1,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_s4,
};

static struct meson_vdac_data meson_t3_vdac_data = {
	.cpu_id = VDAC_CPU_T3,
	.name = "meson-t3-vdac",

	.reg_cntl0 = ANACTRL_VDAC_CTRL0,
	.reg_cntl1 = ANACTRL_VDAC_CTRL1,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_s4,
};
#endif

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT_C1A
static struct meson_vdac_data meson_s4d_vdac_data = {
	.cpu_id = VDAC_CPU_S4D,
	.name = "meson-s4d-vdac",

	.reg_cntl0 = ANACTRL_VDAC_CTRL0,
	.reg_cntl1 = ANACTRL_VDAC_CTRL1,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_s4,
};
#endif

static struct meson_vdac_data meson_s1a_vdac_data = {
	.cpu_id = VDAC_CPU_S1A,
	.name = "meson-s1a-vdac",

	.reg_cntl0 = ANACTRL_VDAC_CTRL0,
	.reg_cntl1 = ANACTRL_VDAC_CTRL1,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_s4,
	.bypass_cfg_cntl0 = 0x00418982, //vlsi suggestion value
	.cvbsout_cfg_cntl0 = 0x00418982, //vlsi suggestion value
};

#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
static struct meson_vdac_data meson_t5w_vdac_data = {
	.cpu_id = VDAC_CPU_T5W,
	.name = "meson-t5w-vdac",

	.reg_cntl0 = HHI_VDAC_CNTL0_G12A,
	.reg_cntl1 = HHI_VDAC_CNTL1_G12A,
	.reg_vid_clk_ctrl2 = HHI_VID_CLK0_CTRL2,
	.reg_vid2_clk_div = HHI_VIID_CLK0_DIV,
	.ctrl_table = vdac_ctrl_enable_t5,
};

static struct meson_vdac_data meson_t5m_vdac_data = {
	.cpu_id = VDAC_CPU_T5M,
	.name = "meson-t5m-vdac",

	.reg_cntl0 = ANACTRL_VDAC_CTRL0,
	.reg_cntl1 = ANACTRL_VDAC_CTRL1,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_t5m,
	.cdac_disable = 1,
	.bypass_cfg_cntl0 = 0x00416901, //vlsi suggestion value
	.cvbsout_cfg_cntl0 = 0x00410a01, //vlsi suggestion value
};

static struct meson_vdac_data meson_t3x_vdac_data = {
	.cpu_id = VDAC_CPU_T3X,
	.name = "meson-t3x-vdac",

	.reg_cntl0 = ANACTRL_VDAC_CTRL0,
	.reg_cntl1 = ANACTRL_VDAC_CTRL1,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_t5m,
	.cdac_disable = 1,
	.bypass_cfg_cntl0 = 0x00416901, //vlsi suggestion value
	.cvbsout_cfg_cntl0 = 0x00410a01, //vlsi suggestion value
};

static struct meson_vdac_data meson_txhd2_vdac_data = {
	.cpu_id = VDAC_CPU_TXHD2,
	.name = "meson-txhd2-vdac",

	.reg_cntl0 = HHI_VDAC_CNTL0_G12A,
	.reg_cntl1 = HHI_VDAC_CNTL1_G12A,
	.reg_vid_clk_ctrl2 = HHI_VID_CLK_CNTL2,
	.reg_vid2_clk_div = HHI_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_txhd2,
	.cdac_disable = 1,
	.bypass_cfg_cntl0 = 0x00411982, //vlsi suggestion value
	.cvbsout_cfg_cntl0 = 0x00411a82, //vlsi suggestion value
};

static struct meson_vdac_data meson_s7_vdac_data = {
	.cpu_id = VDAC_CPU_S7,
	.name = "meson-s7-vdac",

	.reg_cntl0 = ANACTRL_VDAC_CTRL0,
	.reg_cntl1 = ANACTRL_VDAC_CTRL1,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_s7,
	.bypass_cfg_cntl0 = 0x00419A82, //vlsi suggestion value
	.cvbsout_cfg_cntl0 = 0x00419A82, //vlsi suggestion value
};

static struct meson_vdac_data meson_s7d_vdac_data = {
	.cpu_id = VDAC_CPU_S7D,
	.name = "meson-s7d-vdac",

	.reg_cntl0 = ANACTRL_VDAC_CTRL0,
	.reg_cntl1 = ANACTRL_VDAC_CTRL1,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.ctrl_table = vdac_ctrl_enable_s7d,
	.bypass_cfg_cntl0 = 0x00010A82, //vlsi suggestion value
	.cvbsout_cfg_cntl0 = 0x00010A82, //vlsi suggestion value
};
#endif

const struct of_device_id meson_vdac_dt_match[] = {
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	{
		.compatible = "amlogic, vdac-g12a",
		.data		= &meson_g12ab_vdac_data,
	}, {
		.compatible = "amlogic, vdac-g12b",
		.data		= &meson_g12ab_vdac_data,
	},
#endif
#ifndef CONFIG_AMLOGIC_REMOVE_OLD
	{
		.compatible = "amlogic, vdac-tl1",
		.data		= &meson_tl1_vdac_data,
	},
#endif
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	{
		.compatible = "amlogic, vdac-sm1",
		.data		= &meson_sm1_vdac_data,
	}, {
		.compatible = "amlogic, vdac-tm2",
		.data		= &meson_tm2_vdac_data,
	}, {
		.compatible = "amlogic, vdac-sc2",
		.data		= &meson_sc2_vdac_data,
	}, {
		.compatible = "amlogic, vdac-t5",
		.data		= &meson_t5_vdac_data,
	}, {
		.compatible = "amlogic, vdac-t5d",
		.data		= &meson_t5d_vdac_data,
	}, {
		.compatible = "amlogic, vdac-s4",
		.data		= &meson_s4_vdac_data,
	}, {
		.compatible = "amlogic, vdac-t3",
		.data		= &meson_t3_vdac_data,
	},
#endif
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT_C1A
	{
		.compatible = "amlogic, vdac-s4d",
		.data		= &meson_s4d_vdac_data,
	},
#endif
	{
		.compatible = "amlogic, vdac-s1a",
		.data		= &meson_s1a_vdac_data,
	},
#ifndef CONFIG_AMLOGIC_ZAPPER_CUT
	{
		.compatible = "amlogic, vdac-t5w",
		.data		= &meson_t5w_vdac_data,
	}, {
		.compatible = "amlogic, vdac-t5m",
		.data		= &meson_t5m_vdac_data,
	}, {
		.compatible = "amlogic, vdac-t3x",
		.data		= &meson_t3x_vdac_data,
	}, {
		.compatible = "amlogic, vdac-txhd2",
		.data		= &meson_txhd2_vdac_data,
	},
	{
		.compatible = "amlogic, vdac-s7",
		.data		= &meson_s7_vdac_data,
	},
	{
		.compatible = "amlogic, vdac-s7d",
		.data = &meson_s7d_vdac_data,
	},
#endif
	{}
};

struct meson_vdac_data *aml_vdac_config_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct meson_vdac_data *data;
	unsigned int value;
	int ret;

	match = of_match_device(meson_vdac_dt_match, &pdev->dev);
	if (!match) {
		pr_err("%s, no matched table\n", __func__);
		return NULL;
	}
	data = (struct meson_vdac_data *)match->data;
	if (data) {
		pr_info("%s: cpu_id:%d, name:%s\n", __func__,
			data->cpu_id, data->name);
	}

	/* cvbsout used earphone disable cdac default */
	ret = of_property_read_u32(pdev->dev.of_node, "cdac_disable", &value);
	if (ret) {
		pr_info("not get cdac_disable.\n");
		data->cdac_disable = 0;
	} else {
		data->cdac_disable = value;
	}

	return data;
}
