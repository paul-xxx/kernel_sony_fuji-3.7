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
#ifndef MSM8X10_WCD_H
#define MSM8X10_WCD_H

#include <sound/soc.h>
#include <sound/jack.h>
#include "wcd9xxx-mbhc.h"
#include "wcd9xxx-resmgr.h"

#define MSM8X10_WCD_NUM_REGISTERS	0x600
#define MSM8X10_WCD_MAX_REGISTER	(MSM8X10_WCD_NUM_REGISTERS-1)
#define MSM8X10_WCD_CACHE_SIZE		MSM8X10_WCD_NUM_REGISTERS
#define MSM8X10_WCD_NUM_IRQ_REGS	3
#define MAX_REGULATOR				7
#define MSM8X10_WCD_REG_VAL(reg, val)		{reg, 0, val}

#define MSM8X10_WCD_IS_DINO_REG(reg) \
	(((reg >= 0x400) && (reg <= 0x5FF)) ? 1 : 0)
#define MSM8X10_WCD_IS_HELICON_REG(reg) \
	(((reg >= 0x000) && (reg <= 0x1FF)) ? 1 : 0)
extern const u8 msm8x10_wcd_reg_readable[MSM8X10_WCD_CACHE_SIZE];
extern const u8 msm8x10_wcd_reset_reg_defaults[MSM8X10_WCD_CACHE_SIZE];
struct msm8x10_wcd_codec_dai_data {
	u32 rate;
	u32 *ch_num;
	u32 ch_act;
	u32 ch_tot;
};

enum msm8x10_wcd_pid_current {
	MSM8X10_WCD_PID_MIC_2P5_UA,
	MSM8X10_WCD_PID_MIC_5_UA,
	MSM8X10_WCD_PID_MIC_10_UA,
	MSM8X10_WCD_PID_MIC_20_UA,
};

struct msm8x10_wcd_reg_mask_val {
	u16	reg;
	u8	mask;
	u8	val;
};

enum msm8x10_wcd_mbhc_analog_pwr_cfg {
	MSM8X10_WCD_ANALOG_PWR_COLLAPSED = 0,
	MSM8X10_WCD_ANALOG_PWR_ON,
	MSM8X10_WCD_NUM_ANALOG_PWR_CONFIGS,
};

/* Number of input and output Slimbus port */
enum {
	MSM8X10_WCD_RX1 = 0,
	MSM8X10_WCD_RX2,
	MSM8X10_WCD_RX3,
	MSM8X10_WCD_RX_MAX,
};

enum {
	MSM8X10_WCD_TX1 = 0,
	MSM8X10_WCD_TX2,
	MSM8X10_WCD_TX3,
	MSM8X10_WCD_TX4,
	MSM8X10_WCD_TX_MAX,
};


enum {
	/* INTR_REG 0 */
	MSM8X10_WCD_IRQ_RESERVED_0 = 0,
	MSM8X10_WCD_IRQ_MBHC_REMOVAL,
	MSM8X10_WCD_IRQ_MBHC_SHORT_TERM,
	MSM8X10_WCD_IRQ_MBHC_PRESS,
	MSM8X10_WCD_IRQ_MBHC_RELEASE,
	MSM8X10_WCD_IRQ_MBHC_POTENTIAL,
	MSM8X10_WCD_IRQ_MBHC_INSERTION,
	MSM8X10_WCD_IRQ_MBHC_HS_DET,
	/* INTR_REG 1 */
	MSM8X10_WCD_IRQ_PA_STARTUP,
	MSM8X10_WCD_IRQ_BG_PRECHARGE,
	MSM8X10_WCD_IRQ_RESERVED_1,
	MSM8X10_WCD_IRQ_EAR_PA_OCPL_FAULT,
	MSM8X10_WCD_IRQ_EAR_PA_STARTUP,
	MSM8X10_WCD_IRQ_SPKR_PA_OCPL_FAULT,
	MSM8X10_WCD_IRQ_SPKR_CLIP_FAULT,
	MSM8X10_WCD_IRQ_RESERVED_2,
	/* INTR_REG 2 */
	MSM8X10_WCD_IRQ_HPH_L_PA_STARTUP,
	MSM8X10_WCD_IRQ_HPH_R_PA_STARTUP,
	MSM8X10_WCD_IRQ_HPH_PA_OCPL_FAULT,
	MSM8X10_WCD_IRQ_HPH_PA_OCPR_FAULT,
	MSM8X10_WCD_IRQ_RESERVED_3,
	MSM8X10_WCD_IRQ_RESERVED_4,
	MSM8X10_WCD_IRQ_RESERVED_5,
	MSM8X10_WCD_IRQ_RESERVED_6,
	MSM8X10_WCD_NUM_IRQS,
};


