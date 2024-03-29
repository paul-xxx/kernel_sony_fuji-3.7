/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/slab.h>

#include "msm_vidc_internal.h"
#include "msm_vidc_common.h"
#include "vidc_hfi_api.h"
#include "msm_smem.h"
#include "msm_vidc_debug.h"

#define MSM_VDEC_DVC_NAME "msm_vdec_8974"
#define DEFAULT_HEIGHT 720
#define DEFAULT_WIDTH 1280
#define MAX_SUPPORTED_WIDTH 3820
#define MAX_SUPPORTED_HEIGHT 2160
#define MIN_NUM_OUTPUT_BUFFERS 4
#define MAX_NUM_OUTPUT_BUFFERS 6

enum msm_vdec_ctrl_cluster {
	MSM_VDEC_CTRL_CLUSTER_MAX = 1,
};

static const char *const mpeg_video_vidc_divx_format[] = {
	"DIVX Format 3",
	"DIVX Format 4",
	"DIVX Format 5",
	"DIVX Format 6",
	NULL
};
static const char *mpeg_video_stream_format[] = {
	"NAL Format Start Codes",
	"NAL Format One NAL Per Buffer",
	"NAL Format One Byte Length",
	"NAL Format Two Byte Length",
	"NAL Format Four Byte Length",
	NULL
};
static const char *const mpeg_video_output_order[] = {
	"Display Order",
	"Decode Order",
	NULL
};
static const char *const mpeg_video_vidc_extradata[] = {
	"Extradata none",
	"Extradata MB Quantization",
	"Extradata Interlace Video",
	"Extradata VC1 Framedisp",
	"Extradata VC1 Seqdisp",
	"Extradata timestamp",
	"Extradata S3D Frame Packing",
	"Extradata Frame Rate",
	"Extradata Panscan Window",
	"Extradata Recovery point SEI",
	"Extradata Closed Caption UD",
	"Extradata AFD UD",
	"Extradata Multislice info",
	"Extradata number of concealed MB",
	"Extradata metadata filler",
	"Extradata input crop",
	"Extradata digital zoom",
	"Extradata aspect ratio",
};

static struct msm_vidc_ctrl msm_vdec_ctrls[] = {
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_STREAM_FORMAT,
		.name = "NAL Format",
		.type = V4L2_CTRL_TYPE_MENU,
		.minimum = V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_STARTCODES,
		.maximum = V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_FOUR_BYTE_LENGTH,
		.default_value = V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_STARTCODES,
		.menu_skip_mask = ~(
		(1 << V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_STARTCODES) |
		(1 << V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_ONE_NAL_PER_BUFFER) |
		(1 << V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_ONE_BYTE_LENGTH) |
		(1 << V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_TWO_BYTE_LENGTH) |
		(1 << V4L2_MPEG_VIDC_VIDEO_NAL_FORMAT_FOUR_BYTE_LENGTH)
		),
		.qmenu = mpeg_video_stream_format,
		.step = 0,
		.cluster = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_OUTPUT_ORDER,
		.name = "Output Order",
		.type = V4L2_CTRL_TYPE_MENU,
		.minimum = V4L2_MPEG_VIDC_VIDEO_OUTPUT_ORDER_DISPLAY,
		.maximum = V4L2_MPEG_VIDC_VIDEO_OUTPUT_ORDER_DECODE,
		.default_value = V4L2_MPEG_VIDC_VIDEO_OUTPUT_ORDER_DISPLAY,
		.menu_skip_mask = ~(
			(1 << V4L2_MPEG_VIDC_VIDEO_OUTPUT_ORDER_DISPLAY) |
			(1 << V4L2_MPEG_VIDC_VIDEO_OUTPUT_ORDER_DECODE)
			),
		.qmenu = mpeg_video_output_order,
		.step = 0,
		.cluster = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_ENABLE_PICTURE_TYPE,
		.name = "Picture Type Decoding",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 1,
		.maximum = 15,
		.default_value = 15,
		.step = 1,
		.menu_skip_mask = 0,
		.qmenu = NULL,
		.cluster = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_KEEP_ASPECT_RATIO,
		.name = "Keep Aspect Ratio",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.minimum = 0,
		.maximum = 1,
		.default_value = 0,
		.step = 1,
		.menu_skip_mask = 0,
		.qmenu = NULL,
		.cluster = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_POST_LOOP_DEBLOCKER_MODE,
		.name = "Deblocker Mode",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.minimum = 0,
		.maximum = 1,
		.default_value = 0,
		.step = 1,
		.menu_skip_mask = 0,
		.qmenu = NULL,
		.cluster = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_DIVX_FORMAT,
		.name = "Divx Format",
		.type = V4L2_CTRL_TYPE_MENU,
		.minimum = V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_4,
		.maximum = V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_6,
		.default_value = V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_4,
		.menu_skip_mask = ~(
			(1 << V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_4) |
			(1 << V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_5) |
			(1 << V4L2_MPEG_VIDC_VIDEO_DIVX_FORMAT_6)
			),
		.qmenu = mpeg_video_vidc_divx_format,
		.step = 0,
		.cluster = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_MB_ERROR_MAP_REPORTING,
		.name = "MB Error Map Reporting",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.minimum = 0,
		.maximum = 1,
		.default_value = 0,
		.step = 1,
		.menu_skip_mask = 0,
		.qmenu = NULL,
		.cluster = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_CONTINUE_DATA_TRANSFER,
		.name = "control",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.minimum = 0,
		.maximum = 1,
		.default_value = 0,
		.step = 1,
		.menu_skip_mask = 0,
		.qmenu = NULL,
		.cluster = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_SYNC_FRAME_DECODE,
		.name = "Sync Frame Decode",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.minimum = V4L2_MPEG_VIDC_VIDEO_SYNC_FRAME_DECODE_DISABLE,
		.maximum = V4L2_MPEG_VIDC_VIDEO_SYNC_FRAME_DECODE_ENABLE,
		.default_value = V4L2_MPEG_VIDC_VIDEO_SYNC_FRAME_DECODE_DISABLE,
	},
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_SECURE,
		.name = "Secure mode",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.minimum = 0,
		.maximum = 0,
		.default_value = 0,
		.step = 0,
		.menu_skip_mask = 0,
		.qmenu = NULL,
		.cluster = 0,
	},
	{
		.id = V4L2_CID_MPEG_VIDC_VIDEO_EXTRADATA,
		.name = "Extradata Type",
		.type = V4L2_CTRL_TYPE_MENU,
		.minimum = V4L2_MPEG_VIDC_EXTRADATA_NONE,
		.maximum = V4L2_MPEG_VIDC_INDEX_EXTRADATA_ASPECT_RATIO,
		.default_value = V4L2_MPEG_VIDC_EXTRADATA_NONE,
		.menu_skip_mask = ~(
			(1 << V4L2_MPEG_VIDC_EXTRADATA_NONE) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_MB_QUANTIZATION) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_INTERLACE_VIDEO) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_VC1_FRAMEDISP) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_VC1_SEQDISP) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_TIMESTAMP) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_S3D_FRAME_PACKING) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_FRAME_RATE) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_PANSCAN_WINDOW) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_RECOVERY_POINT_SEI) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_CLOSED_CAPTION_UD) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_AFD_UD) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_MULTISLICE_INFO) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_NUM_CONCEALED_MB) |
			(1 << V4L2_MPEG_VIDC_EXTRADATA_METADATA_FILLER) |
			(1 << V4L2_MPEG_VIDC_INDEX_EXTRADATA_INPUT_CROP) |
			(1 << V4L2_MPEG_VIDC_INDEX_EXTRADATA_DIGITAL_ZOOM) |
			(1 << V4L2_MPEG_VIDC_INDEX_EXTRADATA_ASPECT_RATIO)
			),
		.qmenu = mpeg_video_vidc_extradata,
		.step = 0,
	},
};

