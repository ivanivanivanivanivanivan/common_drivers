// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * drivers/amlogic/media/video_sink/video_keeper.c
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>

#include <linux/amlogic/media/vfm/vframe.h>
#include <linux/amlogic/media/vfm/vframe_provider.h>
#include <linux/amlogic/media/vfm/vframe_receiver.h>
#ifdef CONFIG_AMLOGIC_VOUT
#include <linux/amlogic/media/vout/vout_notify.h>
#endif
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/clk.h>
/*#include <linux/amlogic/gpio-amlogic.h>*/
#include <linux/amlogic/media/canvas/canvas.h>
#include <linux/amlogic/media/canvas/canvas_mgr.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>
#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
#include <linux/amlogic/media/ge2d/ge2d.h>
#include <linux/amlogic/media/canvas/canvas_mgr.h>
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
#include <linux/amlogic/media/frame_sync/ptsserv.h>
#include <linux/amlogic/media/frame_sync/timestamp.h>
#include <linux/amlogic/media/frame_sync/tsync.h>
#endif

#include <linux/amlogic/media/codec_mm/codec_mm.h>
#include <linux/amlogic/media/codec_mm/codec_mm_keeper.h>

#include "video_priv.h"
#include <linux/amlogic/media/video_sink/video_keeper.h>
#include <linux/amlogic/media/registers/register.h>
#include <linux/amlogic/media/video_sink/video.h>
#include <linux/amlogic/media/utils/vdec_reg.h>

#define MEM_NAME "video-keeper"
static DEFINE_MUTEX(video_keeper_mutex);

static unsigned long keep_y_addr, keep_u_addr, keep_v_addr;
static int keep_video_on[MAX_VD_LAYER];
static int keep_id[MAX_VD_LAYER];
static int keep_head_id[MAX_VD_LAYER];
static int keep_dw_id[MAX_VD_LAYER];
static int keep_el_id[MAX_VD_LAYER];
static int keep_el_head_id[MAX_VD_LAYER];
static int keep_el_dw_id[MAX_VD_LAYER];

#define Y_BUFFER_SIZE   0x600000	/* for 1920*1088 */
#define U_BUFFER_SIZE   0x100000	/* compatible with NV21 */
#define V_BUFFER_SIZE   0x80000

#define RESERVE_CLR_FRAME

static inline ulong keep_phy_addr(unsigned long addr)
{
	return addr;
}

#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
static int display_canvas_y_dup;
static int display_canvas_u_dup;
static int display_canvas_v_dup;
static struct ge2d_context_s *ge2d_video_context;

static int ge2d_videotask_init(void)
{
	const char *keep_owner = "keepframe";

	if (!ge2d_video_context)
		ge2d_video_context = create_ge2d_work_queue();

	if (!ge2d_video_context) {
		pr_info("create_ge2d_work_queue video task failed\n");
		return -1;
	}
	if (!display_canvas_y_dup)
		display_canvas_y_dup = canvas_pool_map_alloc_canvas(keep_owner);
	if (!display_canvas_u_dup)
		display_canvas_u_dup = canvas_pool_map_alloc_canvas(keep_owner);
	if (!display_canvas_v_dup)
		display_canvas_v_dup = canvas_pool_map_alloc_canvas(keep_owner);
	pr_info("create_ge2d_work_queue video task ok\n");

	return 0;
}

static int ge2d_videotask_release(void)
{
	if (ge2d_video_context) {
		destroy_ge2d_work_queue(ge2d_video_context);
		ge2d_video_context = NULL;
	}
	if (display_canvas_y_dup)
		canvas_pool_map_free_canvas(display_canvas_y_dup);
	if (display_canvas_u_dup)
		canvas_pool_map_free_canvas(display_canvas_u_dup);
	if (display_canvas_v_dup)
		canvas_pool_map_free_canvas(display_canvas_v_dup);

	return 0;
}

static int ge2d_store_frame_S_YUV444(u32 cur_index)
{
	u32 y_index, des_index, src_index;
	struct canvas_s cs, cd;
	ulong yaddr;
	u32 ydupindex;

	struct config_para_ex_s ge2d_config;

	memset(&ge2d_config, 0, sizeof(struct config_para_ex_s));

	ydupindex = display_canvas_y_dup;

	pr_info("%s cur_index:s:0x%x\n", __func__, cur_index);
	/* pr_info("ge2d_store_frame cur_index:d:0x%x\n", canvas_tab[0]); */
	y_index = cur_index & 0xff;
	canvas_read(y_index, &cs);

	yaddr = keep_phy_addr(keep_y_addr);
	canvas_config(ydupindex,
		      (ulong)yaddr,
		      cs.width, cs.height, CANVAS_ADDR_NOWRAP, cs.blkmode);

	canvas_read(ydupindex, &cd);
	src_index = y_index;
	des_index = ydupindex;
	if (cs.addr == cd.addr)
		return 1;
	pr_info("ge2d_canvas_dup ADDR srcy[0x%lx] des[0x%lx] des_index[0x%x]\n",
		cs.addr, cd.addr, des_index);

	ge2d_config.alu_const_color = 0;
	ge2d_config.bitmask_en = 0;
	ge2d_config.src1_gb_alpha = 0;

	ge2d_config.src_planes[0].addr = cs.addr;
	ge2d_config.src_planes[0].w = cs.width;
	ge2d_config.src_planes[0].h = cs.height;

	ge2d_config.dst_planes[0].addr = cd.addr;
	ge2d_config.dst_planes[0].w = cd.width;
	ge2d_config.dst_planes[0].h = cd.height;

	ge2d_config.src_para.canvas_index = src_index;
	ge2d_config.src_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.src_para.format = GE2D_FORMAT_S24_YUV444;
	ge2d_config.src_para.fill_color_en = 0;
	ge2d_config.src_para.fill_mode = 0;
	ge2d_config.src_para.color = 0;
	ge2d_config.src_para.top = 0;
	ge2d_config.src_para.left = 0;
	ge2d_config.src_para.width = cs.width;
	ge2d_config.src_para.height = cs.height;
	ge2d_config.src2_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.dst_para.canvas_index = des_index;
	ge2d_config.dst_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.dst_para.format = GE2D_FORMAT_S24_YUV444;
	ge2d_config.dst_para.fill_color_en = 0;
	ge2d_config.dst_para.fill_mode = 0;
	ge2d_config.dst_para.color = 0;
	ge2d_config.dst_para.top = 0;
	ge2d_config.dst_para.left = 0;
	ge2d_config.dst_para.width = cs.width;
	ge2d_config.dst_para.height = cs.height;

	if (ge2d_context_config_ex(ge2d_video_context, &ge2d_config) < 0) {
		pr_info("ge2d_context_config_ex failed\n");
		return -1;
	}

	stretchblt_noalpha(ge2d_video_context, 0, 0, cs.width, cs.height,
			   0, 0, cs.width, cs.height);
	return 0;
}