/* Each micbias can be assigned to one of three cfilters
 * Vbatt_min >= .15V + ldoh_v
 * ldoh_v >= .15v + cfiltx_mv
 * If ldoh_v = 1.95 160 mv < cfiltx_mv < 1800 mv
 * If ldoh_v = 2.35 200 mv < cfiltx_mv < 2200 mv
 * If ldoh_v = 2.75 240 mv < cfiltx_mv < 2600 mv
 * If ldoh_v = 2.85 250 mv < cfiltx_mv < 2700 mv
 */

struct msm8x10_wcd_micbias_setting {
	u8 ldoh_v;
	u32 cfilt1_mv; /* in mv */
	/* Different WCD9xxx series codecs may not
	 * have 4 mic biases. If a codec has fewer
	 * mic biases, some of these properties will
	 * not be used.
	 */
	u8 bias1_cfilt_sel;
	u8 bias1_cap_mode;
};

struct msm8x10_wcd_ocp_setting {
	unsigned int	use_pdata:1; /* 0 - use sys default as recommended */
	unsigned int	num_attempts:4; /* up to 15 attempts */
	unsigned int	run_time:4; /* in duty cycle */
	unsigned int	wait_time:4; /* in duty cycle */
	unsigned int	hph_ocp_limit:3; /* Headphone OCP current limit */
};


struct msm8x10_wcd_regulator {
	const char *name;
	int min_uV;
	int max_uV;
	int optimum_uA;
	struct regulator *regulator;
};

struct msm8x10_wcd_pdata {
	int irq;
	int irq_base;
	int num_irqs;
	int reset_gpio;
	void *msm8x10_wcd_ahb_base_vaddr;
	struct msm8x10_wcd_micbias_setting micbias;
	struct msm8x10_wcd_ocp_setting ocp;
	struct msm8x10_wcd_regulator regulator[MAX_REGULATOR];
	u32 mclk_rate;
};


enum msm8x10_wcd_micbias_num {
	MSM8X10_WCD_MICBIAS1 = 0,
};

struct msm8x10_wcd_mbhc_config {
	struct snd_soc_jack *headset_jack;
	struct snd_soc_jack *button_jack;
	bool read_fw_bin;
	/* void* calibration contains:
	 *  struct msm8x10_wcd_mbhc_general_cfg generic;
	 *  struct msm8x10_wcd_mbhc_plug_detect_cfg plug_det;
	 *  struct msm8x10_wcd_mbhc_plug_type_cfg plug_type;
	 *  struct msm8x10_wcd_mbhc_btn_detect_cfg btn_det;
	 *  struct msm8x10_wcd_mbhc_imped_detect_cfg imped_det;
	 * Note: various size depends on btn_det->num_btn
	 */
	void *calibration;
	enum msm8x10_wcd_micbias_num micbias;
	int (*mclk_cb_fn) (struct snd_soc_codec*, int, bool);
	unsigned int mclk_rate;
	unsigned int gpio;
	unsigned int gpio_irq;
	int gpio_level_insert;
	bool detect_extn_cable;
	/* swap_gnd_mic returns true if extern GND/MIC swap switch toggled */
	bool (*swap_gnd_mic) (struct snd_soc_codec *);
};


enum msm8x10_wcd_pm_state {
	MSM8X10_WCD_PM_SLEEPABLE,
	MSM8X10_WCD_PM_AWAKE,
	MSM8X10_WCD_PM_ASLEEP,
};

struct msm8x10_wcd {
	struct device *dev;
	struct mutex io_lock;
	struct mutex xfer_lock;
	struct mutex irq_lock;
	u8 version;

	int reset_gpio;

	u32 num_of_supplies;
	struct regulator_bulk_data *supplies;

	enum msm8x10_wcd_pm_state pm_state;
	struct mutex pm_lock;
	/* pm_wq notifies change of pm_state */
	wait_queue_head_t pm_wq;
	struct pm_qos_request pm_qos_req;
	int wlock_holders;

	u8 idbyte[4];

	unsigned int irq_base;
	unsigned int irq;
	u8 irq_masks_cur[MSM8X10_WCD_NUM_IRQ_REGS];
	u8 irq_masks_cache[MSM8X10_WCD_NUM_IRQ_REGS];
	bool irq_level_high[MSM8X10_WCD_NUM_IRQS];
	int num_irqs;
	u32 mclk_rate;
};

extern int msm8x10_wcd_mclk_enable(struct snd_soc_codec *codec, int mclk_enable,
			     bool dapm);
extern int msm8x10_wcd_hs_detect(struct snd_soc_codec *codec,
			   struct msm8x10_wcd_mbhc_config *mbhc_cfg);

#endif