#define NUM_CTRLS ARRAY_SIZE(msm_vdec_ctrls)

static u32 get_frame_size_nv12(int plane,
					u32 height, u32 width)
{
	return VENUS_BUFFER_SIZE(COLOR_FMT_NV12, width, height);
}

static u32 get_frame_size_compressed(int plane,
					u32 height, u32 width)
{
	return (MAX_SUPPORTED_WIDTH * MAX_SUPPORTED_HEIGHT * 3/2)/2;
}

static const struct msm_vidc_format vdec_formats[] = {
	{
		.name = "YCbCr Semiplanar 4:2:0",
		.description = "Y/CbCr 4:2:0",
		.fourcc = V4L2_PIX_FMT_NV12,
		.num_planes = 2,
		.get_frame_size = get_frame_size_nv12,
		.type = CAPTURE_PORT,
	},
	{
		.name = "Mpeg4",
		.description = "Mpeg4 compressed format",
		.fourcc = V4L2_PIX_FMT_MPEG4,
		.num_planes = 1,
		.get_frame_size = get_frame_size_compressed,
		.type = OUTPUT_PORT,
	},
	{
		.name = "Mpeg2",
		.description = "Mpeg2 compressed format",
		.fourcc = V4L2_PIX_FMT_MPEG2,
		.num_planes = 1,
		.get_frame_size = get_frame_size_compressed,
		.type = OUTPUT_PORT,
	},
	{
		.name = "H263",
		.description = "H263 compressed format",
		.fourcc = V4L2_PIX_FMT_H263,
		.num_planes = 1,
		.get_frame_size = get_frame_size_compressed,
		.type = OUTPUT_PORT,
	},
	{
		.name = "VC1",
		.description = "VC-1 compressed format",
		.fourcc = V4L2_PIX_FMT_VC1_ANNEX_G,
		.num_planes = 1,
		.get_frame_size = get_frame_size_compressed,
		.type = OUTPUT_PORT,
	},
	{
		.name = "VC1 SP",
		.description = "VC-1 compressed format G",
		.fourcc = V4L2_PIX_FMT_VC1_ANNEX_L,
		.num_planes = 1,
		.get_frame_size = get_frame_size_compressed,
		.type = OUTPUT_PORT,
	},
	{
		.name = "H264",
		.description = "H264 compressed format",
		.fourcc = V4L2_PIX_FMT_H264,
		.num_planes = 1,
		.get_frame_size = get_frame_size_compressed,
		.type = OUTPUT_PORT,
	},
	{
		.name = "VP8",
		.description = "VP8 compressed format",
		.fourcc = V4L2_PIX_FMT_VP8,
		.num_planes = 1,
		.get_frame_size = get_frame_size_compressed,
		.type = OUTPUT_PORT,
	},
	{
		.name = "DIVX 311",
		.description = "DIVX 311 compressed format",
		.fourcc = V4L2_PIX_FMT_DIVX_311,
		.num_planes = 1,
		.get_frame_size = get_frame_size_compressed,
		.type = OUTPUT_PORT,
	},
	{
		.name = "DIVX",
		.description = "DIVX 4/5/6 compressed format",
		.fourcc = V4L2_PIX_FMT_DIVX,
		.num_planes = 1,
		.get_frame_size = get_frame_size_compressed,
		.type = OUTPUT_PORT,
	}
};

int msm_vdec_streamon(struct msm_vidc_inst *inst, enum v4l2_buf_type i)
{
	int rc = 0;
	struct buf_queue *q;
	q = msm_comm_get_vb2q(inst, i);
	if (!q) {
		dprintk(VIDC_ERR,
			"Failed to find buffer queue for type = %d\n", i);
		return -EINVAL;
	}
	dprintk(VIDC_DBG, "Calling streamon\n");
	mutex_lock(&q->lock);
	rc = vb2_streamon(&q->vb2_bufq, i);
	mutex_unlock(&q->lock);
	if (rc)
		dprintk(VIDC_ERR, "streamon failed on port: %d\n", i);
	return rc;
}

int msm_vdec_streamoff(struct msm_vidc_inst *inst, enum v4l2_buf_type i)
{
	int rc = 0;
	struct buf_queue *q;

	q = msm_comm_get_vb2q(inst, i);
	if (!q) {
		dprintk(VIDC_ERR,
			"Failed to find buffer queue for type = %d\n", i);
		return -EINVAL;
	}
	dprintk(VIDC_DBG, "Calling streamoff\n");
	mutex_lock(&q->lock);
	rc = vb2_streamoff(&q->vb2_bufq, i);
	mutex_unlock(&q->lock);
	if (rc)
		dprintk(VIDC_ERR, "streamoff failed on port: %d\n", i);
	return rc;
}

int msm_vdec_prepare_buf(struct msm_vidc_inst *inst,
					struct v4l2_buffer *b)
{
	int rc = 0;
	struct vidc_buffer_addr_info buffer_info;
	int extra_idx = 0;
	int i;
	struct hfi_device *hdev;

	if (!inst || !inst->core || !inst->core->device) {
		dprintk(VIDC_ERR, "%s invalid parameters", __func__);
		return -EINVAL;
	}
	hdev = inst->core->device;