static int ge2d_store_frame_YUV444(u32 cur_index)
{
	u32 y_index, des_index, src_index;
	struct canvas_s cs, cd;
	ulong yaddr;
	u32 ydupindex;
	struct config_para_ex_s ge2d_config;

	memset(&ge2d_config, 0, sizeof(struct config_para_ex_s));

	ydupindex = display_canvas_y_dup;

	pr_info("%s cur_index:s:0x%x\n", __func__, cur_index);
	/* pr_info("ge2d_store_frame cur_index:d:0x%x\n", canvas_tab[0]); */
	y_index = cur_index & 0xff;
	canvas_read(y_index, &cs);

	yaddr = keep_phy_addr(keep_y_addr);
	canvas_config(ydupindex,
		      (ulong)yaddr,
		      cs.width, cs.height,
		      CANVAS_ADDR_NOWRAP, cs.blkmode);

	canvas_read(ydupindex, &cd);
	src_index = y_index;
	des_index = ydupindex;

	pr_info("ge2d_canvas_dup ADDR srcy[0x%lx] des[0x%lx]\n",
		cs.addr, cd.addr);

	ge2d_config.alu_const_color = 0;
	ge2d_config.bitmask_en = 0;
	ge2d_config.src1_gb_alpha = 0;

	ge2d_config.src_planes[0].addr = cs.addr;
	ge2d_config.src_planes[0].w = cs.width;
	ge2d_config.src_planes[0].h = cs.height;

	ge2d_config.dst_planes[0].addr = cd.addr;
	ge2d_config.dst_planes[0].w = cd.width;
	ge2d_config.dst_planes[0].h = cd.height;

	ge2d_config.src_para.canvas_index = src_index;
	ge2d_config.src_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.src_para.format = GE2D_FORMAT_M24_YUV444;
	ge2d_config.src_para.fill_color_en = 0;
	ge2d_config.src_para.fill_mode = 0;
	ge2d_config.src_para.color = 0;
	ge2d_config.src_para.top = 0;
	ge2d_config.src_para.left = 0;
	ge2d_config.src_para.width = cs.width;
	ge2d_config.src_para.height = cs.height;
	ge2d_config.src2_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.dst_para.canvas_index = des_index;
	ge2d_config.dst_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.dst_para.format = GE2D_FORMAT_M24_YUV444;
	ge2d_config.dst_para.fill_color_en = 0;
	ge2d_config.dst_para.fill_mode = 0;
	ge2d_config.dst_para.color = 0;
	ge2d_config.dst_para.top = 0;
	ge2d_config.dst_para.left = 0;
	ge2d_config.dst_para.width = cs.width;
	ge2d_config.dst_para.height = cs.height;

	if (ge2d_context_config_ex(ge2d_video_context, &ge2d_config) < 0) {
		pr_info("ge2d_context_config_ex failed\n");
		return -1;
	}

	stretchblt_noalpha(ge2d_video_context,
			   0, 0, cs.width, cs.height,
			   0, 0, cs.width, cs.height);

	return 0;
}

