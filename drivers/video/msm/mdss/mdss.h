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

#ifndef MDSS_H
#define MDSS_H

#include <linux/msm_ion.h>
#include <linux/earlysuspend.h>
#include <linux/msm_mdp.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include <mach/iommu_domains.h>

#define MDSS_REG_WRITE(addr, val) writel_relaxed(val, mdss_res->mdp_base + addr)
#define MDSS_REG_READ(addr) readl_relaxed(mdss_res->mdp_base + addr)

enum mdss_mdp_clk_type {
	MDSS_CLK_AHB,
	MDSS_CLK_AXI,
	MDSS_CLK_MDP_SRC,
	MDSS_CLK_MDP_CORE,
	MDSS_CLK_MDP_LUT,
	MDSS_CLK_MDP_VSYNC,
	MDSS_MAX_CLK
};

enum mdss_iommu_domain_type {
	MDSS_IOMMU_DOMAIN_SECURE,
	MDSS_IOMMU_DOMAIN_UNSECURE,
	MDSS_IOMMU_MAX_DOMAIN
};

struct mdss_iommu_map_type {
	char *client_name;
	char *ctx_name;
	struct device *ctx;
	struct msm_iova_partition partitions[1];
	int npartitions;
	int domain_idx;
};

struct mdss_hw_settings {
	char __iomem *reg;
	u32 val;
};

struct mdss_data_type {
	u32 rev;
	u32 mdp_rev;
	struct clk *mdp_clk[MDSS_MAX_CLK];
	struct regulator *fs;

	struct workqueue_struct *clk_ctrl_wq;
	struct delayed_work clk_ctrl_worker;
	struct platform_device *pdev;
	char __iomem *mdp_base;
	size_t mdp_reg_size;
	char __iomem *vbif_base;

	u32 irq;
	u32 irq_mask;
	u32 irq_ena;
	u32 irq_buzy;

	u32 mdp_irq_mask;
	u32 mdp_hist_irq_mask;

	u32 suspend;
	u32 timeout;

	u8 clk_ena;
	u8 fs_ena;
	u8 vsync_ena;

	u32 res_init;
	u32 bus_hdl;

	u32 smp_mb_cnt;
	u32 smp_mb_size;

	struct mdss_hw_settings *hw_settings;

	struct mdss_mdp_pipe *vig_pipes;
	struct mdss_mdp_pipe *rgb_pipes;
	struct mdss_mdp_pipe *dma_pipes;
	u32 nvig_pipes;
	u32 nrgb_pipes;
	u32 ndma_pipes;
	struct mdss_mdp_mixer *mixer_intf;
	struct mdss_mdp_mixer *mixer_wb;
	u32 nmixers_intf;
	u32 nmixers_wb;
	struct mdss_mdp_ctl *ctl_off;
	u32 nctl;
	struct mdss_mdp_dp_intf *dp_off;
	u32 ndp;
	void *video_intf;
	u32 nintf;

	struct ion_client *iclient;
	int iommu_attached;
	struct mdss_iommu_map_type *iommu_map;

	struct early_suspend early_suspend;
	void *debug_data;
};
extern struct mdss_data_type *mdss_res;

enum mdss_hw_index {
	MDSS_HW_MDP,
	MDSS_HW_DSI0,
	MDSS_HW_DSI1,
	MDSS_HW_HDMI,
	MDSS_HW_EDP,
	MDSS_MAX_HW_BLK
};

struct mdss_hw {
	u32 hw_ndx;
	void *ptr;
	irqreturn_t (*irq_handler)(int irq, void *ptr);
};

void mdss_enable_irq(struct mdss_hw *hw);
void mdss_disable_irq(struct mdss_hw *hw);
void mdss_disable_irq_nosync(struct mdss_hw *hw);

static inline struct ion_client *mdss_get_ionclient(void)
{
	if (!mdss_res)
		return NULL;
	return mdss_res->iclient;
}

static inline int is_mdss_iommu_attached(void)
{
	if (!mdss_res)
		return false;
	return mdss_res->iommu_attached;
}

static inline int mdss_get_iommu_domain(u32 type)
{
	if (type >= MDSS_IOMMU_MAX_DOMAIN)
		return -EINVAL;

	if (!mdss_res)
		return -ENODEV;

	return mdss_res->iommu_map[type].domain_idx;
}
#endif /* MDSS_H */