	switch (b->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
			if (b->length != inst->fmts[CAPTURE_PORT]->num_planes) {
				dprintk(VIDC_ERR,
				"Planes mismatch: needed: %d, allocated: %d\n",
				inst->fmts[CAPTURE_PORT]->num_planes,
				b->length);
				rc = -EINVAL;
				break;
			}
			for (i = 0; (i < b->length)
				&& (i < VIDEO_MAX_PLANES); ++i) {
				dprintk(VIDC_DBG,
				"prepare plane: %d, device_addr = 0x%lx, size = %d\n",
				i, b->m.planes[i].m.userptr,
				b->m.planes[i].length);
			}
			buffer_info.buffer_size = b->m.planes[0].length;
			buffer_info.buffer_type = HAL_BUFFER_OUTPUT;
			buffer_info.num_buffers = 1;
			buffer_info.align_device_addr =
				b->m.planes[0].m.userptr;
			extra_idx = EXTRADATA_IDX(b->length);
			if (extra_idx && (extra_idx < VIDEO_MAX_PLANES) &&
				b->m.planes[extra_idx].m.userptr) {
				buffer_info.extradata_addr =
					b->m.planes[extra_idx].m.userptr;
				dprintk(VIDC_DBG,
				"extradata: 0x%lx\n",
				b->m.planes[extra_idx].m.userptr);
				buffer_info.extradata_size =
					b->m.planes[extra_idx].length;
			} else {
				buffer_info.extradata_addr = 0;
				buffer_info.extradata_size = 0;
			}
			rc = call_hfi_op(hdev, session_set_buffers,
					(void *)inst->session, &buffer_info);
			if (rc)
				dprintk(VIDC_ERR,
				"vidc_hal_session_set_buffers failed");
		break;
	default:
		dprintk(VIDC_ERR, "Buffer type not recognized: %d\n", b->type);
		break;
	}
	return rc;
}

int msm_vdec_release_buf(struct msm_vidc_inst *inst,
					struct v4l2_buffer *b)
{
	int rc = 0;
	struct vidc_buffer_addr_info buffer_info;
	struct msm_vidc_core *core = inst->core;
	int extra_idx = 0;
	int i;
	struct hfi_device *hdev;

	if (!inst || !inst->core || !inst->core->device) {
		dprintk(VIDC_ERR, "%s invalid parameters", __func__);
		return -EINVAL;
	}

	hdev = inst->core->device;

	if (inst->state == MSM_VIDC_CORE_INVALID ||
			core->state == VIDC_CORE_INVALID) {
		dprintk(VIDC_ERR,
			"Core %p in bad state, ignoring release output buf\n",
				core);
		goto exit;
	}
	if (!inst->in_reconfig) {
		rc = msm_comm_try_state(inst, MSM_VIDC_RELEASE_RESOURCES_DONE);
		if (rc) {
			dprintk(VIDC_ERR,
				"Failed to move inst: %p to relase res done\n",
				inst);
			goto exit;
		}
	}
	switch (b->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
			if (b->length !=
				inst->fmts[CAPTURE_PORT]->num_planes) {
				dprintk(VIDC_ERR,
				"Planes mismatch: needed: %d, to release: %d\n",
				inst->fmts[CAPTURE_PORT]->num_planes,
				b->length);
				rc = -EINVAL;
				break;
			}
			for (i = 0; i < b->length; ++i) {
				dprintk(VIDC_DBG,
				"Release plane: %d device_addr = 0x%lx, size = %d\n",
				i, b->m.planes[i].m.userptr,
				b->m.planes[i].length);
			}
			buffer_info.buffer_size = b->m.planes[0].length;
			buffer_info.buffer_type = HAL_BUFFER_OUTPUT;
			buffer_info.num_buffers = 1;
			buffer_info.align_device_addr =
				 b->m.planes[0].m.userptr;
			extra_idx = EXTRADATA_IDX(b->length);
			if (extra_idx && (extra_idx < VIDEO_MAX_PLANES)
				&& b->m.planes[extra_idx].m.userptr)
				buffer_info.extradata_addr =
					b->m.planes[extra_idx].m.userptr;
			else
				buffer_info.extradata_addr = 0;
			buffer_info.response_required = false;
			rc = call_hfi_op(hdev, session_release_buffers,
				(void *)inst->session, &buffer_info);
			if (rc)
				dprintk(VIDC_ERR,
				"vidc_hal_session_release_buffers failed");
		break;
	default:
		dprintk(VIDC_ERR, "Buffer type not recognized: %d\n", b->type);
		break;
	}
exit:
	return rc;
}

int msm_vdec_qbuf(struct msm_vidc_inst *inst, struct v4l2_buffer *b)
{
	struct buf_queue *q = NULL;
	int rc = 0;
	q = msm_comm_get_vb2q(inst, b->type);
	if (!q) {
		dprintk(VIDC_ERR, "Failed to find buffer queue for type = %d\n"
			, b->type);
		return -EINVAL;
	}
	mutex_lock(&q->lock);
	rc = vb2_qbuf(&q->vb2_bufq, b);
	mutex_unlock(&q->lock);
	if (rc)
		dprintk(VIDC_ERR, "Failed to qbuf, %d\n", rc);
	return rc;
}
int msm_vdec_dqbuf(struct msm_vidc_inst *inst, struct v4l2_buffer *b)
{
	struct buf_queue *q = NULL;
	int rc = 0;
	q = msm_comm_get_vb2q(inst, b->type);
	if (!q) {
		dprintk(VIDC_ERR, "Failed to find buffer queue for type = %d\n"
			, b->type);
		return -EINVAL;
	}
	mutex_lock(&q->lock);
	rc = vb2_dqbuf(&q->vb2_bufq, b, true);
	mutex_unlock(&q->lock);
	if (rc)
		dprintk(VIDC_DBG, "Failed to dqbuf, %d\n", rc);
	return rc;
}

int msm_vdec_reqbufs(struct msm_vidc_inst *inst, struct v4l2_requestbuffers *b)
{
	struct buf_queue *q = NULL;
	int rc = 0;
	if (!inst || !b) {
		dprintk(VIDC_ERR,
			"Invalid input, inst = %p, buffer = %p\n", inst, b);
		return -EINVAL;
	}
	q = msm_comm_get_vb2q(inst, b->type);
	if (!q) {
		dprintk(VIDC_ERR, "Failed to find buffer queue for type = %d\n"
			, b->type);
		return -EINVAL;
	}

	mutex_lock(&q->lock);
	rc = vb2_reqbufs(&q->vb2_bufq, b);
	mutex_unlock(&q->lock);
	if (rc)
		dprintk(VIDC_ERR, "Failed to get reqbufs, %d\n", rc);
	return rc;
}