/* static u32 canvas_tab[1]; */
static int ge2d_store_frame_NV21(u32 cur_index)
{
	u32 y_index, u_index, des_index, src_index;
	struct canvas_s cs0, cs1, cd;
	ulong yaddr, uaddr;
	u32 ydupindex, udupindex;
	struct config_para_ex_s ge2d_config;

	memset(&ge2d_config, 0, sizeof(struct config_para_ex_s));

	ydupindex = display_canvas_y_dup;
	udupindex = display_canvas_u_dup;

	pr_info("%s cur_index:s:0x%x\n", __func__, cur_index);

	/* pr_info("ge2d_store_frame cur_index:d:0x%x\n", canvas_tab[0]); */
	yaddr = keep_phy_addr(keep_y_addr);
	uaddr = keep_phy_addr(keep_u_addr);

	y_index = cur_index & 0xff;
	u_index = (cur_index >> 8) & 0xff;

	canvas_read(y_index, &cs0);
	canvas_read(u_index, &cs1);

	canvas_config_ex(ydupindex, (ulong)yaddr, cs0.width, cs0.height,
			 CANVAS_ADDR_NOWRAP, cs0.blkmode, cs0.endian);
	canvas_config_ex(udupindex, (ulong)uaddr, cs1.width, cs1.height,
			 CANVAS_ADDR_NOWRAP, cs1.blkmode, cs1.endian);

	canvas_read(ydupindex, &cd);
	src_index = ((y_index & 0xff) | ((u_index << 8) & 0x0000ff00));
	des_index = ((ydupindex & 0xff) | ((udupindex << 8) & 0x0000ff00));

	pr_info("ge2d_store_frame d:0x%x\n", des_index);

	ge2d_config.alu_const_color = 0;
	ge2d_config.bitmask_en = 0;
	ge2d_config.src1_gb_alpha = 0;

	ge2d_config.src_planes[0].addr = cs0.addr;
	ge2d_config.src_planes[0].w = cs0.width;
	ge2d_config.src_planes[0].h = cs0.height;
	ge2d_config.src_planes[1].addr = cs1.addr;
	ge2d_config.src_planes[1].w = cs1.width;
	ge2d_config.src_planes[1].h = cs1.height;

	ge2d_config.dst_planes[0].addr = cd.addr;
	ge2d_config.dst_planes[0].w = cd.width;
	ge2d_config.dst_planes[0].h = cd.height;

	ge2d_config.src_para.canvas_index = src_index;
	ge2d_config.src_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.src_para.format = GE2D_FORMAT_M24_NV21;
	ge2d_config.src_para.fill_color_en = 0;
	ge2d_config.src_para.fill_mode = 0;
	ge2d_config.src_para.color = 0;
	ge2d_config.src_para.top = 0;
	ge2d_config.src_para.left = 0;
	ge2d_config.src_para.width = cs0.width;
	ge2d_config.src_para.height = cs0.height;
	ge2d_config.src2_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.dst_para.canvas_index = des_index;
	ge2d_config.dst_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.dst_para.format = GE2D_FORMAT_M24_NV21;
	ge2d_config.dst_para.fill_color_en = 0;
	ge2d_config.dst_para.fill_mode = 0;
	ge2d_config.dst_para.color = 0;
	ge2d_config.dst_para.top = 0;
	ge2d_config.dst_para.left = 0;
	ge2d_config.dst_para.width = cs0.width;
	ge2d_config.dst_para.height = cs0.height;

	if (ge2d_context_config_ex(ge2d_video_context, &ge2d_config) < 0) {
		pr_info("ge2d_context_config_ex failed\n");
		return -1;
	}

	stretchblt_noalpha(ge2d_video_context,
			   0, 0, cs0.width, cs0.height,
			   0, 0, cs0.width, cs0.height);

	return 0;
}

