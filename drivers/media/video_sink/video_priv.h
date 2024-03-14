/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * drivers/amlogic/media/video_sink/video_priv.h
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

#ifndef VIDEO_PRIV_HEADER_HH
#define VIDEO_PRIV_HEADER_HH

#include <linux/amlogic/media/video_sink/vpp.h>
#include "video_reg.h"
#include <linux/amlogic/media/amvecm/amvecm.h>
#include "video_reg_s5.h"

#ifdef CONFIG_AMLOGIC_MEDIA_DEINTERLACE
#define ENABLE_PRE_LINK
#endif

#define VIDEO_ENABLE_STATE_IDLE       0
#define VIDEO_ENABLE_STATE_ON_REQ     1
#define VIDEO_ENABLE_STATE_ON_PENDING 2
#define VIDEO_ENABLE_STATE_OFF_REQ    3

#define DEBUG_FLAG_BASIC_INFO     0x1
#define DEBUG_FLAG_PRINT_TOGGLE_FRAME 0x2
#define DEBUG_FLAG_PRINT_RDMA                0x4
#define DEBUG_FLAG_GET_COUNT                 0x8
#define DEBUG_FLAG_PRINT_DISBUF_PER_VSYNC        0x10
#define DEBUG_FLAG_PRINT_PATH_SWITCH        0x20
#define DEBUG_FLAG_TRACE_EVENT	        0x40
#define DEBUG_FLAG_AI_FACE	        0x80
#define DEBUG_FLAG_LOG_RDMA_LINE_MAX         0x100
#define DEBUG_FLAG_BLACKOUT     0x200
#define DEBUG_FLAG_NO_CLIP_SETTING     0x400
#define DEBUG_FLAG_VPP_GET_BUFFER_TIME     0x800
#define DEBUG_FLAG_PRINT_FRAME_DETAIL     0x1000
#define DEBUG_FLAG_PRELINK			0x2000
#define DEBUG_FLAG_PRELINK_MORE     0x4000
#define DEBUG_FLAG_AFD_INFO	        0x8000
#define DEBUG_FLAG_TOGGLE_SKIP_KEEP_CURRENT  0x10000
#define DEBUG_FLAG_TOGGLE_FRAME_PER_VSYNC    0x20000
#define DEBUG_FLAG_RDMA_WAIT_1		     0x40000
#define DEBUG_FLAG_VSYNC_DONONE                0x80000
#define DEBUG_FLAG_GOFIELD_MANUL             0x100000
#define DEBUG_FLAG_LATENCY             0x200000
#define DEBUG_FLAG_PTS_TRACE            0x400000
#define DEBUG_FLAG_FRAME_DETECT            0x800000
#define DEBUG_FLAG_OMX_DEBUG_DROP_FRAME        0x1000000
#define DEBUG_FLAG_RECEIVER_DEBUG        0x2000000
#define DEBUG_FLAG_PRINT_DROP_FRAME        0x4000000
#define DEBUG_FLAG_OMX_DV_DROP_FRAME        0x8000000
#define DEBUG_FLAG_COMPOSER_NO_DROP_FRAME     0x10000000
#define DEBUG_FLAG_AXIS_NO_UPDATE     0x20000000
#define DEBUG_FLAG_HDMI_AVSYNC_DEBUG     0x40000000
#define DEBUG_FLAG_HDMI_DV_CRC     0x80000000

/*for performance_debug*/
#define DEBUG_FLAG_VSYNC_PROCESS_TIME  0x1
#define DEBUG_FLAG_OVER_VSYNC          0x2

#define VOUT_TYPE_TOP_FIELD 0
#define VOUT_TYPE_BOT_FIELD 1
#define VOUT_TYPE_PROG      2

#define VIDEO_DISABLE_NONE    0
#define VIDEO_DISABLE_NORMAL  1
#define VIDEO_DISABLE_FORNEXT 2

#define VIDEO_NOTIFY_TRICK_WAIT   0x01
#define VIDEO_NOTIFY_PROVIDER_GET 0x02
#define VIDEO_NOTIFY_PROVIDER_PUT 0x04
#define VIDEO_NOTIFY_FRAME_WAIT   0x08
#define VIDEO_NOTIFY_NEED_NO_COMP  0x10

#define MAX_VD_LAYER 3
#define COMPOSE_MODE_NONE			0
#define COMPOSE_MODE_3D			1
#define COMPOSE_MODE_DV			2
#define COMPOSE_MODE_BYPASS_CM	4

#define VIDEO_PROP_CHANGE_NONE		0
#define VIDEO_PROP_CHANGE_SIZE		0x1
#define VIDEO_PROP_CHANGE_FMT		0x2
#define VIDEO_PROP_CHANGE_ENABLE	0x4
#define VIDEO_PROP_CHANGE_DISABLE	0x8
#define VIDEO_PROP_CHANGE_AXIS		0x10
#define VIDEO_PROP_CHANGE_FMM		0x20
#define VIDEO_PROP_CHANGE_FMM_DISABLE	0x40
#define VIDEO_PROP_CHANGE_SLICE_NUM  0x60

#define MAX_ZOOM_RATIO 300

#define VPP_PREBLEND_VD_V_END_LIMIT 2304

#define LAYER1_CANVAS_BASE_INDEX 0x58
#define LAYER2_CANVAS_BASE_INDEX 0x64
#define LAYER3_CANVAS_BASE_INDEX 0xd8

#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
#define CANVAS_TABLE_CNT 2
#else
#define CANVAS_TABLE_CNT 1
#endif

#define VIDEO_AUTO_POST_BLEND_DUMMY BIT(24)

#define DISPBUF_TO_PUT_MAX 6