int msm_vdec_g_fmt(struct msm_vidc_inst *inst, struct v4l2_format *f)
{
	const struct msm_vidc_format *fmt = NULL;
	struct hal_frame_size frame_sz;
	struct hfi_device *hdev;
	int stride, scanlines;
	int extra_idx = 0;
	int rc = 0;
	int ret;
	int i;
	if (!inst || !f || !inst->core || !inst->core->device) {
		dprintk(VIDC_ERR,
			"Invalid input, inst = %p, format = %p\n", inst, f);
		return -EINVAL;
	}
	hdev = inst->core->device;
	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		fmt = inst->fmts[CAPTURE_PORT];
	else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		fmt = inst->fmts[OUTPUT_PORT];

	if (fmt) {
		f->fmt.pix_mp.pixelformat = fmt->fourcc;
		f->fmt.pix_mp.num_planes = fmt->num_planes;
		if (inst->in_reconfig == true) {
			inst->prop.height = inst->reconfig_height;
			inst->prop.width = inst->reconfig_width;
		}
		f->fmt.pix_mp.height = inst->prop.height;
		f->fmt.pix_mp.width = inst->prop.width;
		stride = inst->prop.width;
		scanlines = inst->prop.height;
		frame_sz.buffer_type = HAL_BUFFER_OUTPUT;
		frame_sz.width = inst->prop.width;
		frame_sz.height = inst->prop.height;
		dprintk(VIDC_DBG, "width = %d, height = %d\n",
				frame_sz.width, frame_sz.height);
		ret = msm_comm_try_set_prop(inst,
			HAL_PARAM_FRAME_SIZE, &frame_sz);
		ret = ret || msm_comm_try_get_bufreqs(inst);
		if (ret || (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)) {
			for (i = 0; i < fmt->num_planes; ++i) {
				f->fmt.pix_mp.plane_fmt[i].sizeimage =
					fmt->get_frame_size(i,
						f->fmt.pix_mp.height,
						f->fmt.pix_mp.width);
				inst->bufq[OUTPUT_PORT].
					vb2_bufq.plane_sizes[i] =
					f->fmt.pix_mp.plane_fmt[i].sizeimage;
			}
		} else {
			switch (fmt->fourcc) {
			case V4L2_PIX_FMT_NV12:
				call_hfi_op(hdev, get_stride_scanline,
					COLOR_FMT_NV12,
					inst->prop.width, inst->prop.height,
					&stride, &scanlines);
				break;
			default:
				dprintk(VIDC_WARN,
					"Color format not recognized\n");
			}
			f->fmt.pix_mp.plane_fmt[0].sizeimage =
			inst->buff_req.buffer[HAL_BUFFER_OUTPUT].buffer_size;
			extra_idx = EXTRADATA_IDX(fmt->num_planes);
			if (extra_idx && (extra_idx < VIDEO_MAX_PLANES)) {
				f->fmt.pix_mp.plane_fmt[extra_idx].sizeimage =
		inst->buff_req.buffer[HAL_BUFFER_EXTRADATA_OUTPUT].buffer_size;
			}
			for (i = 0; i < fmt->num_planes; ++i)
				inst->bufq[CAPTURE_PORT].
					vb2_bufq.plane_sizes[i] =
					f->fmt.pix_mp.plane_fmt[i].sizeimage;
		}
		if (stride && scanlines) {
			f->fmt.pix_mp.plane_fmt[0].bytesperline =
				(__u16)stride;
			f->fmt.pix_mp.plane_fmt[0].reserved[0] =
				(__u16)scanlines;
		} else {
			f->fmt.pix_mp.plane_fmt[0].bytesperline =
				(__u16)inst->prop.width;
			f->fmt.pix_mp.plane_fmt[0].reserved[0] =
				(__u16)inst->prop.height;
		}
	} else {
		dprintk(VIDC_ERR,
			"Buf type not recognized, type = %d\n",
			f->type);
		rc = -EINVAL;
	}
	return rc;
}
int msm_vdec_s_parm(struct msm_vidc_inst *inst, struct v4l2_streamparm *a)
{
	u32 us_per_frame = 0;
	int rc = 0;
	if (a->parm.output.timeperframe.denominator) {
		switch (a->type) {
		case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
			us_per_frame = a->parm.output.timeperframe.numerator/
				a->parm.output.timeperframe.denominator;
			break;
		case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
			us_per_frame = a->parm.capture.timeperframe.numerator/
				a->parm.capture.timeperframe.denominator;
			break;
		default:
			dprintk(VIDC_ERR,
				"Scale clocks : Unknown buffer type\n");
			break;
		}
	}
	if (!us_per_frame) {
		dprintk(VIDC_ERR,
				"Failed to scale clocks : time between frames is 0\n");
		rc = -EINVAL;
		goto exit;
	}
	inst->prop.fps = (u8) (USEC_PER_SEC / us_per_frame);
	if (inst->prop.fps) {
		msm_comm_scale_clocks_and_bus(inst);
	}
exit:
	return rc;
}
int msm_vdec_s_fmt(struct msm_vidc_inst *inst, struct v4l2_format *f)
{
	const struct msm_vidc_format *fmt = NULL;
	struct hal_frame_size frame_sz;
	int extra_idx = 0;
	int rc = 0;
	int ret = 0;
	int i;
	if (!inst || !f) {
		dprintk(VIDC_ERR,
			"Invalid input, inst = %p, format = %p\n", inst, f);
		return -EINVAL;
	}
	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		struct hal_frame_size frame_sz;

		fmt = msm_comm_get_pixel_fmt_fourcc(vdec_formats,
			ARRAY_SIZE(vdec_formats), f->fmt.pix_mp.pixelformat,
			CAPTURE_PORT);
		if (!fmt || (fmt && fmt->type != CAPTURE_PORT)) {
			dprintk(VIDC_ERR,
				"Format: %d not supported on CAPTURE port\n",
				f->fmt.pix_mp.pixelformat);
			rc = -EINVAL;
			goto err_invalid_fmt;
		}
		inst->fmts[fmt->type] = fmt;
		frame_sz.buffer_type = HAL_BUFFER_OUTPUT;
		frame_sz.width = inst->prop.width;
		frame_sz.height = inst->prop.height;
		dprintk(VIDC_DBG, "width = %d, height = %d\n",
				frame_sz.width, frame_sz.height);
		ret = msm_comm_try_set_prop(inst,
			HAL_PARAM_FRAME_SIZE, &frame_sz);
		ret = ret || msm_comm_try_get_bufreqs(inst);
		if (ret) {
			for (i = 0; i < fmt->num_planes; ++i) {
				f->fmt.pix_mp.plane_fmt[i].sizeimage =
					fmt->get_frame_size(i,
						f->fmt.pix_mp.height,
						f->fmt.pix_mp.width);
			}
		} else {
			f->fmt.pix_mp.plane_fmt[0].sizeimage =
			inst->buff_req.buffer[HAL_BUFFER_OUTPUT].buffer_size;
			extra_idx = EXTRADATA_IDX(fmt->num_planes);
			if (extra_idx && (extra_idx < VIDEO_MAX_PLANES)) {
				f->fmt.pix_mp.plane_fmt[1].sizeimage =
		inst->buff_req.buffer[HAL_BUFFER_EXTRADATA_OUTPUT].buffer_size;
			}
		}
		f->fmt.pix_mp.num_planes = fmt->num_planes;
		for (i = 0; i < fmt->num_planes; ++i) {
			inst->bufq[CAPTURE_PORT].vb2_bufq.plane_sizes[i] =
				f->fmt.pix_mp.plane_fmt[i].sizeimage;
		}
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		inst->prop.width = f->fmt.pix_mp.width;
		inst->prop.height = f->fmt.pix_mp.height;
		fmt = msm_comm_get_pixel_fmt_fourcc(vdec_formats,
				ARRAY_SIZE(vdec_formats),
				f->fmt.pix_mp.pixelformat,
				OUTPUT_PORT);
		if (!fmt || fmt->type != OUTPUT_PORT) {
			dprintk(VIDC_ERR,
			"Format: %d not supported on OUTPUT port\n",
			f->fmt.pix_mp.pixelformat);
			rc = -EINVAL;
			goto err_invalid_fmt;
		}
		inst->fmts[fmt->type] = fmt;
		rc = msm_comm_try_state(inst, MSM_VIDC_OPEN_DONE);
		if (rc) {
			dprintk(VIDC_ERR, "Failed to open instance\n");
			goto err_invalid_fmt;
		}
		frame_sz.buffer_type = HAL_BUFFER_INPUT;
		frame_sz.width = inst->prop.width;
		frame_sz.height = inst->prop.height;
		msm_comm_try_set_prop(inst, HAL_PARAM_FRAME_SIZE, &frame_sz);
		f->fmt.pix_mp.plane_fmt[0].sizeimage =
			fmt->get_frame_size(0, f->fmt.pix_mp.height,
					f->fmt.pix_mp.width);
		f->fmt.pix_mp.num_planes = fmt->num_planes;
		for (i = 0; i < fmt->num_planes; ++i) {
			inst->bufq[OUTPUT_PORT].vb2_bufq.plane_sizes[i] =
				f->fmt.pix_mp.plane_fmt[i].sizeimage;
		}
	}
