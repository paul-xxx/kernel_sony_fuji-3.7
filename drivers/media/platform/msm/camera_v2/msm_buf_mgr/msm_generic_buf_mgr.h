/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MSM_BUF_GENERIC_MNGR_H__
#define __MSM_BUF_GENERIC_MNGR_H__

#include <linux/io.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <media/v4l2-subdev.h>
#include <media/msmb_camera.h>
#include <media/msmb_generic_buf_mgr.h>

#include "msm.h"
#include "msm_sd.h"

struct msm_get_bufs {
	struct list_head list;
	struct vb2_buffer *vb2_buf;
};

struct msm_buf_mngr_device {
	struct msm_sd_subdev subdev;
	struct msm_sd_req_vb2_q vb2_ops;
	struct msm_get_bufs bufs;
};

#endif