/* static u32 canvas_tab[1]; */
static int ge2d_store_frame_YUV420(u32 cur_index)
{
	u32 y_index, u_index, v_index;
	struct canvas_s cs, cd;
	ulong yaddr, uaddr, vaddr;
	u32 ydupindex, udupindex, vdupindex;
	struct config_para_ex_s ge2d_config;

	memset(&ge2d_config, 0, sizeof(struct config_para_ex_s));

	ydupindex = display_canvas_y_dup;
	udupindex = display_canvas_u_dup;
	vdupindex = display_canvas_v_dup;

	pr_info("%s cur_index:s:0x%x\n", __func__, cur_index);
	/* operation top line */
	/* Y data */
	ge2d_config.alu_const_color = 0;
	ge2d_config.bitmask_en = 0;
	ge2d_config.src1_gb_alpha = 0;

	y_index = cur_index & 0xff;
	canvas_read(y_index, &cs);
	ge2d_config.src_planes[0].addr = cs.addr;
	ge2d_config.src_planes[0].w = cs.width;
	ge2d_config.src_planes[0].h = cs.height;
	ge2d_config.src_planes[1].addr = 0;
	ge2d_config.src_planes[1].w = 0;
	ge2d_config.src_planes[1].h = 0;
	ge2d_config.src_planes[2].addr = 0;
	ge2d_config.src_planes[2].w = 0;
	ge2d_config.src_planes[2].h = 0;

	yaddr = keep_phy_addr(keep_y_addr);
	canvas_config(ydupindex,
		      (ulong)yaddr,
		      cs.width, cs.height,
		      CANVAS_ADDR_NOWRAP, cs.blkmode);

	canvas_read(ydupindex, &cd);
	ge2d_config.dst_planes[0].addr = cd.addr;
	ge2d_config.dst_planes[0].w = cd.width;
	ge2d_config.dst_planes[0].h = cd.height;
	ge2d_config.dst_planes[1].addr = 0;
	ge2d_config.dst_planes[1].w = 0;
	ge2d_config.dst_planes[1].h = 0;
	ge2d_config.dst_planes[2].addr = 0;
	ge2d_config.dst_planes[2].w = 0;
	ge2d_config.dst_planes[2].h = 0;

	ge2d_config.src_key.key_enable = 0;
	ge2d_config.src_key.key_mask = 0;
	ge2d_config.src_key.key_mode = 0;
	ge2d_config.src_key.key_color = 0;

	ge2d_config.src_para.canvas_index = y_index;
	ge2d_config.src_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.src_para.format = GE2D_FMT_S8_Y;
	ge2d_config.src_para.fill_color_en = 0;
	ge2d_config.src_para.fill_mode = 0;
	ge2d_config.src_para.x_rev = 0;
	ge2d_config.src_para.y_rev = 0;
	ge2d_config.src_para.color = 0;
	ge2d_config.src_para.top = 0;
	ge2d_config.src_para.left = 0;
	ge2d_config.src_para.width = cs.width;
	ge2d_config.src_para.height = cs.height;
	ge2d_config.src2_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.dst_para.canvas_index = ydupindex;
	ge2d_config.dst_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.dst_para.format = GE2D_FMT_S8_Y;
	ge2d_config.dst_para.fill_color_en = 0;
	ge2d_config.dst_para.fill_mode = 0;
	ge2d_config.dst_para.x_rev = 0;
	ge2d_config.dst_para.y_rev = 0;
	ge2d_config.dst_xy_swap = 0;
	ge2d_config.dst_para.color = 0;
	ge2d_config.dst_para.top = 0;
	ge2d_config.dst_para.left = 0;
	ge2d_config.dst_para.width = cs.width;
	ge2d_config.dst_para.height = cs.height;

	if (ge2d_context_config_ex(ge2d_video_context, &ge2d_config) < 0) {
		pr_info("++ge2d configing error.\n");
		return -1;
	}
	stretchblt_noalpha(ge2d_video_context,
			   0, 0, cs.width, cs.height,
			   0, 0, cs.width, cs.height);

	/* U data */
	ge2d_config.alu_const_color = 0;
	ge2d_config.bitmask_en = 0;
	ge2d_config.src1_gb_alpha = 0;

	u_index = (cur_index >> 8) & 0xff;
	canvas_read(u_index, &cs);
	ge2d_config.src_planes[0].addr = cs.addr;
	ge2d_config.src_planes[0].w = cs.width;
	ge2d_config.src_planes[0].h = cs.height;
	ge2d_config.src_planes[1].addr = 0;
	ge2d_config.src_planes[1].w = 0;
	ge2d_config.src_planes[1].h = 0;
	ge2d_config.src_planes[2].addr = 0;
	ge2d_config.src_planes[2].w = 0;
	ge2d_config.src_planes[2].h = 0;

	uaddr = keep_phy_addr(keep_u_addr);
	canvas_config(udupindex,
		      (ulong)uaddr,
		      cs.width, cs.height,
		      CANVAS_ADDR_NOWRAP, cs.blkmode);

	canvas_read(udupindex, &cd);
	ge2d_config.dst_planes[0].addr = cd.addr;
	ge2d_config.dst_planes[0].w = cd.width;
	ge2d_config.dst_planes[0].h = cd.height;
	ge2d_config.dst_planes[1].addr = 0;
	ge2d_config.dst_planes[1].w = 0;
	ge2d_config.dst_planes[1].h = 0;
	ge2d_config.dst_planes[2].addr = 0;
	ge2d_config.dst_planes[2].w = 0;
	ge2d_config.dst_planes[2].h = 0;

	ge2d_config.src_key.key_enable = 0;
	ge2d_config.src_key.key_mask = 0;
	ge2d_config.src_key.key_mode = 0;
	ge2d_config.src_key.key_color = 0;

	ge2d_config.src_para.canvas_index = u_index;
	ge2d_config.src_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.src_para.format = GE2D_FMT_S8_CB;
	ge2d_config.src_para.fill_color_en = 0;
	ge2d_config.src_para.fill_mode = 0;
	ge2d_config.src_para.x_rev = 0;
	ge2d_config.src_para.y_rev = 0;
	ge2d_config.src_para.color = 0;
	ge2d_config.src_para.top = 0;
	ge2d_config.src_para.left = 0;
	ge2d_config.src_para.width = cs.width;
	ge2d_config.src_para.height = cs.height;

	ge2d_config.dst_para.canvas_index = udupindex;
	ge2d_config.dst_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.dst_para.format = GE2D_FMT_S8_CB;
	ge2d_config.dst_para.fill_color_en = 0;
	ge2d_config.dst_para.fill_mode = 0;
	ge2d_config.dst_para.x_rev = 0;
	ge2d_config.dst_para.y_rev = 0;
	ge2d_config.dst_xy_swap = 0;
	ge2d_config.dst_para.color = 0;
	ge2d_config.dst_para.top = 0;
	ge2d_config.dst_para.left = 0;
	ge2d_config.dst_para.width = cs.width;
	ge2d_config.dst_para.height = cs.height;

	if (ge2d_context_config_ex(ge2d_video_context, &ge2d_config) < 0) {
		pr_info("++ge2d configing error.\n");
		return -1;
	}
	stretchblt_noalpha(ge2d_video_context,
			   0, 0, cs.width, cs.height,
			   0, 0, cs.width, cs.height);

	/* operation top line */
	/* V data */
	ge2d_config.alu_const_color = 0;
	ge2d_config.bitmask_en = 0;
	ge2d_config.src1_gb_alpha = 0;

	v_index = (cur_index >> 16) & 0xff;
	canvas_read(v_index, &cs);
	ge2d_config.src_planes[0].addr = cs.addr;
	ge2d_config.src_planes[0].w = cs.width;
	ge2d_config.src_planes[0].h = cs.height;
	ge2d_config.src_planes[1].addr = 0;
	ge2d_config.src_planes[1].w = 0;
	ge2d_config.src_planes[1].h = 0;
	ge2d_config.src_planes[2].addr = 0;
	ge2d_config.src_planes[2].w = 0;
	ge2d_config.src_planes[2].h = 0;

	vaddr = keep_phy_addr(keep_v_addr);
	canvas_config(vdupindex,
		      (ulong)vaddr,
		      cs.width, cs.height,
		      CANVAS_ADDR_NOWRAP, cs.blkmode);

	ge2d_config.dst_planes[0].addr = cd.addr;
	ge2d_config.dst_planes[0].w = cd.width;
	ge2d_config.dst_planes[0].h = cd.height;
	ge2d_config.dst_planes[1].addr = 0;
	ge2d_config.dst_planes[1].w = 0;
	ge2d_config.dst_planes[1].h = 0;
	ge2d_config.dst_planes[2].addr = 0;
	ge2d_config.dst_planes[2].w = 0;
	ge2d_config.dst_planes[2].h = 0;

	ge2d_config.src_key.key_enable = 0;
	ge2d_config.src_key.key_mask = 0;
	ge2d_config.src_key.key_mode = 0;
	ge2d_config.src_key.key_color = 0;

	ge2d_config.src_para.canvas_index = v_index;
	ge2d_config.src_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.src_para.format = GE2D_FMT_S8_CR;
	ge2d_config.src_para.fill_color_en = 0;
	ge2d_config.src_para.fill_mode = 0;
	ge2d_config.src_para.x_rev = 0;
	ge2d_config.src_para.y_rev = 0;
	ge2d_config.src_para.color = 0;
	ge2d_config.src_para.top = 0;
	ge2d_config.src_para.left = 0;
	ge2d_config.src_para.width = cs.width;
	ge2d_config.src_para.height = cs.height;

	ge2d_config.dst_para.canvas_index = vdupindex;
	ge2d_config.dst_para.mem_type = CANVAS_TYPE_INVALID;
	ge2d_config.dst_para.format = GE2D_FMT_S8_CR;
	ge2d_config.dst_para.fill_color_en = 0;
	ge2d_config.dst_para.fill_mode = 0;
	ge2d_config.dst_para.x_rev = 0;
	ge2d_config.dst_para.y_rev = 0;
	ge2d_config.dst_xy_swap = 0;
	ge2d_config.dst_para.color = 0;
	ge2d_config.dst_para.top = 0;
	ge2d_config.dst_para.left = 0;
	ge2d_config.dst_para.width = cs.width;
	ge2d_config.dst_para.height = cs.height;

	if (ge2d_context_config_ex(ge2d_video_context, &ge2d_config) < 0) {
		pr_info("++ge2d configing error.\n");
		return -1;
	}
	stretchblt_noalpha(ge2d_video_context,
			   0, 0, cs.width, cs.height,
			   0, 0, cs.width, cs.height);
	return 0;
}