err_invalid_fmt:
	return rc;
}

int msm_vdec_querycap(struct msm_vidc_inst *inst, struct v4l2_capability *cap)
{
	if (!inst || !cap) {
		dprintk(VIDC_ERR,
			"Invalid input, inst = %p, cap = %p\n", inst, cap);
		return -EINVAL;
	}
	strlcpy(cap->driver, MSM_VIDC_DRV_NAME, sizeof(cap->driver));
	strlcpy(cap->card, MSM_VDEC_DVC_NAME, sizeof(cap->card));
	cap->bus_info[0] = 0;
	cap->version = MSM_VIDC_VERSION;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE_MPLANE |
						V4L2_CAP_VIDEO_OUTPUT_MPLANE |
						V4L2_CAP_STREAMING;
	memset(cap->reserved, 0, sizeof(cap->reserved));
	return 0;
}

int msm_vdec_enum_fmt(struct msm_vidc_inst *inst, struct v4l2_fmtdesc *f)
{
	const struct msm_vidc_format *fmt = NULL;
	int rc = 0;
	if (!inst || !f) {
		dprintk(VIDC_ERR,
			"Invalid input, inst = %p, f = %p\n", inst, f);
		return -EINVAL;
	}
	if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		fmt = msm_comm_get_pixel_fmt_index(vdec_formats,
			ARRAY_SIZE(vdec_formats), f->index, CAPTURE_PORT);
	} else if (f->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		fmt = msm_comm_get_pixel_fmt_index(vdec_formats,
			ARRAY_SIZE(vdec_formats), f->index, OUTPUT_PORT);
		f->flags = V4L2_FMT_FLAG_COMPRESSED;
	}

	memset(f->reserved, 0 , sizeof(f->reserved));
	if (fmt) {
		strlcpy(f->description, fmt->description,
				sizeof(f->description));
		f->pixelformat = fmt->fourcc;
	} else {
		dprintk(VIDC_INFO, "No more formats found\n");
		rc = -EINVAL;
	}
	return rc;
}