#define IS_DI_PROCESSED(vftype) ((vftype) & (VIDTYPE_PRE_INTERLACE | VIDTYPE_DI_PW))
#define IS_DI_POST(vftype) \
	(((vftype) & (VIDTYPE_PRE_INTERLACE | VIDTYPE_DI_PW)) \
	 == VIDTYPE_PRE_INTERLACE)
#define IS_DI_POSTWRTIE(vftype) ((vftype) & VIDTYPE_DI_PW)
#define IS_DI_PRELINK(di_flag) ((di_flag) & DI_FLAG_DI_PVPPLINK)
#define IS_DI_PRELINK_BYPASS(di_flag) ((di_flag) & DI_FLAG_DI_PVPPLINK_BYPASS)

#define MAX_PIP_WINDOW    16
#define VPP_FILER_COEFS_NUM   33
#define VPP_NUM 3
#define RDMA_INTERFACE_NUM 4

#define OP_VPP_MORE_LOG 1
#define OP_FORCE_SWITCH_VF 2
#define OP_FORCE_NOT_SWITCH_VF 4
#define OP_HAS_DV_EL 8

enum tvin_surface_type_e {
	TVIN_SOURCE_TYPE_OTHERS = 0,
	TVIN_SOURCE_TYPE_DECODER = 1,  /*DTV*/
	TVIN_SOURCE_TYPE_VDIN = 2,   /*ATV HDMIIN CVBS*/
};

enum vd_path_id {
	VFM_PATH_DEF = -1,
	VFM_PATH_AMVIDEO = 0,
	VFM_PATH_PIP = 1,
	VFM_PATH_VIDEO_RENDER0 = 2,
	VFM_PATH_VIDEO_RENDER1 = 3,
	VFM_PATH_PIP2 = 4,
	VFM_PATH_VIDEO_RENDER2 = 5,
	VFM_PATH_VIDEO_RENDER3 = 6,
	VFM_PATH_VIDEO_RENDER4 = 7,
	VFM_PATH_VIDEO_RENDER5 = 8,
	VFM_PATH_VIDEO_RENDER6 = 9,
	VFM_PATH_AUTO = 0xfe,
	VFM_PATH_INVALID = 0xff
};

enum toggle_out_fl_frame_e {
	OUT_FA_A_FRAME,
	OUT_FA_BANK_FRAME,
	OUT_FA_B_FRAME
};

enum pre_hscaler_e {
	PRE_HSCALER_2TAP = 2,
	PRE_HSCALER_4TAP = 4,
	PRE_HSCALER_6TAP = 6,
	PRE_HSCALER_8TAP = 8
};

enum pre_vscaler_e {
	PRE_VSCALER_2TAP = 2,
	PRE_VSCALER_4TAP = 4,
	PRE_VSCALER_6TAP = 6,
	PRE_VSCALER_8TAP = 8
};

enum vpp_type_e {
	VPP0 = 0,
	VPP1 = 1,
	VPP2 = 2,
	PRE_VSYNC = 3,
	VPP_MAX = 4
};

enum reshape_mode_e {
	MODE_2X2 = 1,
	MODE_3X3 = 2,
	MODE_4X4 = 3,
};

enum VPU_MODULE_e {
	FRC0_R,
	FRC0_W,
	FRC1_R,
	FRC1_W,
	FRC2_R,
	VPU0_R,
	VPU0_W,
	VPU1_R,
	VPU1_W,
	VPU2_R,
};

enum VPU_MODULE_S5_e {
	VPP_ARB0_S5,
	VPP_ARB1_S5,
	VPP_ARB2_S5,
	VPU_SUB_READ_S5,
	DCNTR_GRID_S5,
	TCON_P1_S5,
	TCON_P2_S5,
	TCON_P3_S5
};

enum VPU_MODULE_T7_Qos_e {
	VPP_ARB0_T7,
	VPP_ARB1_T7,
	RDMA_READ_T7,
	LDIM_T7 = 7,
	VDIN_AFBCE_T7,
	VPU_DMA_T7
};

enum VPU_MODULE_T5M_Qos_e {
	VPP_ARB0_T5M,
	VPP_ARB1_T5M,
	RDMA_READ_T5M,
	VPU_SUB_READ_T5M,
	TCON_P1_T5M,
	DCNTR_GRID_T5M,
	TCON_P2_T5M
};

enum display_module_e {
	OLD_DISPLAY_MODULE,
	T7_DISPLAY_MODULE,
	S5_DISPLAY_MODULE,
	C3_DISPLAY_MODULE,
};

enum matrix_type_e {
	MATRIX_BYPASS = 0,
	YUV2RGB,
	RGB2YUV
};

typedef u32 (*rdma_rd_op)(u32 reg);
typedef int (*rdma_wr_op)(u32 reg, u32 val);
typedef int (*rdma_wr_bits_op)(u32 reg, u32 val, u32 start, u32 len);

struct rdma_fun_s {
	rdma_rd_op rdma_rd;
	rdma_wr_op rdma_wr;
	rdma_wr_bits_op rdma_wr_bits;
};

struct video_dev_s {
	int vpp_off;
	int viu_off;
	int mif_linear;
	int display_module;
	int max_vd_layers;
	int vd2_independ_blend_ctrl;
	int aisr_support;
	int aisr_enable;
	int scaler_sep_coef_en;
	int pps_auto_calc;
	bool di_hf_y_reverse;
	bool aisr_demo_en;
	u32 aisr_demo_xstart;
	u32 aisr_demo_ystart;
	u32 aisr_demo_xend;
	u32 aisr_demo_yend;
	bool power_ctrl;
	struct hw_pps_reg_s aisr_pps_reg;
	struct vpp_frame_par_s aisr_frame_parms;
	struct rdma_fun_s rdma_func[RDMA_INTERFACE_NUM];
	u32 sr_in_size;
	u8 sr0_support;
	u8 sr1_support;
	u8 is_tv_panel;
	u8 prevsync_support;
	u8 pre_vsync_enable;
	u8 secure_src;
	u8 sr01_num;
	u8 mosaic_support;
	u8 cr_loss;
	u8 amdv_tvcore;
	u8 vsync_2to1_enable;
	u8 vpp_in_padding_support;
	u8 has_vpp1;
	u8 has_vpp2;
};