static void ge2d_keeplastframe_block(int cur_index, int format)
{
	u32 y_index, u_index, v_index;
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	u32 y_index2, u_index2, v_index2;
#endif

	video_module_lock();

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	y_index = vd_layer[0].canvas_tbl[0][0];
	y_index2 = vd_layer[0].canvas_tbl[1][0];
	u_index = vd_layer[0].canvas_tbl[0][1];
	u_index2 = vd_layer[0].canvas_tbl[1][1];
	v_index = vd_layer[0].canvas_tbl[0][2];
	v_index2 = vd_layer[0].canvas_tbl[1][2];
#else
	/*
	 *cur_index = READ_VCBUS_REG(VD1_IF0_CANVAS0 +
	 *	get_video_cur_dev->viu_off);
	 */
	y_index = cur_index & 0xff;
	u_index = (cur_index >> 8) & 0xff;
	v_index = (cur_index >> 16) & 0xff;
#endif

	switch (format) {
	case GE2D_FORMAT_S24_YUV444:
		pr_info("GE2D_FORMAT_S24_YUV444\n");
		if (ge2d_store_frame_S_YUV444(cur_index) > 0)
			break;
		canvas_update_addr(y_index, keep_phy_addr(keep_y_addr));
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
		canvas_update_addr(y_index2, keep_phy_addr(keep_y_addr));
		pr_info("y_index: [0x%x],y_index2: [0x%x]\n",
			y_index, y_index2);
#endif
		pr_info("cur_index: [0x%x],keep_y_addr: [0x%lx]\n",
			cur_index, keep_y_addr);
		break;
	case GE2D_FORMAT_M24_YUV444:
		pr_info("GE2D_FORMAT_M24_YUV444\n");
		ge2d_store_frame_YUV444(cur_index);
		canvas_update_addr(y_index, keep_phy_addr(keep_y_addr));
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
		canvas_update_addr(y_index2, keep_phy_addr(keep_y_addr));
#endif
		break;
	case GE2D_FORMAT_M24_NV21:
		pr_info("GE2D_FORMAT_M24_NV21\n");
		ge2d_store_frame_NV21(cur_index);
		canvas_update_addr(y_index, keep_phy_addr(keep_y_addr));
		canvas_update_addr(u_index, keep_phy_addr(keep_u_addr));
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
		canvas_update_addr(y_index2, keep_phy_addr(keep_y_addr));
		canvas_update_addr(u_index2, keep_phy_addr(keep_u_addr));
#endif
		break;
	case GE2D_FORMAT_M24_YUV420:
		pr_info("GE2D_FORMAT_M24_YUV420\n");
		ge2d_store_frame_YUV420(cur_index);
		canvas_update_addr(y_index, keep_phy_addr(keep_y_addr));
		canvas_update_addr(u_index, keep_phy_addr(keep_u_addr));
		canvas_update_addr(v_index, keep_phy_addr(keep_v_addr));
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
		canvas_update_addr(y_index2, keep_phy_addr(keep_y_addr));
		canvas_update_addr(u_index2, keep_phy_addr(keep_u_addr));
		canvas_update_addr(v_index2, keep_phy_addr(keep_v_addr));
#endif
		break;
	default:
		pr_info("default\n");
		break;
	}
	video_module_unlock();
}
#endif

#ifdef DEBUG_CANVAS_DUP
#ifndef CONFIG_AMLOGIC_MEDIA_GE2D
#define FETCHBUF_SIZE (64 * 1024) /*DEBUG_TMP*/
static int canvas_dup(ulong dst, ulong src_paddr, ulong size)
{
	void *src_addr = codec_mm_phys_to_virt(src_paddr);
	void *dst_addr = codec_mm_phys_to_virt(dst);

	if (src_paddr && dst && src_addr && dst_addr) {
		dma_addr_t dma_addr = 0;

		memcpy(dst_addr, src_addr, size);
		dma_addr = dma_map_single(get_video_device(), dst_addr,
					  size, DMA_TO_DEVICE);
		dma_unmap_single(get_video_device(), dma_addr,
				 FETCHBUF_SIZE, DMA_TO_DEVICE);
		return 1;
	}
	return 0;
}
#endif
#endif

#ifdef RESERVE_CLR_FRAME
static int free_alloced_keep_buffer(void)
{
	/*pr_info("free_alloced_keep_buffer %p.%p.%p\n",
	 *(void *)keep_y_addr, (void *)keep_u_addr, (void *)keep_v_addr);
	 */
	if (keep_y_addr) {
		codec_mm_free_for_dma(MEM_NAME, keep_y_addr);
		keep_y_addr = 0;
	}

	if (keep_u_addr) {
		codec_mm_free_for_dma(MEM_NAME, keep_u_addr);
		keep_u_addr = 0;
	}

	if (keep_v_addr) {
		codec_mm_free_for_dma(MEM_NAME, keep_v_addr);
		keep_v_addr = 0;
	}
	return 0;
}