static int msm_vdec_queue_setup(struct vb2_queue *q,
				const struct v4l2_format *fmt,
				unsigned int *num_buffers,
				unsigned int *num_planes, unsigned int sizes[],
				void *alloc_ctxs[])
{
	int i, rc = 0;
	struct msm_vidc_inst *inst;
	struct hal_buffer_requirements *bufreq;
	int extra_idx = 0;
	struct hfi_device *hdev;
	if (!q || !num_buffers || !num_planes
		|| !sizes || !q->drv_priv) {
		dprintk(VIDC_ERR, "Invalid input, q = %p, %p, %p\n",
			q, num_buffers, num_planes);
		return -EINVAL;
	}
	inst = q->drv_priv;

	if (!inst || !inst->core || !inst->core->device) {
		dprintk(VIDC_ERR, "%s invalid parameters", __func__);
		return -EINVAL;
	}

	hdev = inst->core->device;

	switch (q->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		*num_planes = inst->fmts[OUTPUT_PORT]->num_planes;
		if (*num_buffers < MIN_NUM_OUTPUT_BUFFERS ||
				*num_buffers > MAX_NUM_OUTPUT_BUFFERS)
			*num_buffers = MIN_NUM_OUTPUT_BUFFERS;
		for (i = 0; i < *num_planes; i++) {
			sizes[i] = inst->fmts[OUTPUT_PORT]->get_frame_size(
					i, inst->prop.height, inst->prop.width);
		}
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		dprintk(VIDC_DBG, "Getting bufreqs on capture plane\n");
		*num_planes = inst->fmts[CAPTURE_PORT]->num_planes;
		rc = msm_comm_try_state(inst, MSM_VIDC_OPEN_DONE);
		if (rc) {
			dprintk(VIDC_ERR, "Failed to open instance\n");
			break;
		}
		rc = msm_comm_try_get_bufreqs(inst);
		if (rc) {
			dprintk(VIDC_ERR,
				"Failed to get buffer requirements: %d\n", rc);
			break;
		}
		mutex_lock(&inst->lock);
		if (*num_buffers && *num_buffers >
			inst->buff_req.buffer[HAL_BUFFER_OUTPUT].
				buffer_count_actual) {
			struct hal_buffer_count_actual new_buf_count;
			enum hal_property property_id =
				HAL_PARAM_BUFFER_COUNT_ACTUAL;

			new_buf_count.buffer_type = HAL_BUFFER_OUTPUT;
			new_buf_count.buffer_count_actual = *num_buffers;
			rc = call_hfi_op(hdev, session_set_property,
				inst->session, property_id, &new_buf_count);

		}
		bufreq = &inst->buff_req.buffer[HAL_BUFFER_OUTPUT];
		if (bufreq->buffer_count_actual > *num_buffers)
			*num_buffers =  bufreq->buffer_count_actual;
		else
			bufreq->buffer_count_actual = *num_buffers ;
		mutex_unlock(&inst->lock);
		dprintk(VIDC_DBG, "count =  %d, size = %d, alignment = %d\n",
				inst->buff_req.buffer[1].buffer_count_actual,
				inst->buff_req.buffer[1].buffer_size,
				inst->buff_req.buffer[1].buffer_alignment);
		sizes[0] = inst->buff_req.buffer[HAL_BUFFER_OUTPUT].buffer_size;
		extra_idx =
			EXTRADATA_IDX(inst->fmts[CAPTURE_PORT]->num_planes);
		if (extra_idx && (extra_idx < VIDEO_MAX_PLANES)) {
			sizes[extra_idx] =
		inst->buff_req.buffer[HAL_BUFFER_EXTRADATA_OUTPUT].buffer_size;
		}
		break;
	default:
		dprintk(VIDC_ERR, "Invalid q type = %d\n", q->type);
		rc = -EINVAL;
		break;
	}
	return rc;
}

static inline int start_streaming(struct msm_vidc_inst *inst)
{
	int rc = 0;
	struct vb2_buf_entry *temp;
	struct list_head *ptr, *next;
	inst->in_reconfig = false;
	rc = msm_comm_set_scratch_buffers(inst);
	if (rc) {
		dprintk(VIDC_ERR,
			"Failed to set scratch buffers: %d\n", rc);
		goto fail_start;
	}
	rc = msm_comm_set_persist_buffers(inst);
	if (rc) {
		dprintk(VIDC_ERR,
			"Failed to set persist buffers: %d\n", rc);
		goto fail_start;
	}
	msm_comm_scale_clocks_and_bus(inst);
	rc = msm_comm_try_state(inst, MSM_VIDC_START_DONE);
	if (rc) {
		dprintk(VIDC_ERR,
			"Failed to move inst: %p to start done state\n", inst);
		goto fail_start;
	}

	mutex_lock(&inst->sync_lock);
	if (!list_empty(&inst->pendingq)) {
		list_for_each_safe(ptr, next, &inst->pendingq) {
			temp = list_entry(ptr, struct vb2_buf_entry, list);
			rc = msm_comm_qbuf(temp->vb);
			if (rc) {
				dprintk(VIDC_ERR,
					"Failed to qbuf to hardware\n");
				break;
			}
			list_del(&temp->list);
			kfree(temp);
		}
	}
	mutex_unlock(&inst->sync_lock);
	return rc;
fail_start:
	return rc;
}

static inline int stop_streaming(struct msm_vidc_inst *inst)
{
	int rc = 0;
	rc = msm_comm_try_state(inst, MSM_VIDC_RELEASE_RESOURCES_DONE);
	if (rc)
		dprintk(VIDC_ERR,
			"Failed to move inst: %p to start done state\n", inst);
	return rc;
}

static int msm_vdec_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct msm_vidc_inst *inst;
	int rc = 0;
	if (!q || !q->drv_priv) {
		dprintk(VIDC_ERR, "Invalid input, q = %p\n", q);
		return -EINVAL;
	}
	inst = q->drv_priv;
	dprintk(VIDC_DBG,
		"Streamon called on: %d capability\n", q->type);
	switch (q->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		if (inst->bufq[CAPTURE_PORT].vb2_bufq.streaming)
			rc = start_streaming(inst);
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		if (inst->bufq[OUTPUT_PORT].vb2_bufq.streaming)
			rc = start_streaming(inst);
		break;
	default:
		dprintk(VIDC_ERR, "Q-type is not supported: %d\n", q->type);
		rc = -EINVAL;
		break;
	}
	return rc;
}

static int msm_vdec_stop_streaming(struct vb2_queue *q)
{
	struct msm_vidc_inst *inst;
	int rc = 0;
	if (!q || !q->drv_priv) {
		dprintk(VIDC_ERR, "Invalid input, q = %p\n", q);
		return -EINVAL;
	}
	inst = q->drv_priv;
	dprintk(VIDC_DBG, "Streamoff called on: %d capability\n", q->type);
	switch (q->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		if (!inst->bufq[CAPTURE_PORT].vb2_bufq.streaming)
			rc = stop_streaming(inst);
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		if (!inst->bufq[OUTPUT_PORT].vb2_bufq.streaming)
			rc = stop_streaming(inst);
		break;
	default:
		dprintk(VIDC_ERR,
			"Q-type is not supported: %d\n", q->type);
		rc = -EINVAL;
		break;
	}
	msm_comm_scale_clocks_and_bus(inst);

	if (rc)
		dprintk(VIDC_ERR,
			"Failed to move inst: %p, cap = %d to state: %d\n",
			inst, q->type, MSM_VIDC_RELEASE_RESOURCES_DONE);
	return rc;
}

static void msm_vdec_buf_queue(struct vb2_buffer *vb)
{
	int rc;
	rc = msm_comm_qbuf(vb);
	if (rc)
		dprintk(VIDC_ERR, "Failed to queue buffer: %d\n", rc);
}