struct video_layer_s;

struct mif_pos_s {
	u32 id;
	struct hw_vd_reg_s *p_vd_mif_reg;
	struct hw_afbc_reg_s *p_vd_afbc_reg;

	/* frame original size */
	u32 src_w;
	u32 src_h;

	/* mif start - end lines */
	u32 start_x_lines;
	u32 end_x_lines;
	u32 start_y_lines;
	u32 end_y_lines;

	/* left and right eye position, skip flag. */
	/* And if non 3d case, left eye = right eye */
	u32 l_hs_luma;
	u32 l_he_luma;
	u32 l_hs_chrm;
	u32 l_he_chrm;
	u32 r_hs_luma;
	u32 r_he_luma;
	u32 r_hs_chrm;
	u32 r_he_chrm;
	u32 h_skip;
	u32 hc_skip; /* afbc chroma skip */

	u32 l_vs_luma;
	u32 l_ve_luma;
	u32 l_vs_chrm;
	u32 l_ve_chrm;
	u32 r_vs_luma;
	u32 r_ve_luma;
	u32 r_vs_chrm;
	u32 r_ve_chrm;
	u32 v_skip;
	u32 vc_skip; /* afbc chroma skip */

	bool reverse;

	bool skip_afbc;
	u32 vpp_3d_mode;
	u8 block_mode;
};

struct scaler_setting_s {
	u32 id;
	u32 misc_reg_offt;
	bool support;

	bool sc_h_enable;
	bool sc_v_enable;
	bool sc_top_enable;

	bool last_line_fix;

	u32 vinfo_width;
	u32 vinfo_height;
	/* u32 VPP_pic_in_height_; */
	/* u32 VPP_line_in_length_; */

	struct vpp_frame_par_s *frame_par;
};

struct blend_setting_s {
	u32 id;
	u32 misc_reg_offt;

	u32 layer_alpha;

	u32 preblend_h_start;
	u32 preblend_h_end;
	u32 preblend_v_start;
	u32 preblend_v_end;

	u32 preblend_h_size;

	u32 postblend_h_start;
	u32 postblend_h_end;
	u32 postblend_v_start;
	u32 postblend_v_end;

	u32 postblend_h_size;

	struct vpp_frame_par_s *frame_par;
};

struct clip_setting_s {
	u32 id;
	u32 misc_reg_offt;

	u32 clip_max;
	u32 clip_min;

	bool clip_done;
};

struct pip_alpha_scpxn_s {
	u32 win_en; /* bitmask for which window enable */
	u32 scpxn_bgn_h[MAX_PIP_WINDOW];
	u32 scpxn_end_h[MAX_PIP_WINDOW];
	u32 scpxn_bgn_v[MAX_PIP_WINDOW];
	u32 scpxn_end_v[MAX_PIP_WINDOW];
};

struct fgrain_setting_s {
	u32 id;
	u32 start_x;
	u32 end_x;
	u32 start_y;
	u32 end_y;
	u32 fmt_mode; /* only support 420 */
	u32 bitdepth; /* 8 bit or 10 bit */
	u32 reverse;
	u32 afbc; /* afbc or not */
	u32 last_in_mode; /* related with afbc */
	u32 used;
	/* lut dma */
	u32 fgs_table_adr;
	u32 table_size;
};

struct aisr_setting_s {
	u32 aisr_enable;
	u32 in_ratio; /* 1:2x2  2:3x3 3:4x4 */
	u32 src_w;
	u32 src_h;
	u32 src_align_w;
	u32 src_align_h;
	u32 buf_align_w;
	u32 buf_align_h;
	u32 x_start;
	u32 x_end;
	u32 y_start;
	u32 y_end;
	u32 little_endian;
	u32 swap_64bit;
	u32 di_hf_y_reverse;
	u32 vscale_skip_count;
	ulong phy_addr;
};

enum mode_3d_e {
	mode_3d_disable = 0,
	mode_3d_enable,
	mode_3d_mvc_enable
};

struct sub_slice_s {
	u32 slice_index;
	u32 src_field;
	u32 src_fmt;
	u32 src_mode;
	u32 src_bits;
	ulong src_addr;
	u32 src_hsize;
	u32 src_vsize;
	u32 src_x_start;
	u32 src_x_end;
	u32 src_y_start;
	u32 src_y_end;
};

struct path_id_s {
	s32 vd1_path_id;
	s32 vd2_path_id;
	s32 vd3_path_id;
};

typedef struct vframe_s *(*toggle_frame_op)(u8 layer_id,
		u8 fake_layer_id,
		s32 *vd_path_id,
		struct path_id_s *path_id);
typedef void (*swap_frame_op)(u8 layer_id,
				s32 vd1_path_id,
				s32 cur_vd1_path_id,
				struct vframe_s **path_new_frame);
typedef s32 (*render_frame_op)(struct video_layer_s *layer, const struct vinfo_s *vinfo);
typedef int (*recv_early_op)(u8 layer_id, u8 fake_layer_id);
typedef int (*recv_late_op)(u8 layer_id, u8 fake_layer_id);
typedef int (*misc_early_op)(u8 layer_id, bool rdma_enable, bool rdma_enable_pre);
typedef void (*misc_late_op)(u8 layer_id);

struct vd_func_s {
	toggle_frame_op vd_toggle_frame;
	swap_frame_op vd_swap_frame;
	render_frame_op vd_render_frame;
	recv_early_op vd_early_process;
	recv_late_op vd_late_process;
	misc_early_op vd_misc_early_proc;
	misc_late_op vd_misc_late_proc;
	u32 path_frame_index;
	u32 video_process_flag;
	u32 fake_func_id;
};