#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
static int alloc_keep_buffer(void)
{
	int flags = CODEC_MM_FLAGS_DMA |
		CODEC_MM_FLAGS_FOR_VDECODER;
#ifndef CONFIG_AMLOGIC_MEDIA_GE2D
	/*
	 *	if not used ge2d.
	 *	need CPU access.
	 */
	flags = CODEC_MM_FLAGS_DMA | CODEC_MM_FLAGS_FOR_VDECODER;
#endif
	if ((flags & CODEC_MM_FLAGS_FOR_VDECODER) &&
	    codec_mm_video_tvp_enabled())
		/*TVP TODO for MULTI*/
		flags |= CODEC_MM_FLAGS_TVP;

	if (!keep_y_addr) {
		keep_y_addr = codec_mm_alloc_for_dma
			(MEM_NAME,
			PAGE_ALIGN(Y_BUFFER_SIZE) / PAGE_SIZE, 0, flags);
		if (!keep_y_addr) {
			pr_err("%s: failed to alloc y addr\n", __func__);
			goto err1;
		}
	}

	if (!keep_u_addr) {
		keep_u_addr = codec_mm_alloc_for_dma
			(MEM_NAME,
			PAGE_ALIGN(U_BUFFER_SIZE) / PAGE_SIZE, 0, flags);
		if (!keep_u_addr) {
			pr_err("%s: failed to alloc u addr\n", __func__);
			goto err1;
		}
	}

	if (!keep_v_addr) {
		keep_v_addr = codec_mm_alloc_for_dma
			(MEM_NAME,
			PAGE_ALIGN(V_BUFFER_SIZE) / PAGE_SIZE, 0, flags);
		if (!keep_v_addr) {
			pr_err("%s: failed to alloc v addr\n", __func__);
			goto err1;
		}
	}
	pr_info("alloced keep buffer yaddr=%p,u_addr=%p,v_addr=%p,tvp=%d\n",
		(void *)keep_y_addr,
		(void *)keep_u_addr,
		(void *)keep_v_addr,
		codec_mm_video_tvp_enabled());
	return 0;
 err1:
	free_alloced_keep_buffer();
	return -ENOMEM;
}
#endif
#endif

/*
 *flags,used per bit:
 *default free alloced keeper buffer.
 *0x1: free scatters keeper..
 *0x2:
 */
void try_free_keep_vdx(int flags, u8 layer_id)
{
	int free_scatter_keeper = flags & 0x1;
	bool layer1_used = false;
	bool layer2_used = false;
	bool layer3_used = false;

	if (layer_id >= MAX_VD_LAYER)
		return;
	if (vd_layer[0].dispbuf_mapping
		== &cur_dispbuf[1])
		layer1_used = true;
	if (vd_layer[1].dispbuf_mapping
		== &cur_dispbuf[1])
		layer2_used = true;
	if (vd_layer[2].dispbuf_mapping
		== &cur_dispbuf[2])
		layer3_used = true;

	if (keep_video_on[layer_id] || free_scatter_keeper) {
		/*pr_info("disabled keep video before free keep buffer.\n");*/
		keep_video_on[layer_id] = 0;
		if (layer1_used) {
			if (!get_video_enabled(0)) {
				pr_info("disabled pip%d on vd1 for next before free keep buffer!\n",
					layer_id);
				_video_set_disable(flags ?
					VIDEO_DISABLE_NORMAL :
					VIDEO_DISABLE_FORNEXT);
			} else {
				safe_switch_videolayer(0, false, false);
			}
		}
		if (layer2_used) {
			if (!get_video_enabled(1)) {
				pr_info("disabled pip%d on vd2 for next before free keep buffer!\n",
					layer_id);
				_videopip_set_disable(1, flags ?
					VIDEO_DISABLE_NORMAL :
					VIDEO_DISABLE_FORNEXT);
			} else {
				safe_switch_videolayer(1, false, false);
			}
		}
		if (layer3_used) {
			if (!get_video_enabled(2)) {
				pr_info("disabled pip%d on vd3 for next before free keep buffer!\n",
					layer_id);
				_videopip_set_disable(2, flags ?
					VIDEO_DISABLE_NORMAL :
					VIDEO_DISABLE_FORNEXT);
			} else {
				safe_switch_videolayer(2, false, false);
			}
		}
	}
	mutex_lock(&video_keeper_mutex);
	video_keeper_new_frame_notify(layer_id);
	if (layer_id == 0)
		free_alloced_keep_buffer();
	mutex_unlock(&video_keeper_mutex);
}

void try_free_keep_video(int flags)
{
	try_free_keep_vdx(flags, 0);
}
EXPORT_SYMBOL(try_free_keep_video);

static void video_keeper_update_keeper_mem(void *mem_handle,
					   int type,
					   int *id)
{
	int ret;
	int old_id = *id;

	if (!mem_handle)
		return;
	ret = codec_mm_keeper_mask_keep_mem(mem_handle,
					    type);
	if (ret > 0) {
		if (old_id > 0 && ret != old_id) {
			/*wait 80 ms for vsync post.*/
			codec_mm_keeper_unmask_keeper(old_id, 120);
		}
		*id = ret;
	}
}

static int video_keeper_frame_keep_locked(u8 layer_id,
	struct vframe_s *cur_buf,
	struct vframe_s *cur_buf_el)
{
	int type = MEM_TYPE_CODEC_MM;
	int keeped = 0;

