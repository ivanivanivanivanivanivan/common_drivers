/*
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Description:
 */
#ifndef VDEC_PROFILE_H
#define VDEC_PROFILE_H

struct vdec_s;

#define VDEC_PROFILE_EVENT_RUN         0
#define VDEC_PROFILE_EVENT_CB          1
#define VDEC_PROFILE_EVENT_SAVE_INPUT  2
#define VDEC_PROFILE_EVENT_CHK_RUN_READY 3
#define VDEC_PROFILE_EVENT_RUN_READY   4
#define VDEC_PROFILE_EVENT_DISCONNECT  5
#define VDEC_PROFILE_EVENT_DEC_WORK    6
#define VDEC_PROFILE_EVENT_INFO        7
#define VDEC_PROFILE_DECODED_FRAME      8
#define VDEC_PROFILE_EVENT_WAIT_BACK_CORE    9
#define VDEC_PROFILE_EVENT_AGAIN       10
#define VDEC_PROFILE_DECODER_START     11
#define VDEC_PROFILE_DECODER_END       12
#define VDEC_PROFILE_MAX_EVENT         13

extern uint dec_time_stat_reset;

extern void vdec_profile(struct vdec_s *vdec, int event, int mask);
extern void vdec_profile_more(struct vdec_s *vdec, int event, int para1, int para2, int mask);
extern void vdec_profile_flush(struct vdec_s *vdec);

int vdec_profile_init_debugfs(void);
void vdec_profile_exit_debugfs(void);

#endif /* VDEC_PROFILE_H */