struct video_layer_s {
	u8 layer_id;
	u8 layer_support;
	/* reg map offsett*/
	u32 misc_reg_offt;
	struct hw_vd_reg_s vd_mif_reg;
	struct hw_vd_linear_reg_s vd_mif_linear_reg;
	struct hw_afbc_reg_s vd_afbc_reg;
	struct hw_fg_reg_s fg_reg;
	struct hw_pps_reg_s pps_reg;
	struct hw_vpp_blend_reg_s vpp_blend_reg;
	u8 cur_canvas_id;
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
	u8 next_canvas_id;
#endif

	/* vframe buffer */
	struct vframe_s *dispbuf;
	struct vframe_s **dispbuf_mapping;

	u32 canvas_tbl[CANVAS_TABLE_CNT][6];
	u32 disp_canvas[CANVAS_TABLE_CNT][2];

	bool property_changed;
	u8 force_config_cnt;
	bool new_vpp_setting;
	bool new_frame;
	u32 vout_type;
	bool bypass_pps;
	bool switch_vf;
	u8 force_switch_mode;
	struct vframe_s *vf_ext;

	struct vpp_frame_par_s *cur_frame_par;
	struct vpp_frame_par_s *next_frame_par;
	struct vpp_frame_par_s frame_parms[2];

	/* struct disp_info_s disp_info; */
	struct mif_pos_s mif_setting;
	struct mif_pos_s slice_mif_setting[SLICE_NUM];
	struct scaler_setting_s sc_setting;
	struct blend_setting_s bld_setting;
	struct fgrain_setting_s fgrain_setting;
	struct fgrain_setting_s slice_fgrain_setting[SLICE_NUM];
	struct clip_setting_s clip_setting;
	struct aisr_setting_s aisr_mif_setting;
	struct scaler_setting_s aisr_sc_setting;
	struct pip_alpha_scpxn_s alpha_win_setting;
	struct vd_func_s vd_func;
	struct vd_func_s pre_vd_func[2];
	struct vd_func_s *cur_pre_func;
	struct vd_func_s *next_pre_func;
	s32 vd_path_id;

	u32 new_vframe_count;

	u32 start_x_lines;
	u32 end_x_lines;
	u32 start_y_lines;
	u32 end_y_lines;

	u32 disable_video;
	u32 enabled;
	u32 pre_blend_en;
	u32 post_blend_en;
	u32 enabled_status_saved;
	u32 onoff_state;
	u32 onoff_time;

	u32 layer_alpha;
	u32 global_output;

	u8 func_path_id;
	u8 keep_frame_id;

	u8 enable_3d_mode;

	u32 global_debug;

	bool need_switch_vf;
	bool do_switch;
	bool force_black;
	bool force_disable;
	u8 vpp_index;
	u8 vppx_blend_en;
	bool vd1_vd2_mux;
	u32 video_en_bg_color;
	u32 video_dis_bg_color;
	u32 dummy_alpha;
	u32 compWidth;
	u32 compHeight;
	u32 src_width;
	u32 src_height;
	u32 alpha_win_en;
	struct pip_alpha_scpxn_s alpha_win;

	bool pre_link_en;
	bool need_disable_prelink;
	bool prelink_bypass_check;
	atomic_t disable_prelink_done;

	bool mosaic_frame;
	bool frc_n2m_1st_frame;
	u8 prelink_skip_cnt;
	s32 last_di_instance;
	u32 slice_num;
	u32 pi_enable;
	u32 vd1s1_vd2_prebld_en;
	u32 display_cnt;//count the number of times vf is displayed
	u32 mosaic_mode;
	struct sub_slice_s sub_slice[SLICE_NUM - 1];
	struct vframe_s *vf_top1;
	u32 frc_h_size_pre;
	u32 frc_v_size_pre;
};

struct video_save_s {
	struct vframe_s *save_vf;
	struct vframe_s *toggle_vf;
	bool save_vf_en;
};

enum {
	ONLY_CORE0,
	ONLY_CORE1,
	NEW_CORE0_CORE1,
	OLD_CORE0_CORE1,
};
enum cpu_type_e {
	MESON_CPU_MAJOR_ID_COMPATIBLE = 0x1,
	MESON_CPU_MAJOR_ID_TM2_REVB,
	MESON_CPU_MAJOR_ID_SC2_,
	MESON_CPU_MAJOR_ID_T5_,
	MESON_CPU_MAJOR_ID_T5D_,
	MESON_CPU_MAJOR_ID_T7_,
	MESON_CPU_MAJOR_ID_S4_,
	MESON_CPU_MAJOR_ID_T5D_REVB_,
	MESON_CPU_MAJOR_ID_T3_,
	MESON_CPU_MAJOR_ID_T5W_,
	MESON_CPU_MAJOR_ID_C3_,
	MESON_CPU_MAJOR_ID_S5_,
	MESON_CPU_MAJOR_ID_T5M_,
	MESON_CPU_MAJOR_ID_T3X_,
	MESON_CPU_MAJOR_ID_TXHD2_,
	MESON_CPU_MAJOR_ID_S1A_,
	MESON_CPU_MAJOR_ID_S7_,
	MESON_CPU_MAJOR_ID_UNKNOWN_,
};

struct video_device_hw_s {
	u8 vd2_independ_blend_ctrl;
	u8 aisr_support;
	u8 prevsync_support;
	u8 di_hf_y_reverse;
	u8 sr_in_size;
	u8 mosaic_support;
	u8 sr01_num;
	u8 cr_loss;
	u8 amdv_tvcore;
	u8 vpp_in_padding_support;
};