int msm_vdec_cmd(struct msm_vidc_inst *inst, struct v4l2_decoder_cmd *dec)
{
	int rc = 0;
	struct v4l2_event dqevent = {0};
	struct msm_vidc_core *core = inst->core;
	switch (dec->cmd) {
	case V4L2_DEC_QCOM_CMD_FLUSH:
		rc = msm_comm_flush(inst, dec->flags);
		break;
	case V4L2_DEC_CMD_STOP:
		rc = msm_comm_release_scratch_buffers(inst);
		if (rc)
			dprintk(VIDC_ERR,
				"Failed to release scratch buffers: %d\n", rc);
		rc = msm_comm_release_persist_buffers(inst);
		if (rc)
			pr_err("Failed to release persist buffers: %d\n", rc);
		if (inst->state == MSM_VIDC_CORE_INVALID ||
			core->state == VIDC_CORE_INVALID) {
			dprintk(VIDC_ERR,
				"Core %p in bad state, Sending CLOSE event\n",
					core);
			dqevent.type = V4L2_EVENT_MSM_VIDC_CLOSE_DONE;
			v4l2_event_queue_fh(&inst->event_handler, &dqevent);
			goto exit;
		}
		rc = msm_comm_try_state(inst, MSM_VIDC_CLOSE_DONE);
		break;
	default:
		dprintk(VIDC_ERR, "Unknown Decoder Command\n");
		rc = -ENOTSUPP;
		goto exit;
	}
	if (rc) {
		dprintk(VIDC_ERR, "Failed to exec decoder cmd %d\n", dec->cmd);
		goto exit;
	}
exit:
	return rc;
}


static const struct vb2_ops msm_vdec_vb2q_ops = {
	.queue_setup = msm_vdec_queue_setup,
	.start_streaming = msm_vdec_start_streaming,
	.buf_queue = msm_vdec_buf_queue,
	.stop_streaming = msm_vdec_stop_streaming,
};

const struct vb2_ops *msm_vdec_get_vb2q_ops(void)
{
	return &msm_vdec_vb2q_ops;
}

int msm_vdec_inst_init(struct msm_vidc_inst *inst)
{
	int rc = 0;
	if (!inst) {
		dprintk(VIDC_ERR, "Invalid input = %p\n", inst);
		return -EINVAL;
	}
	inst->fmts[OUTPUT_PORT] = &vdec_formats[1];
	inst->fmts[CAPTURE_PORT] = &vdec_formats[0];
	inst->prop.height = DEFAULT_HEIGHT;
	inst->prop.width = DEFAULT_WIDTH;
	inst->prop.fps = 30;
	inst->prop.prev_time_stamp = 0;
	return rc;
}

static int try_set_ctrl(struct msm_vidc_inst *inst, struct v4l2_ctrl *ctrl)
{
	int rc = 0;
	struct v4l2_control control = {0, 0};
	struct hal_nal_stream_format_supported stream_format;
	struct hal_enable_picture enable_picture;
	struct hal_enable hal_property;/*, prop;*/
	u32 control_idx = 0;
	enum hal_property property_id = 0;
	u32 property_val = 0;
	void *pdata;
	struct hfi_device *hdev;

	if (!inst || !inst->core || !inst->core->device) {
		dprintk(VIDC_ERR, "%s invalid parameters", __func__);
		return -EINVAL;
	}
	hdev = inst->core->device;

	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDC_VIDEO_STREAM_FORMAT:
		property_id =
		HAL_PARAM_NAL_STREAM_FORMAT_SELECT;
		stream_format.nal_stream_format_supported =
		(0x00000001 << ctrl->val);
		pdata = &stream_format;
		break;
	case V4L2_CID_MPEG_VIDC_VIDEO_OUTPUT_ORDER:
		property_id = HAL_PARAM_VDEC_OUTPUT_ORDER;
		property_val = ctrl->val;
		pdata = &property_val;
		break;
	case V4L2_CID_MPEG_VIDC_VIDEO_ENABLE_PICTURE_TYPE:
		property_id =
			HAL_PARAM_VDEC_PICTURE_TYPE_DECODE;
		enable_picture.picture_type = ctrl->val;
		pdata = &enable_picture;
		break;
	case V4L2_CID_MPEG_VIDC_VIDEO_KEEP_ASPECT_RATIO:
		property_id =
			HAL_PARAM_VDEC_OUTPUT2_KEEP_ASPECT_RATIO;
		hal_property.enable = ctrl->val;
		pdata = &hal_property;
		break;
	case V4L2_CID_MPEG_VIDC_VIDEO_POST_LOOP_DEBLOCKER_MODE:
		property_id =
			HAL_CONFIG_VDEC_POST_LOOP_DEBLOCKER;
		hal_property.enable = ctrl->val;
		pdata = &hal_property;
		break;
	case V4L2_CID_MPEG_VIDC_VIDEO_DIVX_FORMAT:
		property_id = HAL_PARAM_DIVX_FORMAT;
		property_val = ctrl->val;
		pdata = &property_val;
		break;
	case V4L2_CID_MPEG_VIDC_VIDEO_MB_ERROR_MAP_REPORTING:
		property_id =
			HAL_CONFIG_VDEC_MB_ERROR_MAP_REPORTING;
		hal_property.enable = ctrl->val;
		pdata = &hal_property;
		break;
	case V4L2_CID_MPEG_VIDC_VIDEO_CONTINUE_DATA_TRANSFER:
		property_id =
			HAL_PARAM_VDEC_CONTINUE_DATA_TRANSFER;
		hal_property.enable = ctrl->val;
		pdata = &hal_property;
		break;
	case V4L2_CID_MPEG_VIDC_VIDEO_SYNC_FRAME_DECODE:
		property_id =
			HAL_PARAM_VDEC_SYNC_FRAME_DECODE;
		hal_property.enable = ctrl->val;
		pdata = &hal_property;
		break;
	case V4L2_CID_MPEG_VIDC_VIDEO_SECURE:
		inst->mode = VIDC_SECURE;
		dprintk(VIDC_DBG, "Setting secure mode to :%d\n", inst->mode);
		break;
	case V4L2_CID_MPEG_VIDC_VIDEO_EXTRADATA:
	{
		struct hal_extradata_enable extra;
		property_id = HAL_PARAM_INDEX_EXTRADATA;
		extra.index = msm_comm_get_hal_extradata_index(ctrl->val);
		extra.enable = 1;
		pdata = &extra;
		break;
	}
	default:
		break;
	}

	if (property_id) {
		dprintk(VIDC_DBG,
			"Control: HAL property=%d,ctrl_id=%d,ctrl_value=%d\n",
			property_id,
			msm_vdec_ctrls[control_idx].id,
			control.value);
			rc = call_hfi_op(hdev, session_set_property, (void *)
				inst->session, property_id, pdata);
	}

	return rc;
}