	if (cur_buf->type & VIDTYPE_SCATTER)
		type = MEM_TYPE_CODEC_MM_SCATTER;
	video_keeper_update_keeper_mem
		(cur_buf->mem_handle,
		type,
		&keep_id[layer_id]);
	video_keeper_update_keeper_mem
		(cur_buf->mem_head_handle,
		MEM_TYPE_CODEC_MM,
		&keep_head_id[layer_id]);
	video_keeper_update_keeper_mem
		(cur_buf->mem_dw_handle,
		MEM_TYPE_CODEC_MM,
		&keep_dw_id[layer_id]);

	if (layer_id == 0 || layer_id == 1) {
		if (cur_buf_el) {
			if (cur_buf_el->type & VIDTYPE_SCATTER)
				type = MEM_TYPE_CODEC_MM_SCATTER;
			else
				type = MEM_TYPE_CODEC_MM;
			video_keeper_update_keeper_mem
				(cur_buf_el->mem_handle,
				type,
				&keep_el_id[layer_id]);
			video_keeper_update_keeper_mem
				(cur_buf_el->mem_head_handle,
				MEM_TYPE_CODEC_MM,
				&keep_el_head_id[layer_id]);
			video_keeper_update_keeper_mem
				(cur_buf_el->mem_dw_handle,
				MEM_TYPE_CODEC_MM,
				&keep_el_dw_id[layer_id]);
			}
		keeped = (keep_id[layer_id] + keep_head_id[layer_id] + keep_dw_id[layer_id]) > 0;
	} else {
		keeped = (keep_id[layer_id] + keep_head_id[layer_id]) > 0;
	}
	return keeped;
}

/*
 * call in irq.
 *don't used mutex
 */
void video_keeper_new_frame_notify(u8 layer_id)
{
	if (layer_id >= MAX_VD_LAYER)
		return;

	if (keep_video_on[layer_id]) {
		pr_info("new frame show, free keeper\n");
		keep_video_on[layer_id] = 0;
	}
	if (keep_id[layer_id] > 0) {
		/*wait 80 ms for vsync post.*/
		codec_mm_keeper_unmask_keeper(keep_id[layer_id], 120);
		keep_id[layer_id] = 0;
	}
	if (keep_head_id[layer_id] > 0) {
		/*wait 80 ms for vsync post.*/
		codec_mm_keeper_unmask_keeper(keep_head_id[layer_id], 120);
		keep_head_id[layer_id] = 0;
	}
	if (keep_dw_id[layer_id] > 0) {
		/*wait 80 ms for vsync post.*/
		codec_mm_keeper_unmask_keeper(keep_dw_id[layer_id], 120);
		keep_dw_id[layer_id] = 0;
	}

	if (layer_id == 0 || layer_id == 1) {
		if (keep_el_id[layer_id] > 0) {
			/*wait 80 ms for vsync post.*/
			codec_mm_keeper_unmask_keeper(keep_el_id[layer_id], 120);
			keep_el_id[layer_id] = 0;
		}
		if (keep_el_head_id[layer_id] > 0) {
			/*wait 80 ms for vsync post.*/
			codec_mm_keeper_unmask_keeper(keep_el_head_id[layer_id], 120);
			keep_el_head_id[layer_id] = 0;
		}
		if (keep_el_dw_id[layer_id] > 0) {
			/*wait 80 ms for vsync post.*/
			codec_mm_keeper_unmask_keeper(keep_el_dw_id[layer_id], 120);
			keep_el_dw_id[layer_id] = 0;
		}
	}
	return;
}

#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
static unsigned int vf_ge2d_keep_frame_locked(struct vframe_s *ge2d_buf)
{
	u32 cur_index = 0;
	u32 y_index, u_index, v_index;
	struct canvas_s cs0, cs1, cs2, cd;
	bool layer1_used = false;
	bool layer2_used = false;

	if (vd_layer[0].dispbuf_mapping
		== &cur_dispbuf[0])
		layer1_used = true;
	if (vd_layer[1].dispbuf_mapping
		== &cur_dispbuf[0])
		layer2_used = true;

#ifdef CONFIG_AMLOGIC_MEDIA_MULTI_DEC
	if (codec_mm_video_tvp_enabled()) {
		pr_info("keep exit is TVP\n");
		return 0;
	}
#endif

	if (ge2d_buf->type & VIDTYPE_COMPRESS) {
		/* todo: duplicate compressed video frame */
		pr_info("keep exit is skip VIDTYPE_COMPRESS\n");
		return 0;
	}

	if (!layer1_used && !layer2_used) {
		/* No layer to display this path */
		pr_info("keep exit because no layer to keep this buffer\n");
		return -1;
	}

	if (layer1_used)
		cur_index = get_layer_display_canvas(0);
	else if (layer2_used)
		cur_index = get_layer_display_canvas(1);

	y_index = cur_index & 0xff;
	u_index = (cur_index >> 8) & 0xff;
	v_index = (cur_index >> 16) & 0xff;
	canvas_read(y_index, &cd);

	if ((cd.width * cd.height) <= 1920 * 1088 * 3 &&
	    !keep_y_addr) {
		alloc_keep_buffer();
	}
	if (!keep_y_addr) {
		pr_info("%s:alloc keep buffer failed, keep_y_addr is NULL!\n",
			__func__);
		return 0;
	}

	if (get_video_debug_flags() & DEBUG_FLAG_BASIC_INFO) {
		pr_info("%s keep_y_addr=%p %lx\n",
			__func__, (void *)keep_y_addr,
			canvas_get_addr(y_index));
	}

	if ((ge2d_buf->type & VIDTYPE_VIU_422) == VIDTYPE_VIU_422) {
		pr_info("%s:no support VIDTYPE_VIU_422\n", __func__);
		return 0;
	} else if ((ge2d_buf->type & VIDTYPE_VIU_444) == VIDTYPE_VIU_444) {
		if ((Y_BUFFER_SIZE < (cd.width * cd.height))) {
			pr_info
			    ("[%s::%d] error:data>buf size: %x,%x,%x, %x,%x\n",
			     __func__, __LINE__, Y_BUFFER_SIZE,
			     U_BUFFER_SIZE, V_BUFFER_SIZE,
				cd.width, cd.height);
			return 0;
		}
		ge2d_keeplastframe_block(cur_index, GE2D_FORMAT_S24_YUV444);
		if (get_video_debug_flags() & DEBUG_FLAG_BASIC_INFO)
			pr_info("%s: VIDTYPE_VIU_444\n", __func__);
	} else if ((ge2d_buf->type & VIDTYPE_VIU_NV21) == VIDTYPE_VIU_NV21) {
		canvas_read(y_index, &cs0);
		canvas_read(u_index, &cs1);
		if ((Y_BUFFER_SIZE < (cs0.width * cs0.height)) ||
		    (U_BUFFER_SIZE < (cs1.width * cs1.height))) {
			pr_info("## [%s::%d] error: yuv data size larger",
				__func__, __LINE__);
			return 0;
		}
		ge2d_keeplastframe_block(cur_index, GE2D_FORMAT_M24_NV21);
		if (get_video_debug_flags() & DEBUG_FLAG_BASIC_INFO)
			pr_info("%s: VIDTYPE_VIU_NV21\n", __func__);
	} else {
		canvas_read(y_index, &cs0);
		canvas_read(u_index, &cs1);
		canvas_read(v_index, &cs2);

		if ((Y_BUFFER_SIZE < (cs0.width * cs0.height)) ||
		    (U_BUFFER_SIZE < (cs1.width * cs1.height)) ||
		    (V_BUFFER_SIZE < (cs2.width * cs2.height))) {
			pr_info("## [%s::%d] error: yuv data size larger",
				__func__, __LINE__);
			return 0;
		}
		ge2d_keeplastframe_block(cur_index, GE2D_FORMAT_M24_YUV420);
		if (get_video_debug_flags() & DEBUG_FLAG_BASIC_INFO)
			pr_info("%s: VIDTYPE_VIU_420\n", __func__);
	}
	pr_info("%s: use ge2d keep video\n", __func__);
	return 1;
}
#else
static unsigned int vf_ge2d_keep_frame_locked(struct vframe_s *ge2d_buf)
{
	return 0;
}