struct amvideo_device_data_s {
	enum cpu_type_e cpu_type;
	u32 sr_reg_offt;
	u32 sr_reg_offt2;
	u8 layer_support[MAX_VD_LAYER];
	u8 afbc_support[MAX_VD_LAYER];
	u8 pps_support[MAX_VD_LAYER];
	u8 alpha_support[MAX_VD_LAYER];
	u8 dv_support;
	u8 sr0_support;
	u8 sr1_support;
	u32 core_v_disable_width_max[MAX_SR_NUM];
	u32 core_v_enable_width_max[MAX_SR_NUM];
	u8 supscl_path;
	u8 fgrain_support[MAX_VD_LAYER];
	u8 has_hscaler_8tap[MAX_VD_LAYER];
	/* 1: 4tap; 2: 8tap */
	u8 has_pre_hscaler_ntap[MAX_VD_LAYER];
	u8 has_pre_vscaler_ntap[MAX_VD_LAYER];
	u32 src_width_max[MAX_VD_LAYER];
	u32 src_height_max[MAX_VD_LAYER];
	u32 ofifo_size;
	u32 afbc_conv_lbuf_len[MAX_VD_LAYER];
	u8 mif_linear;
	u8 display_module;
	u8 max_vd_layers;
	u8 has_vpp1;
	u8 has_vpp2;
	struct video_device_hw_s dev_property;
	u8 is_tv_panel;
};

struct pre_scaler_info {
	u32 force_pre_scaler;
	u32 pre_hscaler_ntap_enable;
	u32 pre_hscaler_ntap_set;
	u32 pre_hscaler_ntap;
	u32 pre_hscaler_rate;
	u32 pre_hscaler_coef[4];
	u32 pre_hscaler_coef_set;
	u32 pre_vscaler_ntap_enable;
	u32 pre_vscaler_ntap_set;
	u32 pre_vscaler_ntap;
	u32 pre_vscaler_rate;
	u32 pre_vscaler_coef[4];
	u32 pre_vscaler_coef_set;
};

enum {
	VD1_PROBE = 1,
	VD2_PROBE,
	VD3_PROBE,
	OSD1_PROBE,
	OSD2_PROBE,
	OSD3_PROBE,
	OSD4_PROBE,
	POST_VADJ_PROBE,
	POSTBLEND_PROBE,
};

enum {
	VIDEO_PROBE = 1,
	OSD_PROBE,
	POST_PROBE,
};

/* from video_hw.c */
extern struct video_layer_s vd_layer[MAX_VD_LAYER];
extern struct disp_info_s glayer_info[MAX_VD_LAYER];
extern struct video_dev_s *cur_dev;
extern bool legacy_vpp;
extern bool hscaler_8tap_enable[MAX_VD_LAYER];
extern struct pre_scaler_info pre_scaler[MAX_VD_LAYER];
extern bool vd1_vd2_mux;
extern bool aisr_en;
extern u32 vd1_vd2_mux_dts;
extern u32 osd_vpp1_bld_ctrl;
extern u32 osd_vpp2_bld_ctrl;
extern bool update_osd_vpp1_bld_ctrl;
extern bool update_osd_vpp2_bld_ctrl;
extern int vdec_out_size_threshold_8k;
extern int vpp_in_size_threshold_8k;
extern int vdec_out_size_threshold_4k;
extern int vpp_in_size_threshold_4k;
extern u64 vsync_cnt[VPP_MAX];
extern struct vpu_venc_regs_s venc_regs[VPP_NUM];
extern u32 vpp_hold_line[VPP_MAX];

bool is_amdv_enable(void);
bool is_amdv_on(void);
bool is_amdv_stb_mode(void);
bool is_dovi_tv_on(void);
bool for_amdv_certification(void);

struct video_dev_s *get_video_cur_dev(void);
u32 get_video_enabled(u8 layer_id);
u32 get_video_onoff_state(u8 layer_id);

bool is_di_on(void);
bool is_di_post_on(void);
bool is_di_post_link_on(void);
bool is_di_post_mode(struct vframe_s *vf);
bool is_afbc_enabled(u8 layer_id);
bool is_local_vf(struct vframe_s *vf);
bool is_picmode_changed(u8 layer_id, struct vframe_s *vf);

void safe_switch_videolayer(u8 layer_id,
			    bool on, bool async);

#ifndef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
bool is_amdv_enable(void);
bool is_amdv_on(void);
bool is_amdv_stb_mode(void);
bool for_amdv_certification(void);
int is_amdv_frame(struct vframe_s *vf);
void amdv_set_toggle_flag(int flag);
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
void config_dvel_position(struct video_layer_s *layer,
			  struct mif_pos_s *setting,
			  struct vframe_s *el_vf);
void set_amdv_delay_work_flag(void);
#ifdef CONFIG_AMLOGIC_VOUT
s32 config_dvel_pps(struct video_layer_s *layer,
		    struct scaler_setting_s *setting,
		    const struct vinfo_s *info);
#endif
s32 config_dvel_blend(struct video_layer_s *layer,
		      struct blend_setting_s *setting,
		      struct vframe_s *dvel_vf);
#endif

#ifdef TV_3D_FUNCTION_OPEN
void config_3d_vd2_position(struct video_layer_s *layer,
			    struct mif_pos_s *setting);
s32 config_3d_vd2_pps(struct video_layer_s *layer,
		      struct scaler_setting_s *setting,
		      const struct vinfo_s *info);
s32 config_3d_vd2_blend(struct video_layer_s *layer,
			struct blend_setting_s *setting);
void switch_3d_view_per_vsync(struct video_layer_s *layer);
#endif
void config_vd_param(struct video_layer_s *layer,
	struct vframe_s *dispbuf);
s32 config_vd_position(struct video_layer_s *layer,
		       struct mif_pos_s *setting);
s32 config_vd_pps(struct video_layer_s *layer,
		  struct scaler_setting_s *setting,
		  const struct vinfo_s *info);