static int msm_vdec_op_s_ctrl(struct v4l2_ctrl *ctrl)
{
	int rc = 0, c = 0;
	struct msm_vidc_inst *inst = container_of(ctrl->handler,
				struct msm_vidc_inst, ctrl_handler);
	rc = msm_comm_try_state(inst, MSM_VIDC_OPEN_DONE);
	if (rc) {
		dprintk(VIDC_ERR,
			"Failed to move inst: %p to start done state\n", inst);
		goto failed_open_done;
	}

	for (c = 0; c < ctrl->ncontrols; ++c) {
		if (ctrl->cluster[c]->is_new) {
			rc = try_set_ctrl(inst, ctrl->cluster[c]);
			if (rc) {
				dprintk(VIDC_ERR, "Failed setting %x",
						ctrl->cluster[c]->id);
				break;
			}
		}
	}

failed_open_done:
	if (rc)
		dprintk(VIDC_ERR, "Failed to set hal property for framesize\n");
	return rc;
}

static int msm_vdec_op_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	return 0;
}

static const struct v4l2_ctrl_ops msm_vdec_ctrl_ops = {

	.s_ctrl = msm_vdec_op_s_ctrl,
	.g_volatile_ctrl = msm_vdec_op_g_volatile_ctrl,
};

const struct v4l2_ctrl_ops *msm_vdec_get_ctrl_ops(void)
{
	return &msm_vdec_ctrl_ops;
}

int msm_vdec_s_ctrl(struct msm_vidc_inst *inst, struct v4l2_control *ctrl)
{
	return v4l2_s_ctrl(NULL, &inst->ctrl_handler, ctrl);
}
int msm_vdec_g_ctrl(struct msm_vidc_inst *inst, struct v4l2_control *ctrl)
{
	return v4l2_g_ctrl(&inst->ctrl_handler, ctrl);
}

static struct v4l2_ctrl **get_cluster(int type, int *size)
{
	int c = 0, sz = 0;
	struct v4l2_ctrl **cluster = kmalloc(sizeof(struct v4l2_ctrl *) *
			NUM_CTRLS, GFP_KERNEL);

	if (type <= 0 || !size || !cluster)
		return NULL;

	for (c = 0; c < NUM_CTRLS; c++) {
		if (msm_vdec_ctrls[c].cluster == type) {
			cluster[sz] = msm_vdec_ctrls[c].priv;
			++sz;
		}
	}

	*size = sz;
	return cluster;
}

int msm_vdec_ctrl_init(struct msm_vidc_inst *inst)
{
	int idx = 0;
	struct v4l2_ctrl_config ctrl_cfg;
	int ret_val = 0;

	ret_val = v4l2_ctrl_handler_init(&inst->ctrl_handler, NUM_CTRLS);

	if (ret_val) {
		dprintk(VIDC_ERR, "CTRL ERR: Control handler init failed, %d\n",
				inst->ctrl_handler.error);
		return ret_val;
	}

	for (; idx < NUM_CTRLS; idx++) {
		struct v4l2_ctrl *ctrl = NULL;
		if (IS_PRIV_CTRL(msm_vdec_ctrls[idx].id)) {
			/*add private control*/
			ctrl_cfg.def = msm_vdec_ctrls[idx].default_value;
			ctrl_cfg.flags = 0;
			ctrl_cfg.id = msm_vdec_ctrls[idx].id;
			/* ctrl_cfg.is_private =
			 * msm_vdec_ctrls[idx].is_private;
			 * ctrl_cfg.is_volatile =
			 * msm_vdec_ctrls[idx].is_volatile;*/
			ctrl_cfg.max = msm_vdec_ctrls[idx].maximum;
			ctrl_cfg.min = msm_vdec_ctrls[idx].minimum;
			ctrl_cfg.menu_skip_mask =
				msm_vdec_ctrls[idx].menu_skip_mask;
			ctrl_cfg.name = msm_vdec_ctrls[idx].name;
			ctrl_cfg.ops = &msm_vdec_ctrl_ops;
			ctrl_cfg.step = msm_vdec_ctrls[idx].step;
			ctrl_cfg.type = msm_vdec_ctrls[idx].type;
			ctrl_cfg.qmenu = msm_vdec_ctrls[idx].qmenu;

			ctrl = v4l2_ctrl_new_custom(&inst->ctrl_handler,
					&ctrl_cfg, NULL);
		} else {
			if (msm_vdec_ctrls[idx].type == V4L2_CTRL_TYPE_MENU) {
				ctrl = v4l2_ctrl_new_std_menu(
					&inst->ctrl_handler,
					&msm_vdec_ctrl_ops,
					msm_vdec_ctrls[idx].id,
					msm_vdec_ctrls[idx].maximum,
					msm_vdec_ctrls[idx].menu_skip_mask,
					msm_vdec_ctrls[idx].default_value);
			} else {
				ctrl = v4l2_ctrl_new_std(&inst->ctrl_handler,
					&msm_vdec_ctrl_ops,
					msm_vdec_ctrls[idx].id,
					msm_vdec_ctrls[idx].minimum,
					msm_vdec_ctrls[idx].maximum,
					msm_vdec_ctrls[idx].step,
					msm_vdec_ctrls[idx].default_value);
			}
		}


		msm_vdec_ctrls[idx].priv = ctrl;
	}
	ret_val = inst->ctrl_handler.error;
	if (ret_val)
		dprintk(VIDC_ERR,
			"Error adding ctrls to ctrl handle, %d\n",
			inst->ctrl_handler.error);

	/* Construct clusters */
	for (idx = 1; idx < MSM_VDEC_CTRL_CLUSTER_MAX; ++idx) {
		struct msm_vidc_ctrl_cluster *temp = NULL;
		struct v4l2_ctrl **cluster = NULL;
		int cluster_size = 0;

		cluster = get_cluster(idx, &cluster_size);
		if (!cluster || !cluster_size) {
			dprintk(VIDC_WARN, "Failed to setup cluster of type %d",
					idx);
			continue;
		}

		v4l2_ctrl_cluster(cluster_size, cluster);

		temp = kzalloc(sizeof(*temp), GFP_KERNEL);
		if (!temp) {
			ret_val = -ENOMEM;
			break;
		}

		temp->cluster = cluster;
		INIT_LIST_HEAD(&temp->list);
		list_add_tail(&temp->list, &inst->ctrl_clusters);
	}
	return ret_val;
}

int msm_vdec_ctrl_deinit(struct msm_vidc_inst *inst)
{
	struct msm_vidc_ctrl_cluster *curr, *next;
	list_for_each_entry_safe(curr, next, &inst->ctrl_clusters, list) {
		kfree(curr->cluster);
		kfree(curr);
	}

	return 0;
}