#endif

unsigned int vf_keep_current_locked(u8 layer_id,
	struct vframe_s *cur_buf,
	struct vframe_s *cur_buf_el)
{
	int ret = 0;

	if (layer_id >= MAX_VD_LAYER)
		return 0;

	if (!cur_buf) {
		pr_info("keep vd%d exit without cur_buf\n", layer_id);
		return 0;
	}

	if (get_video_debug_flags() &
		DEBUG_FLAG_TOGGLE_SKIP_KEEP_CURRENT) {
		pr_info("flag: keep vd%d exit is skip current\n", layer_id);
		return 0;
	}

#ifdef CONFIG_AMLOGIC_MEDIA_VIDEOCAPTURE
	ext_frame_capture_poll(1); /*pull  if have capture end frame */
#endif

	if (get_vdx_blackout_policy(layer_id)) {
		pr_info("policy: keep exit is skip current\n");
		return 0;
	}

	if (cur_buf->source_type == VFRAME_SOURCE_TYPE_HDMI ||
		cur_buf->source_type == VFRAME_SOURCE_TYPE_CVBS) {
		pr_info("hdmi/cvbs keep exit\n");
		return 0;
	}

	if (IS_DI_PROCESSED(cur_buf->type)) {
		ret = 2;
		if (cur_buf->vf_ext &&
		    (cur_buf->flag & VFRAME_FLAG_DOUBLE_FRAM)) {
			ret = video_keeper_frame_keep_locked
				(layer_id, (struct vframe_s *)cur_buf->vf_ext,
				cur_buf_el);
			pr_info("vd%d keep di_dec buffer\n", layer_id);
		}
		pr_info("vd%d keep exit is di %s\n",
			layer_id,
			IS_DI_POSTWRTIE(cur_buf->type) ?
			"post write" : "post");
		return ret;
	}

	if (layer_id == 0 &&
		cur_buf->source_type == VFRAME_SOURCE_TYPE_PPMGR) {
		pr_info("ppmgr use ge2d keep frame!\n");
		ret = vf_ge2d_keep_frame_locked(cur_buf);
	} else {
		pr_info("use keep buffer keep frame!\n");
		ret = video_keeper_frame_keep_locked(layer_id, cur_buf, cur_buf_el);
	}

	if (layer_id == 0) {
		if (ret) {
			keep_video_on[layer_id] = 1;
			pr_info("%s: keep video successful!\n", __func__);
		} else {
			keep_video_on[layer_id] = 0;
			pr_info("%s: keep video failed!\n", __func__);
		}
		return ret;
	} else {
		if (ret) {
			/*keeped ok with codec keeper!*/
			pr_info("keep vd%d buffer on!\n", layer_id);
			keep_video_on[layer_id] = 1;
			return 1;
		}
		keep_video_on[layer_id] = 0;
	}
	return 0;
}

unsigned int vf_keep_current(struct vframe_s *cur_buf,
			     struct vframe_s *cur_buf2)
{
	unsigned int ret;
	u8 layer_id = 0;

	mutex_lock(&video_keeper_mutex);
	ret = vf_keep_current_locked(layer_id, cur_buf, cur_buf2);
	mutex_unlock(&video_keeper_mutex);
	return ret;
}

int video_keeper_init(void)
{
#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
	/* video_frame_getmem(); */
	ge2d_videotask_init();
#endif
	return 0;
}

void video_keeper_exit(void)
{
#ifdef CONFIG_AMLOGIC_MEDIA_GE2D
	ge2d_videotask_release();
#endif
}