s32 config_vd_blend(struct video_layer_s *layer,
		    struct blend_setting_s *setting);
void vd_set_dcu(u8 layer_id,
		struct video_layer_s *layer,
		struct vpp_frame_par_s *frame_par,
		struct vframe_s *vf);
void vd_mif_setting(struct video_layer_s *layer,
		    struct mif_pos_s *setting);
void vd_scaler_setting(struct video_layer_s *layer,
		       struct scaler_setting_s *setting);
void vd_blend_setting(struct video_layer_s *layer,
		      struct blend_setting_s *setting);
void vd_clip_setting(u8 vpp_index, u8 layer_id,
		     struct clip_setting_s *setting);
void proc_vd_vsc_phase_per_vsync(struct video_layer_s *layer,
				 struct vpp_frame_par_s *frame_par,
				 struct vframe_s *vf);
void vpp_blend_update(const struct vinfo_s *vinfo, u8 vpp_index);
void vpp_blend_update_t7(const struct vinfo_s *vinfo);
void vpp_blend_update_c3(const struct vinfo_s *vinfo);
void vppx_vd_blend_setting(struct video_layer_s *layer, struct blend_setting_s *setting);
void vppx_blend_update(const struct vinfo_s *vinfo, u32 vpp_index);
int get_layer_display_canvas(u8 layer_id);
int set_layer_display_canvas(struct video_layer_s *layer,
			     struct vframe_s *vf,
			     struct vpp_frame_par_s *cur_frame_par,
			     struct disp_info_s *disp_info, u32 line);
int set_layer_slice_display_canvas_s5(struct video_layer_s *layer,
			     struct vframe_s *vf,
			     struct vpp_frame_par_s *cur_frame_par,
			     struct disp_info_s *disp_info,
			     u32 slice, u32 line);
int set_layer_mosaic_display_canvas_s5(struct video_layer_s *layer,
			     struct vframe_s *vf,
			     struct vpp_frame_par_s *cur_frame_par,
			     struct disp_info_s *disp_info,
			     u32 slice,
			     u8 frame_id);
u32 *get_canvase_tbl(u8 layer_id);
s32 layer_swap_frame(struct vframe_s *vf, struct video_layer_s *layer,
		     bool force_toggle,
		     const struct vinfo_s *vinfo, u32 swap_op_flag);
int detect_vout_type(const struct vinfo_s *vinfo);
int calc_hold_line(void);
u32 get_active_start_line(void);
u32 get_cur_enc_line(void);
u32 get_cur_enc_num(void);
void vpu_work_process(void);
int vpp_crc_check(u32 vpp_crc_en, u8 vpp_index);
void enable_vpp_crc_viu2(u32 vpp_crc_en);
int vpp_crc_viu2_check(u32 vpp_crc_en);
void dump_pps_coefs_info(u8 layer_id, u8 bit9_mode, u8 coef_type);
struct video_layer_s *get_layer_by_layer_id(u8 layer_id);
void update_vd_src_info(u8 layer_id,
							u32 src_width, u32 src_height,
							u32 compWidth, u32 compHeight);
bool is_bandwidth_policy_hit(u8 layer_id);
int video_hw_init(void);
int video_early_init(struct amvideo_device_data_s *p_amvideo);
int video_late_uninit(void);

int video_hw_init_s5(void);
int _video_hw_init_s5(void);
int video_early_init_s5(struct amvideo_device_data_s *p_amvideo);
void vd_scaler_setting_s5(struct video_layer_s *layer,
		       struct scaler_setting_s *setting);
void vd_set_dcu_s5(u8 layer_id,
		struct video_layer_s *layer,
		struct vpp_frame_par_s *frame_par,
		struct vframe_s *vf);
void switch_3d_view_per_vsync_s5(struct video_layer_s *layer);
void aisr_reshape_cfg_s5(struct video_layer_s *layer,
		     struct aisr_setting_s *aisr_mif_setting);
void aisr_scaler_setting_s5(struct video_layer_s *layer,
			     struct scaler_setting_s *setting);
void vd_blend_setting_s5(struct video_layer_s *layer, struct blend_setting_s *setting);
void rx_mute_vpp_s5(u32 black_val);
void vd_clip_setting_s5(u8 vpp_index, u8 layer_id,
	struct clip_setting_s *setting);
void vpp_post_blend_update_s5(const struct vinfo_s *vinfo, u8 vpp_index);
void vpp1_post_blend_update_s5(const struct vinfo_s *vinfo);
void adjust_vpp_filter_parm(struct vpp_frame_par_s *frame_par,
	u32 supsc1_hori_ratio,
	u32 supsc1_vert_ratio,
	u32 horz_phase_step,
	u32 vert_phase_step);
struct video_layer_s *get_vd_layer(u8 layer_id);
void _set_video_mirror(struct disp_info_s *layer, int mirror);
void _set_video_window(struct disp_info_s *layer, int *p);
void _set_video_crop(struct disp_info_s *layer, int *p);
struct mosaic_frame_s *get_mosaic_vframe_info(u32 slice);
void get_mosaic_axis(void);
void set_mosaic_axis(u32 pic_index, u32 x_start, u32 y_start,
	u32 x_end, u32 y_end);
void video_resume_hw_recovery(void);

/* from video.c */
extern u32 osd_vpp_misc;
extern u32 osd_vpp_misc_mask;
extern bool update_osd_vpp_misc;
extern bool update_osd2_blend_src_ctrl;
extern u32 osd2_postbld_src;
extern u32 osd2_blend_path_sel;
extern u32 osd_preblend_en;
extern u32 framepacking_support;
extern u32 g_framepacking_support;
extern unsigned int framepacking_blank;
extern unsigned int process_3d_type;
#ifdef TV_3D_FUNCTION_OPEN
extern unsigned int force_3d_scaler;
extern int toggle_3d_fa_frame;
#endif
extern bool reverse;
extern struct vframe_s vf_local[MAX_VD_LAYER];
extern struct vframe_s vf_local2;
extern struct vframe_s vf_local_ext[MAX_VD_LAYER];

extern struct vframe_s *cur_dispbuf[MAX_VD_LAYER];
extern bool need_disable_vd[MAX_VD_LAYER];
extern u32 last_el_status;
extern u32 video_prop_status;
extern u32 force_blackout;
extern atomic_t video_unreg_flag;
extern atomic_t video_unreg_flag_vpp[2];
extern atomic_t video_inirq_flag;
extern atomic_t video_prevsync_inirq_flag;
extern atomic_t video_inirq_flag_vpp[2];
extern uint load_pps_coef;
extern atomic_t video_recv_cnt;
extern struct video_recv_s *gvideo_recv[3];
extern struct video_recv_s *gvideo_recv_vpp[2];
extern uint load_pps_coef;
extern atomic_t gafbc_request;
extern atomic_t video_unreg_flag;
extern atomic_t video_pause_flag;
extern atomic_t fmm_changed;
extern bool video_suspend;
extern u32 video_suspend_cycle;
extern int log_out;
extern int debug_flag;
extern bool bypass_pps;
extern bool rdma_enable_pre;
extern struct vpp_frame_par_s *cur_frame_par[MAX_VD_LAYER];
extern struct video_layer_s vd_layer_vpp[2];
extern u32 force_switch_vf_mode;
extern u32 video_info_change_status;
extern u32 reference_zorder;
extern u32 pi_enable;

bool black_threshold_check(u8 id);
bool black_threshold_check_s5(u8 id);
extern atomic_t primary_src_fmt;
extern atomic_t cur_primary_src_fmt;

struct vframe_s *get_cur_dispbuf(void);
s32 set_video_path_select(const char *recv_name, u8 layer_id);
s32 set_sideband_type(s32 type, u8 layer_id);

/*for video related files only.*/
void video_module_lock(void);
void video_module_unlock(void);
int get_video_debug_flags(void);
int _video_set_disable(u32 val);
int _videopip_set_disable(u32 index, u32 val);
struct device *get_video_device(void);
#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_DOLBYVISION
struct vframe_s *dv_toggle_frame(struct vframe_s *vf,
				enum vd_path_e vd_path, bool new_frame);
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_VIDEOCAPTURE
int ext_frame_capture_poll(int endflags);
#endif
bool is_meson_tm2_revb(void);
bool video_is_meson_sc2_cpu(void);
bool video_is_meson_t5d_cpu(void);
bool video_is_meson_t7_cpu(void);
bool video_is_meson_s4_cpu(void);
bool video_is_meson_t5d_revb_cpu(void);
bool video_is_meson_t3_cpu(void);
bool video_is_meson_c3_cpu(void);
bool video_is_meson_t5w_cpu(void);
bool video_is_meson_s5_cpu(void);
bool video_is_meson_t3x_cpu(void);
bool video_is_meson_t5m_cpu(void);
bool video_is_meson_txhd2_cpu(void);
bool video_is_meson_s1a_cpu(void);
bool video_is_meson_s7_cpu(void);
void alpha_win_set(struct video_layer_s *layer);
void fgrain_config(struct video_layer_s *layer,
		   struct vpp_frame_par_s *frame_par,
		   struct mif_pos_s *mif_setting,
		   struct fgrain_setting_s *setting,
		   struct vframe_s *vf);
void fgrain_setting(struct video_layer_s *layer,
		    struct fgrain_setting_s *setting,
		    struct vframe_s *vf);
void fgrain_update_table(struct video_layer_s *layer,
			 struct vframe_s *vf);
void video_secure_set(u8 vpp_index);
bool has_hscaler_8tap(u8 layer_id);
bool has_pre_hscaler_ntap(u8 layer_id);
bool has_pre_hscaler_8tap(u8 layer_id);
bool has_pre_vscaler_ntap(u8 layer_id);
void _set_video_window(struct disp_info_s *layer, int *p);
void _set_video_crop(struct disp_info_s *layer, int *p);
void set_alpha_scpxn(struct video_layer_s *layer,
			   struct composer_info_t *composer_info);
void di_used_vd1_afbc(bool di_used);
void pipx_swap_frame(struct video_layer_s *layer, struct vframe_s *vf,
		    const struct vinfo_s *vinfo);
s32 vdx_render_frame(struct video_layer_s *layer, const struct vinfo_s *vinfo);
bool aisr_update_frame_info(struct video_layer_s *layer,
			 struct vframe_s *vf);
void aisr_reshape_addr_set(struct video_layer_s *layer,
				  struct aisr_setting_s *aisr_mif_setting);
void aisr_reshape_cfg(struct video_layer_s *layer,
		      struct aisr_setting_s *aisr_mif_setting);
void aisr_scaler_setting(struct video_layer_s *layer,
				    struct scaler_setting_s *setting);
s32 config_aisr_pps(struct video_layer_s *layer,
			 struct scaler_setting_s *aisr_setting);
s32 config_aisr_position(struct video_layer_s *layer,
			     struct aisr_setting_s *aisr_mif_setting);
void aisr_demo_enable(void);
void aisr_demo_axis_set(struct video_layer_s *layer);
void aisr_reshape_output(u32 enable);
void pre_process_for_3d(struct vframe_s *vf);
int get_vpu_urgent_info_t3(void);
int set_vpu_super_urgent_t3(u32 module_id, u32 low_level, u32 high_level);
int get_vpu_urgent_info_t5m(void);
int set_vpu_super_urgent_t5m(u32 module_id, u32 urgent_level);
int set_vpu_super_urgent_t7(u32 module_id, u32 urgent_level);
#ifdef CONFIG_AMLOGIC_MEDIA_FRC
void update_frc_in_size(struct video_layer_s *layer);
#endif
#ifdef CONFIG_AMLOGIC_MEDIA_VSYNC_RDMA
void vsync_rdma_process(void);
#endif

#ifdef CONFIG_AMLOGIC_MEDIA_ENHANCEMENT_VECM
void amvecm_process(struct path_id_s *path_id, struct video_recv_s *p_gvideo_recv,
			    struct vframe_s *new_frame);
#endif
u32 get_force_skip_cnt(enum vd_path_e path);
bool is_pre_link_source(struct vframe_s *vf);
bool is_pre_link_on(struct video_layer_s *layer);
void vpp_trace_axis(int left, int top, int right, int bottom);
void vpp_trace_timeinfo(unsigned long time1,
	unsigned long time2, unsigned long time3,
	unsigned long time4, unsigned long time5,
	int duration);
void vpp_trace_encline(const char *sub_name, int start_line, int cur_line);
void vpp_trace_field_state(const char *sub_name,
	int cur_state, int new_state,
	int over_field, int cnt1, int cnt2);
void vpp_trace_vframe(const char *name, void *vf, int arg1, int arg2, int id, int cnt);
#ifdef ENABLE_PRE_LINK
bool is_pre_link_available(struct vframe_s *vf);
#endif

#ifdef TV_REVERSE
int screen_orientation(void);
#endif

void hdmi_in_delay_maxmin_old(struct vframe_s *vf);
void hdmi_in_delay_maxmin_new(struct vframe_s *vf);
int vpp_set_super_scaler_regs(struct video_layer_s *layer,
			      int scaler_path_sel,
			      int reg_srscl0_enable,
			      int reg_srscl0_hsize,
			      int reg_srscl0_vsize,
			      int reg_srscl0_hori_ratio,
			      int reg_srscl0_vert_ratio,
			      int reg_srscl1_enable,
			      int reg_srscl1_hsize,
			      int reg_srscl1_vsize,
			      int reg_srscl1_hori_ratio,
			      int reg_srscl1_vert_ratio,
			      int vpp_postblend_out_width,
			      int vpp_postblend_out_height);

#ifndef CONFIG_AMLOGIC_MEDIA_FRAME_SYNC
enum avevent_e {
	VIDEO_START,
	VIDEO_PAUSE,
	VIDEO_STOP,
	VIDEO_TSTAMP_DISCONTINUITY,
	AUDIO_START,
	AUDIO_PAUSE,
	AUDIO_RESUME,
	AUDIO_STOP,
	AUDIO_TSTAMP_DISCONTINUITY,
	AUDIO_PRE_START,
	AUDIO_WAIT
};

enum tsync_mode_e {
	TSYNC_MODE_VMASTER,
	TSYNC_MODE_AMASTER,
	TSYNC_MODE_PCRMASTER,
};

enum {
	PTS_TYPE_VIDEO = 0,
	PTS_TYPE_AUDIO = 1,
	PTS_TYPE_HEVC = 2,
	PTS_TYPE_MAX = 3
};

static inline void tsync_avevent_locked(enum avevent_e event, u32 param) {}
static inline void tsync_avevent(enum avevent_e event, u32 param) {}
static inline int tsync_get_mode(void) { return 0; }
static inline void tsync_set_mode(int mode) {}
static inline void tsync_set_enable(int enable) {}
static inline void tsync_set_syncthresh(unsigned int sync_thresh) {}

static inline int tsync_get_sync_adiscont(void) { return 0; }
static inline int tsync_get_sync_vdiscont(void) { return 0; }
static inline void tsync_set_sync_adiscont(int syncdiscont) {}
static inline void tsync_set_sync_vdiscont(int syncdiscont) {}
static inline u32 tsync_get_sync_adiscont_diff(void) { return 0; }
static inline u32 tsync_get_sync_vdiscont_diff(void) { return 0; }
static inline void tsync_set_sync_adiscont_diff(u32 discontinue_diff) {}
static inline void tsync_set_sync_vdiscont_diff(u32 discontinue_diff) {}
static inline void tsync_trick_mode(int trick_mode) {}
static inline int tsync_set_tunnel_mode(int mode) { return 0; }
static inline void tsync_set_avthresh(unsigned int av_thresh) {}
static inline u32 tsync_vpts_discontinuity_margin(void)
{
	return 0;
}

static inline u32 timestamp_vpts_get(void) { return 0; }
static inline void timestamp_vpts_set(u32 pts) {}
static inline void timestamp_vpts_inc(s32 val)  {}
static inline u32 timestamp_apts_get(void)  { return 0; }
static inline void timestamp_apts_set(u32 pts) {}
static inline void timestamp_apts_inc(s32 val) {}
static inline u32 timestamp_pcrscr_get(void) { return 0; }
static inline void timestamp_pcrscr_set(u32 pts) {}
static inline void timestamp_pcrscr_inc(s32 val) {}
static inline void timestamp_pcrscr_inc_scale(s32 inc, u32 base) {}
static inline void timestamp_pcrscr_enable(u32 enable) {}
static inline u32 timestamp_pcrscr_enable_state(void) { return 0; }

static inline int calculation_stream_delayed_ms(u8 type, u32 *latest_bitrate,
						u32 *avg_bitare)
{return 0; }

static inline bool tsync_check_vpts_discontinuity(unsigned int vpts)
{
	return false;
}
#endif

#ifndef CONFIG_AMLOGIC_VOUT
struct vinfo_s *get_current_vinfo(void);
#endif

#ifndef CONFIG_AMLOGIC_MEDIA_FRC
static inline int frc_get_n2m_setting(void) { return 1; }
#endif
#endif
/*VIDEO_PRIV_HEADER_HH*/
