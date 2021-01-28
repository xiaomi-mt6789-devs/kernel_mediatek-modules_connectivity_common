/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG "[WMT-CONSYS-HW]"

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <connectivity_build_in_adapter.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/memblock.h>
#include <linux/platform_device.h>
#include "connsys_debug_utility.h"
#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
#include "fw_log_wmt.h"
#endif
#include "osal_typedef.h"
#include "mt6779.h"
#include "mtk_wcn_consys_hw.h"
#include "wmt_ic.h"
#include "wmt_lib.h"
#include "stp_dbg.h"

#ifdef CONFIG_MTK_EMI
#include <mt_emi_api.h>
#endif

#if CONSYS_PMIC_CTRL_ENABLE
#include <upmu_common.h>
#include <linux/regulator/consumer.h>
#endif

#ifdef CONFIG_MTK_HIBERNATION
#include <mtk_hibernate_dpm.h>
#endif

#include <linux/of_reserved_mem.h>

#include <mtk_clkbuf_ctl.h>

/* Direct path */
#include <mtk_ccci_common.h>

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static INT32 consys_clock_buffer_ctrl(MTK_WCN_BOOL enable);
static VOID consys_hw_reset_bit_set(MTK_WCN_BOOL enable);
static VOID consys_hw_spm_clk_gating_enable(VOID);
static INT32 consys_hw_power_ctrl(MTK_WCN_BOOL enable);
static INT32 consys_ahb_clock_ctrl(MTK_WCN_BOOL enable);
static INT32 polling_consys_chipid(VOID);
static VOID consys_acr_reg_setting(VOID);
static VOID consys_afe_reg_setting(VOID);
static INT32 consys_hw_vcn18_ctrl(MTK_WCN_BOOL enable);
static VOID consys_vcn28_hw_mode_ctrl(UINT32 enable);
static INT32 consys_hw_vcn28_ctrl(UINT32 enable);
static INT32 consys_hw_wifi_vcn33_ctrl(UINT32 enable);
static INT32 consys_hw_bt_vcn33_ctrl(UINT32 enable);
static UINT32 consys_soc_chipid_get(VOID);
static INT32 consys_emi_mpu_set_region_protection(VOID);
static UINT32 consys_emi_set_remapping_reg(VOID);
static INT32 bt_wifi_share_v33_spin_lock_init(VOID);
static INT32 consys_clk_get_from_dts(struct platform_device *pdev);
static INT32 consys_pmic_get_from_dts(struct platform_device *pdev);
static INT32 consys_read_irq_info_from_dts(struct platform_device *pdev,
		PINT32 irq_num, PUINT32 irq_flag);
static INT32 consys_read_reg_from_dts(struct platform_device *pdev);
static UINT32 consys_read_cpupcr(VOID);
static VOID force_trigger_assert_debug_pin(VOID);
static INT32 consys_co_clock_type(VOID);
static P_CONSYS_EMI_ADDR_INFO consys_soc_get_emi_phy_add(VOID);
static VOID consys_set_if_pinmux(MTK_WCN_BOOL enable);
static INT32 consys_dl_rom_patch(UINT32 ip_ver, UINT32 fw_ver);
static VOID consys_set_dl_rom_patch_flag(INT32 flag);
static INT32 consys_dedicated_log_path_init(struct platform_device *pdev);
static VOID consys_dedicated_log_path_deinit(VOID);
static INT32 consys_emi_coredump_remapping(UINT8 __iomem **addr, UINT32 enable);
static INT32 consys_reset_emi_coredump(UINT8 __iomem *addr);
static INT32 consys_check_reg_readable(VOID);
static VOID consys_ic_clock_fail_dump(VOID);
static INT32 consys_is_connsys_reg(UINT32 addr);
static VOID consys_resume_dump_info(VOID);
/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
/* CCF part */
struct clk *clk_scp_conn_main;	/*ctrl conn_power_on/off */
struct clk *clk_infracfg_ao_ccif4_ap_cg;       /* For direct path */

/* PMIC part */
#if CONSYS_PMIC_CTRL_ENABLE
struct regulator *reg_VCN18;
#if CONSYS_PMIC_CTRL_6635
struct regulator *reg_VCN13;
struct regulator *reg_VCN33_1_BT;
struct regulator *reg_VCN33_1_WIFI;
struct regulator *reg_VCN33_2_WIFI;
#else
struct regulator *reg_VCN28;
struct regulator *reg_VCN33_BT;
struct regulator *reg_VCN33_WIFI;
#endif
#endif

EMI_CTRL_STATE_OFFSET mtk_wcn_emi_state_off = {
	.emi_apmem_ctrl_state = EXP_APMEM_CTRL_STATE,
	.emi_apmem_ctrl_host_sync_state = EXP_APMEM_CTRL_HOST_SYNC_STATE,
	.emi_apmem_ctrl_host_sync_num = EXP_APMEM_CTRL_HOST_SYNC_NUM,
	.emi_apmem_ctrl_chip_sync_state = EXP_APMEM_CTRL_CHIP_SYNC_STATE,
	.emi_apmem_ctrl_chip_sync_num = EXP_APMEM_CTRL_CHIP_SYNC_NUM,
	.emi_apmem_ctrl_chip_sync_addr = EXP_APMEM_CTRL_CHIP_SYNC_ADDR,
	.emi_apmem_ctrl_chip_sync_len = EXP_APMEM_CTRL_CHIP_SYNC_LEN,
	.emi_apmem_ctrl_chip_print_buff_start = EXP_APMEM_CTRL_CHIP_PRINT_BUFF_START,
	.emi_apmem_ctrl_chip_print_buff_len = EXP_APMEM_CTRL_CHIP_PRINT_BUFF_LEN,
	.emi_apmem_ctrl_chip_print_buff_idx = EXP_APMEM_CTRL_CHIP_PRINT_BUFF_IDX,
	.emi_apmem_ctrl_chip_int_status = EXP_APMEM_CTRL_CHIP_INT_STATUS,
	.emi_apmem_ctrl_chip_paded_dump_end = EXP_APMEM_CTRL_CHIP_PAGED_DUMP_END,
	.emi_apmem_ctrl_host_outband_assert_w1 = EXP_APMEM_CTRL_HOST_OUTBAND_ASSERT_W1,
	.emi_apmem_ctrl_chip_page_dump_num = EXP_APMEM_CTRL_CHIP_PAGE_DUMP_NUM,
	.emi_apmem_ctrl_assert_flag = EXP_APMEM_CTRL_ASSERT_FLAG,
};

CONSYS_EMI_ADDR_INFO mtk_wcn_emi_addr_info = {
	.emi_phy_addr = CONSYS_EMI_FW_PHY_BASE,
	.paged_trace_off = CONSYS_EMI_PAGED_TRACE_OFFSET,
	.paged_dump_off = CONSYS_EMI_PAGED_DUMP_OFFSET,
	.full_dump_off = CONSYS_EMI_FULL_DUMP_OFFSET,
	.emi_remap_offset = CONSYS_EMI_MAPPING_OFFSET,
	.p_ecso = &mtk_wcn_emi_state_off,
	.pda_dl_patch_flag = 0,
	.emi_met_size = 0x50000,
	.emi_met_data_offset = CONSYS_EMI_MET_DATA_OFFSET,
	.emi_core_dump_offset = CONSYS_EMI_COREDUMP_OFFSET,
};

WMT_CONSYS_IC_OPS consys_ic_ops = {
	.consys_ic_clock_buffer_ctrl = consys_clock_buffer_ctrl,
	.consys_ic_hw_reset_bit_set = consys_hw_reset_bit_set,
	.consys_ic_hw_spm_clk_gating_enable = consys_hw_spm_clk_gating_enable,
	.consys_ic_hw_power_ctrl = consys_hw_power_ctrl,
	.consys_ic_ahb_clock_ctrl = consys_ahb_clock_ctrl,
	.polling_consys_ic_chipid = polling_consys_chipid,
	.consys_ic_acr_reg_setting = consys_acr_reg_setting,
	.consys_ic_afe_reg_setting = consys_afe_reg_setting,
	.consys_ic_hw_vcn18_ctrl = consys_hw_vcn18_ctrl,
	.consys_ic_vcn28_hw_mode_ctrl = consys_vcn28_hw_mode_ctrl,
	.consys_ic_hw_vcn28_ctrl = consys_hw_vcn28_ctrl,
	.consys_ic_hw_wifi_vcn33_ctrl = consys_hw_wifi_vcn33_ctrl,
	.consys_ic_hw_bt_vcn33_ctrl = consys_hw_bt_vcn33_ctrl,
	.consys_ic_soc_chipid_get = consys_soc_chipid_get,
	.consys_ic_emi_mpu_set_region_protection = consys_emi_mpu_set_region_protection,
	.consys_ic_emi_set_remapping_reg = consys_emi_set_remapping_reg,
	.ic_bt_wifi_share_v33_spin_lock_init = bt_wifi_share_v33_spin_lock_init,
	.consys_ic_clk_get_from_dts = consys_clk_get_from_dts,
	.consys_ic_pmic_get_from_dts = consys_pmic_get_from_dts,
	.consys_ic_read_irq_info_from_dts = consys_read_irq_info_from_dts,
	.consys_ic_read_reg_from_dts = consys_read_reg_from_dts,
	.consys_ic_read_cpupcr = consys_read_cpupcr,
	.ic_force_trigger_assert_debug_pin = force_trigger_assert_debug_pin,
	.consys_ic_co_clock_type = consys_co_clock_type,
	.consys_ic_soc_get_emi_phy_add = consys_soc_get_emi_phy_add,
	.consys_ic_set_if_pinmux = consys_set_if_pinmux,
	.consys_ic_set_dl_rom_patch_flag = consys_set_dl_rom_patch_flag,
	.consys_ic_dedicated_log_path_init = consys_dedicated_log_path_init,
	.consys_ic_dedicated_log_path_deinit = consys_dedicated_log_path_deinit,
	.consys_ic_emi_coredump_remapping = consys_emi_coredump_remapping,
	.consys_ic_reset_emi_coredump = consys_reset_emi_coredump,
	.consys_ic_check_reg_readable = consys_check_reg_readable,
	.consys_ic_clock_fail_dump = consys_ic_clock_fail_dump,
	.consys_ic_is_connsys_reg = consys_is_connsys_reg,
	.consys_ic_resume_dump_info = consys_resume_dump_info,
};

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
INT32 rom_patch_dl_flag = 1;
UINT32 gJtagCtrl;

#if CONSYS_ENALBE_SET_JTAG
#define JTAG_ADDR1_BASE 0x10005000
#define JTAG_ADDR2_BASE 0x11E80000
#define JTAG_ADDR3_BASE 0x11D20000
#define AP2CONN_JTAG_2WIRE_OFFSET 0xF00
#endif

INT32 mtk_wcn_consys_jtag_set_for_mcu(VOID)
{
#if 0
#if CONSYS_ENALBE_SET_JTAG
	INT32 ret = 0;
	UINT32 tmp = 0;
	PVOID addr = 0;
	PVOID remap_addr1 = 0;
	PVOID remap_addr2 = 0;
	PVOID remap_addr3 = 0;

	if (gJtagCtrl) {

		remap_addr1 = ioremap(JTAG_ADDR1_BASE, 0x1000);
		if (remap_addr1 == 0) {
			WMT_PLAT_PR_ERR("remap jtag_addr1 fail!\n");
			ret = -1;
			goto error;
		}

		remap_addr2 = ioremap(JTAG_ADDR2_BASE, 0x100);
		if (remap_addr2 == 0) {
			WMT_PLAT_PR_ERR("remap jtag_addr2 fail!\n");
			ret = -1;
			goto error;
		}

		remap_addr3 = ioremap(JTAG_ADDR3_BASE, 0x100);
		if (remap_addr3 == 0) {
			WMT_PLAT_PR_ERR("remap jtag_addr3 fail!\n");
			ret =  -1;
			goto error;
		}

		WMT_PLAT_PR_INFO("WCN jtag set for mcu start...\n");
		switch (gJtagCtrl) {
		case 1:
			/* 7-wire jtag pinmux setting*/
#if 1
			/* PAD AUX Function Selection */
			addr = remap_addr1 + 0x320;
			tmp = readl(addr);
			tmp = tmp & 0x0000000f;
			tmp = tmp | 0x33333330;
			writel(tmp, addr);
			tmp = readl(addr);
			WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%p, 0x%x)\n", addr, tmp);

			/* PAD Driving Selection */
			addr = remap_addr2 + 0xA0;
			tmp = readl(addr);
			tmp = tmp & 0xfff00fff;
			tmp = tmp | 0x00077000;
			writel(tmp, addr);
			tmp = readl(addr);
			WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%p, 0x%x)\n", addr, tmp);

			/* PAD PULL Selection */
			addr = remap_addr2 + 0x60;
			tmp = readl(addr);
			tmp = tmp & 0xfffe03ff;
			writel(tmp, addr);
			tmp = readl(addr);
			WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%p, 0x%x)\n", addr, tmp);

			addr = remap_addr2 + 0x60;
			tmp = readl(addr);
			tmp = tmp & 0xfff80fff;
			writel(tmp, addr);
			tmp = readl(addr);
			WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%p, 0x%x)\n", addr, tmp);

#else
			/* backup */
			/* PAD AUX Function Selection */
			addr = remap_addr1 + 0x310;
			tmp = readl(addr);
			tmp = tmp & 0xfffff000;
			tmp = tmp | 0x00000444;
			writel(tmp, addr);
			tmp = readl(addr);
			WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%p, 0x%x)\n", addr, tmp);

			addr = remap_addr1 + 0x3C0;
			tmp = readl(addr);
			tmp = tmp & 0xf00ff00f;
			tmp = tmp | 0x04400440;
			writel(tmp, addr);
			tmp = readl(addr);
			WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%p, 0x%x)\n", addr, tmp);

			/* PAD Driving Selection */
			addr = remap_addr3 + 0xA0;
			tmp = readl(addr);
			tmp = tmp & 0x0ffffff0;
			tmp = tmp | 0x70000007;
			writel(tmp, addr);
			tmp = readl(addr);
			WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%p, 0x%x)\n", addr, tmp);

			addr = remap_addr3 + 0xB0;
			tmp = readl(addr);
			tmp = tmp & 0xfff0f0ff;
			tmp = tmp | 0x0007070f;
			writel(tmp, addr);
			tmp = readl(addr);
			WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%p, 0x%x)\n", addr, tmp);

			/* PAD PULL Selection */
			addr = remap_addr3 + 0x60;
			tmp = readl(addr);
			tmp = tmp & 0xf333fffe;
			writel(tmp, addr);
			tmp = readl(addr);
			WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%p, 0x%x)\n", addr, tmp);
#endif
			break;
		case 2:
			/* 2-wire jtag pinmux setting*/
#if 0
			CONSYS_SET_BIT(conn_reg.topckgen_base + AP2CONN_JTAG_2WIRE_OFFSET, 1 << 8);
			addr = remap_addr1 + 0x340;
			tmp = readl(addr);
			tmp = tmp & 0xfff88fff;
			tmp = tmp | 0x00034000;
			writel(tmp, addr);
			tmp = readl(addr);
			WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%p, 0x%x)\n", addr, tmp);
#endif
			break;
		default:
			WMT_PLAT_PR_INFO("unsupported options!\n");
		}

	}
#endif

error:

	if (remap_addr1)
		iounmap(remap_addr1);
	if (remap_addr2)
		iounmap(remap_addr2);
	if (remap_addr3)
		iounmap(remap_addr3);

	return ret;
#else
	WMT_PLAT_PR_INFO("No support switch to JTAG!\n");

	return 0;
#endif
}

UINT32 mtk_wcn_consys_jtag_flag_ctrl(UINT32 en)
{
	WMT_PLAT_PR_INFO("%s jtag set for MCU\n", en ? "enable" : "disable");
	gJtagCtrl = en;

	return 0;
}

static INT32 consys_clk_get_from_dts(struct platform_device *pdev)
{
	clk_scp_conn_main = devm_clk_get(&pdev->dev, "conn");
	if (IS_ERR(clk_scp_conn_main)) {
		WMT_PLAT_PR_ERR("[CCF]cannot get clk_scp_conn_main clock.\n");
		return PTR_ERR(clk_scp_conn_main);
	}
	WMT_PLAT_PR_DBG("[CCF]clk_scp_conn_main=%p\n", clk_scp_conn_main);

	clk_infracfg_ao_ccif4_ap_cg = devm_clk_get(&pdev->dev, "ccif");
	if (IS_ERR(clk_infracfg_ao_ccif4_ap_cg)) {
		WMT_PLAT_PR_ERR("[CCF]cannot get clk_infracfg_ao_ccif4_ap_cg clock.\n");
		return PTR_ERR(clk_infracfg_ao_ccif4_ap_cg);
	}
	WMT_PLAT_PR_DBG("[CCF]clk_infracfg_ao_ccif4_ap_cg=%p\n", clk_infracfg_ao_ccif4_ap_cg);

	return 0;
}

static INT32 consys_pmic_get_from_dts(struct platform_device *pdev)
{
#if CONSYS_PMIC_CTRL_ENABLE
	reg_VCN18 = regulator_get(&pdev->dev, "vcn18");
	if (!reg_VCN18)
		WMT_PLAT_PR_ERR("Regulator_get VCN_1V8 fail\n");
#if CONSYS_PMIC_CTRL_6635
	reg_VCN13 = regulator_get(&pdev->dev, "vcn13");
	if (!reg_VCN13)
		WMT_PLAT_PR_ERR("Regulator_get VCN_1V3 fail\n");
	reg_VCN33_1_BT = regulator_get(&pdev->dev, "vcn33_1_bt");
	if (!reg_VCN33_1_BT)
		WMT_PLAT_PR_ERR("Regulator_get VCN33_1_BT fail\n");
	reg_VCN33_1_WIFI = regulator_get(&pdev->dev, "vcn33_1_wifi");
	if (!reg_VCN33_1_WIFI)
		WMT_PLAT_PR_ERR("Regulator_get VCN33_1_WIFI fail\n");
	reg_VCN33_2_WIFI = regulator_get(&pdev->dev, "vcn33_2_wifi");
	if (!reg_VCN33_2_WIFI)
		WMT_PLAT_PR_ERR("Regulator_get VCN33_2_WIFI fail\n");
#else
	reg_VCN28 = regulator_get(&pdev->dev, "vcn28");
	if (!reg_VCN28)
		WMT_PLAT_PR_ERR("Regulator_get VCN_2V8 fail\n");
	reg_VCN33_BT = regulator_get(&pdev->dev, "vcn33_bt");
	if (!reg_VCN33_BT)
		WMT_PLAT_PR_ERR("Regulator_get VCN33_BT fail\n");
	reg_VCN33_WIFI = regulator_get(&pdev->dev, "vcn33_wifi");
	if (!reg_VCN33_WIFI)
		WMT_PLAT_PR_ERR("Regulator_get VCN33_WIFI fail\n");
#endif
#endif

	return 0;
}

static INT32 consys_co_clock_type(VOID)
{
	return 0;
}

static INT32 consys_clock_buffer_ctrl(MTK_WCN_BOOL enable)
{
	if (enable)
		KERNEL_clk_buf_ctrl(CLK_BUF_CONN, true);	/*open XO_WCN*/
	else
		KERNEL_clk_buf_ctrl(CLK_BUF_CONN, false);	/*close XO_WCN*/

	return 0;
}

static VOID consys_set_if_pinmux(MTK_WCN_BOOL enable)
{
	UINT8 *consys_if_pinmux_reg_base = NULL;
	UINT8 *consys_if_pinmux_driving_base = NULL;

	/* Switch D die pinmux for connecting A die */
	consys_if_pinmux_reg_base = ioremap_nocache(CONSYS_IF_PINMUX_REG_BASE, 0x1000);
	if (!consys_if_pinmux_reg_base) {
		WMT_PLAT_PR_ERR("consys_if_pinmux_reg_base(%x) ioremap fail\n",
				CONSYS_IF_PINMUX_REG_BASE);
		return;
	}

	consys_if_pinmux_driving_base = ioremap_nocache(CONSYS_IF_PINMUX_DRIVING_BASE, 0x100);
	if (!consys_if_pinmux_driving_base) {
		WMT_PLAT_PR_ERR("consys_if_pinmux_driving_base(%x) ioremap fail\n",
				CONSYS_IF_PINMUX_DRIVING_BASE);
		if (consys_if_pinmux_reg_base)
			iounmap(consys_if_pinmux_reg_base);
		return;
	}

	if (enable) {
		CONSYS_REG_WRITE(consys_if_pinmux_reg_base + CONSYS_IF_PINMUX_01_OFFSET,
				(CONSYS_REG_READ(consys_if_pinmux_reg_base +
				CONSYS_IF_PINMUX_01_OFFSET) &
				CONSYS_IF_PINMUX_01_MASK) | CONSYS_IF_PINMUX_01_VALUE);
		CONSYS_REG_WRITE(consys_if_pinmux_reg_base + CONSYS_IF_PINMUX_02_OFFSET,
				(CONSYS_REG_READ(consys_if_pinmux_reg_base +
				CONSYS_IF_PINMUX_02_OFFSET) &
				CONSYS_IF_PINMUX_02_MASK) | CONSYS_IF_PINMUX_02_VALUE);
		/* set pinmux driving to 2mA */
		CONSYS_REG_WRITE(consys_if_pinmux_driving_base + CONSYS_IF_PINMUX_DRIVING_OFFSET,
				(CONSYS_REG_READ(consys_if_pinmux_driving_base +
				CONSYS_IF_PINMUX_DRIVING_OFFSET) &
				CONSYS_IF_PINMUX_DRIVING_MASK) | CONSYS_IF_PINMUX_DRIVING_VALUE);
	} else {
		CONSYS_REG_WRITE(consys_if_pinmux_reg_base + CONSYS_IF_PINMUX_01_OFFSET,
				CONSYS_REG_READ(consys_if_pinmux_reg_base +
				CONSYS_IF_PINMUX_01_OFFSET) & CONSYS_IF_PINMUX_01_MASK);
		CONSYS_REG_WRITE(consys_if_pinmux_reg_base + CONSYS_IF_PINMUX_02_OFFSET,
				CONSYS_REG_READ(consys_if_pinmux_reg_base +
				CONSYS_IF_PINMUX_02_OFFSET) & CONSYS_IF_PINMUX_02_MASK);
	}

	if (consys_if_pinmux_reg_base)
		iounmap(consys_if_pinmux_reg_base);
	if (consys_if_pinmux_driving_base)
		iounmap(consys_if_pinmux_driving_base);
}

static VOID consys_hw_reset_bit_set(MTK_WCN_BOOL enable)
{
	UINT32 consys_ver_id = 0;
	UINT32 cnt = 0;
#if CONSYS_PMIC_CTRL_6635
	UINT8 *consys_reg_base = NULL;
#endif

	if (enable) {
		/*3.assert CONNSYS CPU SW reset  0x10007018 "[12]=1'b1  [31:24]=8'h88 (key)" */
		CONSYS_REG_WRITE((conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET),
				CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET) |
				CONSYS_CPU_SW_RST_BIT | CONSYS_CPU_SW_RST_CTRL_KEY);
	} else {
		/*16.deassert CONNSYS CPU SW reset 0x10007018 "[12]=1'b0 [31:24] =8'h88 (key)" */
		CONSYS_REG_WRITE(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET,
				(CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET) &
				 ~CONSYS_CPU_SW_RST_BIT) | CONSYS_CPU_SW_RST_CTRL_KEY);
		/* check CONNSYS power-on completion
		 * (polling "0x8000_0600[31:0]" == 0x1D1E and each polling interval is "1ms")
		 * (apply this for guarantee that CONNSYS CPU goes to "cos_idle_loop")
		 */
		consys_ver_id = CONSYS_REG_READ(conn_reg.mcu_base + 0x600);
		while (consys_ver_id != 0x1D1E) {
			if (cnt > 10)
				break;
			consys_ver_id = CONSYS_REG_READ(conn_reg.mcu_base + 0x600);
			WMT_PLAT_PR_INFO("0x18002600(0x%x)\n", consys_ver_id);
			WMT_PLAT_PR_INFO("0x1800216c(0x%x)\n",
					CONSYS_REG_READ(conn_reg.mcu_base + 0x16c));
			WMT_PLAT_PR_INFO("0x18007104(0x%x)\n",
					CONSYS_REG_READ(conn_reg.mcu_conn_hif_on_base +
					CONSYS_CPUPCR_OFFSET));
			msleep(20);
			cnt++;
		}

#if CONSYS_PMIC_CTRL_6635
		/* if(MT6635) CONN_WF_CTRL2 swtich to CONN mode */
		consys_reg_base = ioremap_nocache(CONSYS_IF_PINMUX_REG_BASE, 0x1000);
		if (!consys_reg_base) {
			WMT_PLAT_PR_ERR("consys_if_pinmux_reg_base(%x) ioremap fail\n",
					CONSYS_IF_PINMUX_REG_BASE);
			return;
		}
		CONSYS_REG_WRITE(consys_reg_base + CONSYS_WF_CTRL2_03_OFFSET,
				(CONSYS_REG_READ(consys_reg_base + CONSYS_WF_CTRL2_03_OFFSET) &
				CONSYS_WF_CTRL2_03_MASK) | CONSYS_WF_CTRL2_CONN_MODE);
		if (consys_reg_base)
			iounmap(consys_reg_base);
		else
			WMT_PLAT_PR_ERR("consys_if_pinmux_reg_base(0x%x) ioremap fail!\n",
					  CONSYS_IF_PINMUX_REG_BASE);
#endif
	}
}

static VOID consys_hw_spm_clk_gating_enable(VOID)
{
}

static INT32 consys_hw_power_ctrl(MTK_WCN_BOOL enable)
{
#if CONSYS_PWR_ON_OFF_API_AVAILABLE
	INT32 iRet = 0;
#else
	INT32 value = 0;
	INT32 i = 0;
#endif

	if (enable) {
#if CONSYS_PWR_ON_OFF_API_AVAILABLE
		iRet = clk_prepare_enable(clk_scp_conn_main);
		if (iRet) {
			WMT_PLAT_PR_ERR("clk_prepare_enable(clk_scp_conn_main) fail(%d)\n", iRet);
			return iRet;
		}
		WMT_PLAT_PR_DBG("clk_prepare_enable(clk_scp_conn_main) ok\n");

		iRet = clk_prepare_enable(clk_infracfg_ao_ccif4_ap_cg);
		if (iRet) {
			WMT_PLAT_PR_ERR("clk_prepare_enable(clk_infracfg_ao_ccif4_ap_cg) fail(%d)\n", iRet);
			return iRet;
		}
		WMT_PLAT_PR_DBG("clk_prepare_enable(clk_infracfg_ao_ccif4_ap_cg) ok\n");
#else
		/* turn on SPM clock gating enable PWRON_CONFG_EN 0x10006000 32'h0b160001 */
		CONSYS_REG_WRITE((conn_reg.spm_base + CONSYS_PWRON_CONFG_EN_OFFSET),
				 (CONSYS_REG_READ(conn_reg.spm_base +
				 CONSYS_PWRON_CONFG_EN_OFFSET) &
				 0x0000FFFF) | CONSYS_PWRON_CONFG_EN_VALUE);

		/*write assert "conn_top_on" primary part power on,
		 *set "connsys_on_domain_pwr_on"=1
		 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_ON_BIT);
		/*read check "conn_top_on" primary part power status,
		 *check "connsys_on_domain_pwr_ack"=1
		 */
		value = CONSYS_PWR_ON_ACK_BIT &
			CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_OFFSET);
		while (value == 0)
			value = CONSYS_PWR_ON_ACK_BIT &
				CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_OFFSET);
		/*write assert "conn_top_on" secondary part power on,
		 *set "connsys_on_domain_pwr_on_s"=1
		 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_ON_S_BIT);
		/*read check "conn_top_on" secondary part power status,
		 *check "connsys_on_domain_pwr_ack_s"=1
		 */
		value = CONSYS_PWR_ON_ACK_S_BIT &
			CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_S_OFFSET);
		while (value == 0)
			value = CONSYS_PWR_ON_ACK_S_BIT &
				CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_S_OFFSET);
		/*write turn on AP-to-CONNSYS bus clock, set "conn_clk_dis"=0 (apply this for bus
		 *clock toggling)
		 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~CONSYS_CLK_CTRL_BIT);
		/*wait 1us*/
		udelay(1);
		/*de-assert "conn_top_on" isolation, set "connsys_iso_en"=0 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~CONSYS_SPM_PWR_ISO_S_BIT);

		/*de-assert CONNSYS S/W reset (TOP RGU CR), set "ap_sw_rst_b"=1 */
		CONSYS_REG_WRITE(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET,
				(CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET) &
				 ~CONSYS_SW_RST_BIT) | CONSYS_CPU_SW_RST_CTRL_KEY);

		/*de-assert CONNSYS S/W reset (SPM CR), set "ap_sw_rst_b"=1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 CONSYS_SPM_PWR_RST_BIT);
#if 0
		/*read check "conn_top_off" primary part power status, check
		 *"connsys_off_domain_pwr_ack"=1
		 */
		value = CONSYS_TOP2_PWR_ON_ACK_BIT &
			CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_OFFSET);
		while (value == 0)
			value = CONSYS_TOP2_PWR_ON_ACK_BIT &
				CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_OFFSET);
		/*read check "conn_top_off" secondary part power status, check
		 *"connsys_off_domain_pwr_ack_s"=1
		 */
		value = CONSYS_TOP2_PWR_ON_ACK_S_BIT &
			CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_S_OFFSET);
		while (value == 0)
			value = CONSYS_TOP2_PWR_ON_ACK_S_BIT &
				CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_S_OFFSET);
#endif
		/*wait 0.5ms*/
		udelay(500);
		/*Turn off AHB RX bus sleep protect (AP2CONN AHB Bus protect)
		 *(apply this for INFRA AHB bus accessing when CONNSYS had been turned on)
		 */
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AHB_RX_PROT_EN_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base +
				 CONSYS_AHB_RX_PROT_EN_OFFSET) & ~CONSYS_AHB_RX_PROT_MASK);
		value = ~CONSYS_AHB_RX_PROT_MASK &
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AHB_RX_PROT_STA_OFFSET);
		i = 0;
		while (value == 0 || i > 10) {
			value = ~CONSYS_AP2CONN_PROT_MASK &
				CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AHB_RX_PROT_STA_OFFSET);
			i++;
		}

		/*Turn off AXI Rx bus sleep protect (CONN2AP AXI Rx Bus protect)
		 *(disable sleep protection when CONNSYS had been turned on)
		 */
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AXI_RX_PROT_EN_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base +
				 CONSYS_AXI_RX_PROT_EN_OFFSET) & ~CONSYS_AXI_RX_PROT_MASK);
		value = ~CONSYS_AXI_RX_PROT_MASK &
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AXI_RX_PROT_STA_OFFSET);
		i = 0;
		while (value == 0 || i > 10) {
			value = ~CONSYS_AXI_RX_PROT_MASK &
				CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AXI_RX_PROT_STA_OFFSET);
			i++;
		}

		/*Turn off AXI TX bus sleep protect (CONN2AP AXI Tx Bus protect)
		 *(disable sleep protection when CONNSYS had been turned on)
		 */
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AXI_RX_PROT_EN_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base +
				 CONSYS_AXI_RX_PROT_EN_OFFSET) & ~CONSYS_AXI_TX_PROT_MASK);
		value = ~CONSYS_AXI_TX_PROT_MASK &
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AXI_RX_PROT_STA_OFFSET);
		i = 0;
		while (value == 0 || i > 10) {
			value = ~CONSYS_AXI_TX_PROT_MASK &
				CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AXI_RX_PROT_STA_OFFSET);
			i++;
		}

		/*Turn off AHB TX bus sleep protect (AP2CONN AHB Bus protect)
		 *(apply this for INFRA AHB bus accessing when CONNSYS had been turned on)
		 */
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AXI_RX_PROT_EN_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base +
				 CONSYS_AXI_RX_PROT_EN_OFFSET) & ~CONSYS_AHB_TX_PROT_MASK);
		value = ~CONSYS_AHB_TX_PROT_MASK &
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AXI_RX_PROT_STA_OFFSET);
		i = 0;
		while (value == 0 || i > 10) {
			value = ~CONSYS_AHB_TX_PROT_MASK &
				CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AXI_RX_PROT_STA_OFFSET);
			i++;
		}
#if 0
		/*SPM apsrc/ddr_en hardware mask disable & wait cycle setting*/
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_SPM_APSRC_OFFSET,
				CONSYS_SPM_APSRC_VALUE);
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_SPM_DDR_EN_OFFSET,
				CONSYS_SPM_DDR_EN_VALUE);
#endif
		/*wait 5ms*/
		mdelay(5);
#endif /* CONSYS_PWR_ON_OFF_API_AVAILABLE */
	} else {
#if CONSYS_PWR_ON_OFF_API_AVAILABLE
		clk_disable_unprepare(clk_infracfg_ao_ccif4_ap_cg);
		WMT_PLAT_PR_DBG("clk_disable_unprepare(clk_infracfg_ao_ccif4_ap_cg) calling\n");

		clk_disable_unprepare(clk_scp_conn_main);
		WMT_PLAT_PR_DBG("clk_disable_unprepare(clk_scp_conn_main) calling\n");
#else
		/* Turn on AHB bus sleep protect (AP2CONN AHB Bus protect)
		 * (apply this for INFRA AXI bus protection to prevent bus hang
		 * when CONNSYS had been turned off)
		 */
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AHBAXI_PROT_EN_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base +
				 CONSYS_AHBAXI_PROT_EN_OFFSET) &
				 CONSYS_AP2CONN_PROT_MASK);
		value = CONSYS_AP2CONN_PROT_MASK &
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AHBAXI_PROT_STA_OFFSET);
		i = 0;
		while (value == CONSYS_AP2CONN_PROT_MASK || i > 10) {
			value = CONSYS_AP2CONN_PROT_MASK &
				CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AHBAXI_PROT_STA_OFFSET);
			i++;
		}
		/* Turn on AXI Tx bus sleep protect (CONN2AP AXI Tx Bus protect)
		 * (apply this for INFRA AXI bus protection to prevent bus hang
		 * when CONNSYS had been turned off)
		 * Note : Should turn on AXI Tx sleep protection first.
		 */
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AHBAXI_PROT_EN_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base +
				 CONSYS_AHBAXI_PROT_EN_OFFSET) &
				 CONSYS_TX_PROT_MASK);
		value = CONSYS_TX_PROT_MASK &
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AHBAXI_PROT_STA_OFFSET);
		i = 0;
		while (value == CONSYS_TX_PROT_MASK || i > 10) {
			value = CONSYS_TX_PROT_MASK &
				CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AHBAXI_PROT_STA_OFFSET);
			i++;
		}
		/* Turn on AXI Rx bus sleep protect (CONN2AP AXI RX Bus protect)
		 * (apply this for INFRA AXI bus protection to prevent bus hang
		 * when CONNSYS had been turned off)
		 * Note : Should turn on AXI Rx sleep protection
		 * after AXI Tx sleep protection has been turn on.
		 */
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AHBAXI_PROT_EN_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base +
				 CONSYS_AHBAXI_PROT_EN_OFFSET) &
				 CONSYS_RX_PROT_MASK);
		value = CONSYS_RX_PROT_MASK &
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AHBAXI_PROT_STA_OFFSET);
		i = 0;
		while (value == CONSYS_RX_PROT_MASK || i > 10) {
			value = CONSYS_RX_PROT_MASK &
				CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AHBAXI_PROT_STA_OFFSET);
			i++;
		}

		/*assert "conn_top_on" isolation, set "connsys_iso_en"=1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_ISO_S_BIT);
		/*assert CONNSYS S/W reset (TOP RGU CR), set "ap_sw_rst_b"=0 */
		CONSYS_REG_WRITE(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET,
				(CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET) &
				 CONSYS_SW_RST_BIT) | CONSYS_CPU_SW_RST_CTRL_KEY);
		/*assert CONNSYS S/W reset (SPM CR), set "ap_sw_rst_b"=0 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 ~CONSYS_SPM_PWR_RST_BIT);
		/*write conn_clk_dis=1, disable connsys clock  0x1000632C [4]  1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_CLK_CTRL_BIT);
		/*wait 1us      */
		udelay(1);
		/*write conn_top1_pwr_on=0, power off conn_top1 0x1000632C [3:2] 2'b00 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~(CONSYS_SPM_PWR_ON_BIT | CONSYS_SPM_PWR_ON_S_BIT));
#endif /* CONSYS_PWR_ON_OFF_API_AVAILABLE */
	}

#if CONSYS_PWR_ON_OFF_API_AVAILABLE
	return iRet;
#else
	return 0;
#endif
}

static INT32 consys_ahb_clock_ctrl(MTK_WCN_BOOL enable)
{
	return 0;
}

static INT32 polling_consys_chipid(VOID)
{
	UINT32 retry = 10;
	UINT32 consys_ver_id = 0;
	UINT32 consys_hw_ver = 0;
	UINT32 consys_fw_ver = 0;
	UINT8 *consys_reg_base = NULL;
	UINT32 value = 0;

	/*12.poll CONNSYS CHIP ID until chipid is returned  0x18070008 */
	while (retry-- > 0) {
		consys_ver_id = CONSYS_REG_READ(conn_reg.mcu_top_misc_off_base +
				CONSYS_IP_VER_OFFSET);
		if (consys_ver_id == CONSYS_IP_VER_ID) {
			WMT_PLAT_PR_INFO("retry(%d)consys version id(0x%08x)\n",
					retry, consys_ver_id);
			consys_hw_ver = CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HW_ID_OFFSET);
			WMT_PLAT_PR_INFO("consys HW version id(0x%x)\n", consys_hw_ver & 0xFFFF);
			consys_fw_ver = CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_FW_ID_OFFSET);
			WMT_PLAT_PR_INFO("consys FW version id(0x%x)\n", consys_fw_ver & 0xFFFF);

			consys_dl_rom_patch(consys_ver_id, consys_fw_ver);
			break;
		}
		WMT_PLAT_PR_ERR("Read CONSYS version id(0x%08x)", consys_ver_id);
		msleep(20);
	}
	consys_ver_id = CONSYS_REG_READ(conn_reg.mcu_top_misc_off_base + CONSYS_CONF_ID_OFFSET);
	WMT_PLAT_PR_INFO("consys configuration id(0x%x)\n", consys_ver_id & 0xF);
	consys_ver_id = CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HW_ID_OFFSET);
	WMT_PLAT_PR_INFO("consys HW version id(0x%x)\n", consys_ver_id & 0xFFFF);
	consys_ver_id = CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_FW_ID_OFFSET);
	WMT_PLAT_PR_INFO("consys FW version id(0x%x)\n", consys_ver_id & 0xFFFF);

	/* set CR "XO initial stable time" and "XO bg stable time"
	 * to optimize the wakeup time after sleep
	 * clear "source clock enable ack to XO state" mask
	 */
	if (mtk_wcn_soc_co_clock_get()) {
		consys_reg_base = ioremap_nocache(CONSYS_COCLOCK_STABLE_TIME_BASE, 0x100);
		if (consys_reg_base) {
			value = CONSYS_REG_READ(consys_reg_base);
			value = (value & CONSYS_COCLOCK_STABLE_TIME_MASK) |
					CONSYS_COCLOCK_STABLE_TIME;
			CONSYS_REG_WRITE(consys_reg_base, value);
			value = CONSYS_REG_READ(consys_reg_base + CONSYS_COCLOCK_ACK_ENABLE_OFFSET);
			value = value & (~CONSYS_COCLOCK_ACK_ENABLE_BIT);
			CONSYS_REG_WRITE(consys_reg_base + CONSYS_COCLOCK_ACK_ENABLE_OFFSET, value);
			iounmap(consys_reg_base);
		} else
			WMT_PLAT_PR_ERR("connsys co_clock stable time base(0x%x) ioremap fail!\n",
					  CONSYS_COCLOCK_STABLE_TIME_BASE);
	}

	/* EMI control CR setting(bypass hw mode) */
	CONSYS_SET_BIT(conn_reg.mcu_base + CONSYS_SW_IRQ_OFFSET, CONSYS_EMI_CTRL_VALUE);

	/* write reserverd cr for identify adie is 6635 or 6631 */
	consys_reg_base = ioremap_nocache(CONSYS_IDENTIFY_ADIE_CR_ADDRESS, 0x8);
	if (consys_reg_base) {
		value = CONSYS_REG_READ(consys_reg_base);
#if CONSYS_PMIC_CTRL_6635
		value = value & (~CONSYS_IDENTIFY_ADIE_ENABLE_BIT);
#else
		value = value & (CONSYS_IDENTIFY_ADIE_ENABLE_BIT);
#endif
		CONSYS_REG_WRITE(consys_reg_base, value);
		iounmap(consys_reg_base);
	} else
		WMT_PLAT_PR_ERR("connsys identify adie cr address(0x%x) ioremap fail!\n",
				  CONSYS_IDENTIFY_ADIE_CR_ADDRESS);

#if CONSYS_PMIC_CTRL_6635
	/* if(MT6635)
	 * CONN_WF_CTRL2 swtich to GPIO mode, GPIO output value
	 * before patch download swtich back to CONN mode.
	 */
	consys_reg_base = ioremap_nocache(CONSYS_IF_PINMUX_REG_BASE, 0x1000);
	if (!consys_reg_base) {
		WMT_PLAT_PR_ERR("consys_if_pinmux_reg_base(%x) ioremap fail\n",
				CONSYS_IF_PINMUX_REG_BASE);
		return 0;
	}

	CONSYS_REG_WRITE(consys_reg_base + CONSYS_WF_CTRL2_01_OFFSET,
			(CONSYS_REG_READ(consys_reg_base + CONSYS_WF_CTRL2_01_OFFSET) &
			CONSYS_WF_CTRL2_01_MASK) | CONSYS_WF_CTRL2_01_VALUE);
	CONSYS_REG_WRITE(consys_reg_base + CONSYS_WF_CTRL2_02_OFFSET,
			(CONSYS_REG_READ(consys_reg_base + CONSYS_WF_CTRL2_02_OFFSET) &
			CONSYS_WF_CTRL2_02_MASK) | CONSYS_WF_CTRL2_02_VALUE);
	CONSYS_REG_WRITE(consys_reg_base + CONSYS_WF_CTRL2_03_OFFSET,
			(CONSYS_REG_READ(consys_reg_base + CONSYS_WF_CTRL2_03_OFFSET) &
			CONSYS_WF_CTRL2_03_MASK) | CONSYS_WF_CTRL2_GPIO_MODE);
	if (consys_reg_base)
		iounmap(consys_reg_base);
	else
		WMT_PLAT_PR_ERR("consys_if_pinmux_reg_base(0x%x) ioremap fail!\n",
				  CONSYS_IF_PINMUX_REG_BASE);
#endif
	/* connsys bus time out configure,  enable AHB bus timeout */
	consys_reg_base = ioremap_nocache(CONSYS_AHB_TIMEOUT_EN_ADDRESS, 0x100);
	if (consys_reg_base) {
		CONSYS_REG_WRITE(consys_reg_base, CONSYS_AHB_TIMEOUT_EN_ADDRESS);
		iounmap(consys_reg_base);
	} else
		WMT_PLAT_PR_ERR("CONSYS_AHB_TIMEOUT_EN_ADDRESS(0x%x) ioremap fail!\n",
				  CONSYS_AHB_TIMEOUT_EN_ADDRESS);

	return 0;
}

static VOID consys_acr_reg_setting(VOID)
{
	WMT_PLAT_PR_INFO("No need to do acr");
}

static VOID consys_afe_reg_setting(VOID)
{
	WMT_PLAT_PR_INFO("No need to do afe");
}

static INT32 consys_hw_vcn18_ctrl(MTK_WCN_BOOL enable)
{
#if CONSYS_PMIC_CTRL_ENABLE
	if (enable) {
		/*need PMIC driver provide new API protocol */
		/*1.AP power on VCN_1V8 LDO (with PMIC_WRAP API) VCN_1V8  */
		/*default vcn18 SW mode*/
		if (reg_VCN18) {
			regulator_set_voltage(reg_VCN18, 1800000, 1800000);
			if (regulator_enable(reg_VCN18))
				WMT_PLAT_PR_ERR("enable VCN18 fail\n");
			else
				WMT_PLAT_PR_DBG("enable VCN18 ok\n");
		}

		/*set vcn18 HW mode*/
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_HW0_OP_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_HW0_OP_EN, 0);

#if CONSYS_PMIC_CTRL_6635
		/* delay 300us (VCN18 stable time) */
		udelay(300);
		/*1.AP power on VCN_1V3 LDO (with PMIC_WRAP API) VCN_1V3  */
		/*default vcn13 SW mode*/
		if (reg_VCN13) {
			regulator_set_voltage(reg_VCN13, 1300000, 1300000);
			if (regulator_enable(reg_VCN13))
				WMT_PLAT_PR_ERR("enable VCN13 fail\n");
			else
				WMT_PLAT_PR_DBG("enable VCN13 ok\n");
		}

		/*set vcn13 HW mode*/
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN13_HW0_OP_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN13_HW0_OP_EN, 0);
#else
		/*1.AP power on VCN_3V3 LDO (with PMIC_WRAP API) VCN_3V3  */
		/*default vcn33_1 SW mode*/
		if (reg_VCN33_BT) {
			regulator_set_voltage(reg_VCN33_BT, 3300000, 3300000);
			if (regulator_enable(reg_VCN33_BT))
				WMT_PLAT_PR_ERR("WMT do BT PMIC on fail!\n");
		}
#endif
	} else {
#if CONSYS_PMIC_CTRL_6635
		if (reg_VCN13)
			regulator_disable(reg_VCN13);
#else
		if (reg_VCN33_BT)
			regulator_disable(reg_VCN33_BT);
#endif
		/*AP power off MT6351L VCN_1V8 LDO */
		if (reg_VCN18) {
			if (regulator_disable(reg_VCN18))
				WMT_PLAT_PR_ERR("disable VCN_1V8 fail!\n");
			else
				WMT_PLAT_PR_DBG("disable VCN_1V8 ok\n");
		}
	}
#endif
	return 0;
}

static VOID consys_vcn28_hw_mode_ctrl(UINT32 enable)
{
#if CONSYS_PMIC_CTRL_ENABLE
#if CONSYS_PMIC_CTRL_6635
	/* 6635 not supported */
#else
	if (enable) {
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN28_HW0_OP_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN28_HW0_OP_CFG, 0);
	} else {
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN28_HW0_OP_EN, 0);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN28_HW0_OP_CFG, 0);
	}
#endif
#endif
}

static INT32 consys_hw_vcn28_ctrl(UINT32 enable)
{
#if CONSYS_PMIC_CTRL_ENABLE
#if CONSYS_PMIC_CTRL_6635
	/* 6635 not supported */
#else
	if (enable) {
		/*in co-clock mode,need to turn on vcn28 when fm on */
		if (reg_VCN28) {
			regulator_set_voltage(reg_VCN28, 2800000, 2800000);
			if (regulator_enable(reg_VCN28))
				WMT_PLAT_PR_ERR("WMT do VCN28 PMIC on fail!\n");
		}
		WMT_PLAT_PR_INFO("turn on vcn28 for fm/gps usage in co-clock mode\n");
	} else {
		/*in co-clock mode,need to turn off vcn28 when fm off */
		if (reg_VCN28)
			regulator_disable(reg_VCN28);
		WMT_PLAT_PR_INFO("turn off vcn28 for fm/gps usage in co-clock mode\n");
	}
#endif
#endif

	return 0;
}

static INT32 consys_hw_bt_vcn33_ctrl(UINT32 enable)
{
#if CONSYS_PMIC_CTRL_6635
	if (enable) {
		/*do BT PMIC on,depenency PMIC API ready */
		/*switch BT PALDO control from SW mode to HW mode:0x416[5]-->0x1 */
#if CONSYS_PMIC_CTRL_ENABLE
		/* VOL_DEFAULT, VOL_3300, VOL_3400, VOL_3500, VOL_3600 */
		/*default vcn33_1 SW mode*/
		if (reg_VCN33_1_BT) {
			regulator_set_voltage(reg_VCN33_1_BT, 3300000, 3300000);
			if (regulator_enable(reg_VCN33_1_BT))
				WMT_PLAT_PR_ERR("WMT do BT PMIC on fail!\n");
		}

		/*set vcn33_1 HW mode*/
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_HW0_OP_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_HW0_OP_CFG, 0);
#endif
		WMT_PLAT_PR_DBG("WMT do BT PMIC on\n");
	} else {
		/*do BT PMIC off */
		/*switch BT PALDO control from HW mode to SW mode:0x416[5]-->0x0 */
#if CONSYS_PMIC_CTRL_ENABLE
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_HW0_OP_EN, 0);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_HW0_OP_CFG, 0);
		if (reg_VCN33_1_BT)
			regulator_disable(reg_VCN33_1_BT);
#endif
		WMT_PLAT_PR_DBG("WMT do BT PMIC off\n");
	}
#endif

	return 0;
}

static INT32 consys_hw_wifi_vcn33_ctrl(UINT32 enable)
{
#if CONSYS_PMIC_CTRL_6635
	if (enable) {
		/*do WIFI PMIC on,depenency PMIC API ready */
		/*switch WIFI PALDO control from SW mode to HW mode:0x418[14]-->0x1 */
#if CONSYS_PMIC_CTRL_ENABLE
		/*default vcn33_1 SW mode*/
		if (reg_VCN33_1_WIFI) {
			regulator_set_voltage(reg_VCN33_1_WIFI, 3300000, 3300000);
			if (regulator_enable(reg_VCN33_1_WIFI))
				WMT_PLAT_PR_ERR("WMT do WIFI PMIC on fail!\n");
		}

		/*set vcn33_1 HW mode*/
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_HW0_OP_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_HW0_OP_CFG, 0);

		/*default vcn33_3 SW mode*/
		if (reg_VCN33_2_WIFI) {
			regulator_set_voltage(reg_VCN33_2_WIFI, 3300000, 3300000);
			if (regulator_enable(reg_VCN33_2_WIFI))
				WMT_PLAT_PR_ERR("WMT do WIFI PMIC on fail!\n");
		}

		/*set vcn33_2 HW mode*/
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_2_HW0_OP_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_2_HW0_OP_CFG, 0);
#endif
		WMT_PLAT_PR_DBG("WMT do WIFI PMIC on\n");
	} else {
		/*do WIFI PMIC off */
		/*switch WIFI PALDO control from HW mode to SW mode:0x418[14]-->0x0 */
#if CONSYS_PMIC_CTRL_ENABLE
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_HW0_OP_EN, 0);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_1_HW0_OP_CFG, 0);
		if (reg_VCN33_1_WIFI)
			regulator_disable(reg_VCN33_1_WIFI);

		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_2_HW0_OP_EN, 0);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_2_HW0_OP_CFG, 0);
		if (reg_VCN33_2_WIFI)
			regulator_disable(reg_VCN33_2_WIFI);
#endif
		WMT_PLAT_PR_DBG("WMT do WIFI PMIC off\n");
	}

#endif

	return 0;
}

static INT32 consys_emi_mpu_set_region_protection(VOID)
{
#ifdef CONFIG_MTK_EMI
	struct emi_region_info_t region_info;

	/*set MPU for EMI share Memory */
	WMT_PLAT_PR_INFO("setting MPU for EMI share memory\n");

	region_info.start = gConEmiPhyBase;
	region_info.end = gConEmiPhyBase + gConEmiSize - 1;
	region_info.region = 27;
	SET_ACCESS_PERMISSION(region_info.apc, LOCK,
			FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
			FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
			NO_PROTECTION, FORBIDDEN, NO_PROTECTION);
	emi_mpu_set_protection(&region_info);
#endif

	return 0;
}

static UINT32 consys_emi_set_remapping_reg(VOID)
{
	/* For direct path */
	phys_addr_t mdPhy = 0;
	INT32 size = 0;

	mtk_wcn_emi_addr_info.emi_ap_phy_addr = gConEmiPhyBase;
	mtk_wcn_emi_addr_info.emi_size = gConEmiSize;

	/*EMI Registers remapping*/
	CONSYS_REG_WRITE_OFFSET_RANGE(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET,
					  gConEmiPhyBase, 0, 16, 20);
	WMT_PLAT_PR_INFO("CONSYS_EMI_MAPPING dump in restore cb(0x%08x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET));

	/*Perisys Configuration Registers remapping*/
	CONSYS_REG_WRITE_OFFSET_RANGE(conn_reg.topckgen_base + CONSYS_EMI_PERI_MAPPING_OFFSET,
					  0x10003000, 0, 16, 20);
	WMT_PLAT_PR_INFO("PERISYS_MAPPING dump in restore cb(0x%08x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_PERI_MAPPING_OFFSET));

	/*Modem Configuration Registers remapping*/
	mdPhy = get_smem_phy_start_addr(MD_SYS1, SMEM_USER_RAW_MD_CONSYS, &size);
	if (size == 0)
		WMT_PLAT_PR_INFO("MD direct path is not supported\n");
	else {
		CONSYS_REG_WRITE_OFFSET_RANGE(
			conn_reg.topckgen_base + CONSYS_EMI_AP_MD_OFFSET,
			mdPhy, 0, 16, 20);
		WMT_PLAT_PR_INFO("MD_MAPPING dump in restore cb(0x%08x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_AP_MD_OFFSET));
	}

	return 0;
}

static INT32 bt_wifi_share_v33_spin_lock_init(VOID)
{
	return 0;
}

static INT32 consys_read_irq_info_from_dts(struct platform_device *pdev, PINT32 irq_num,
		PUINT32 irq_flag)
{
	struct device_node *node;

	node = pdev->dev.of_node;
	if (node) {
		*irq_num = irq_of_parse_and_map(node, 0);
		*irq_flag = irq_get_trigger_type(*irq_num);
		WMT_PLAT_PR_INFO("get irq id(%d) and irq trigger flag(%d) from DT\n", *irq_num,
				   *irq_flag);
	} else {
		WMT_PLAT_PR_ERR("[%s] can't find CONSYS compatible node\n", __func__);
		return -1;
	}

	return 0;
}

static INT32 consys_read_reg_from_dts(struct platform_device *pdev)
{
	INT32 iRet = -1;
	struct device_node *node = NULL;

	node = pdev->dev.of_node;
	if (node) {
		/* registers base address */
		conn_reg.mcu_base = (SIZE_T) of_iomap(node, MCU_BASE_INDEX);
		WMT_PLAT_PR_DBG("Get mcu register base(0x%zx)\n", conn_reg.mcu_base);
		conn_reg.ap_rgu_base = (SIZE_T) of_iomap(node, TOP_RGU_BASE_INDEX);
		WMT_PLAT_PR_DBG("Get ap_rgu register base(0x%zx)\n", conn_reg.ap_rgu_base);
		conn_reg.topckgen_base = (SIZE_T) of_iomap(node, INFRACFG_AO_BASE_INDEX);
		WMT_PLAT_PR_DBG("Get topckgen register base(0x%zx)\n", conn_reg.topckgen_base);
		conn_reg.spm_base = (SIZE_T) of_iomap(node, SPM_BASE_INDEX);
		WMT_PLAT_PR_DBG("Get spm register base(0x%zx)\n", conn_reg.spm_base);
		conn_reg.mcu_conn_hif_on_base = (SIZE_T) of_iomap(node, MCU_CONN_HIF_ON_BASE_INDEX);
		WMT_PLAT_PR_DBG("Get mcu_conn_hif_on register base(0x%zx)\n",
				conn_reg.mcu_conn_hif_on_base);
		conn_reg.mcu_top_misc_off_base = (SIZE_T) of_iomap(node,
				MCU_TOP_MISC_OFF_BASE_INDEX);
		WMT_PLAT_PR_DBG("Get mcu_top_misc_off register base(0x%zx)\n",
				conn_reg.mcu_top_misc_off_base);
		conn_reg.mcu_cfg_on_base = (SIZE_T) of_iomap(node, MCU_CFG_ON_BASE_INDEX);
		WMT_PLAT_PR_DBG("Get mcu_cfg_on register base(0x%zx)\n", conn_reg.mcu_cfg_on_base);
		conn_reg.mcu_cirq_base = (SIZE_T) of_iomap(node, MCU_CIRQ_BASE_INDEX);
		WMT_PLAT_PR_DBG("Get mcu cirq  register base(0x%zx)\n", conn_reg.mcu_cirq_base);
		conn_reg.mcu_top_misc_on_base = (SIZE_T) of_iomap(node, MCU_TOP_MISC_ON_BASE_INDEX);
		WMT_PLAT_PR_DBG("Get mcu_top_misc_off register base(0x%zx)\n",
				conn_reg.mcu_top_misc_on_base);
	} else {
		WMT_PLAT_PR_ERR("[%s] can't find CONSYS compatible node\n", __func__);
		return iRet;
	}

	return 0;
}

static VOID force_trigger_assert_debug_pin(VOID)
{
	CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET,
			CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AP2CONN_OSC_EN_OFFSET) & ~CONSYS_AP2CONN_WAKEUP_BIT);
	WMT_PLAT_PR_INFO("enable:dump CONSYS_AP2CONN_OSC_EN_REG(0x%x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET));
	usleep_range(64, 96);
	CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET,
			CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AP2CONN_OSC_EN_OFFSET) | CONSYS_AP2CONN_WAKEUP_BIT);
	WMT_PLAT_PR_INFO("disable:dump CONSYS_AP2CONN_OSC_EN_REG(0x%x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET));
}

static UINT32 consys_read_cpupcr(VOID)
{
	return CONSYS_REG_READ(conn_reg.mcu_conn_hif_on_base + CONSYS_CPUPCR_OFFSET);
}

static UINT32 consys_soc_chipid_get(VOID)
{
	return PLATFORM_SOC_CHIP;
}

static P_CONSYS_EMI_ADDR_INFO consys_soc_get_emi_phy_add(VOID)
{
	return &mtk_wcn_emi_addr_info;
}

P_WMT_CONSYS_IC_OPS mtk_wcn_get_consys_ic_ops(VOID)
{
	return &consys_ic_ops;
}

static INT32 consys_dl_rom_patch(UINT32 ip_ver, UINT32 fw_ver)
{
	if (rom_patch_dl_flag) {
		if (mtk_wcn_soc_rom_patch_dwn(ip_ver, fw_ver) == 0)
			rom_patch_dl_flag = 1;
	}

	return 0;
}

static VOID consys_set_dl_rom_patch_flag(INT32 flag)
{
	rom_patch_dl_flag = flag;
}

static INT32 consys_dedicated_log_path_init(struct platform_device *pdev)
{
	struct device_node *node;
	UINT32 irq_num;
	UINT32 irq_flag;
	INT32 iret = -1;

	node = pdev->dev.of_node;
	if (node) {
		irq_num = irq_of_parse_and_map(node, 2);
		irq_flag = irq_get_trigger_type(irq_num);
		WMT_PLAT_PR_INFO("get conn2ap_sw_irq id(%d) and irq trigger flag(%d) from DT\n",
				irq_num, irq_flag);
	} else {
		WMT_PLAT_PR_ERR("[%s] can't find CONSYS compatible node\n", __func__);
		return iret;
	}

	connsys_dedicated_log_path_apsoc_init(gConEmiPhyBase, irq_num, irq_flag);
#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
	fw_log_wmt_init();
#endif
	return 0;
}

static VOID consys_dedicated_log_path_deinit(VOID)
{
#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
	fw_log_wmt_deinit();
#endif
	connsys_dedicated_log_path_apsoc_deinit();
}

static INT32 consys_emi_coredump_remapping(UINT8 __iomem **addr, UINT32 enable)
{
	if (enable) {
		*addr = ioremap_nocache(gConEmiPhyBase + CONSYS_EMI_COREDUMP_OFFSET,
				CONSYS_EMI_MEM_SIZE);
		if (*addr) {
			WMT_PLAT_PR_INFO("COREDUMP EMI mapping OK virtual(0x%p) physical(0x%x)\n",
					*addr, (UINT32) gConEmiPhyBase +
					CONSYS_EMI_COREDUMP_OFFSET);
			memset_io(*addr, 0, CONSYS_EMI_MEM_SIZE);
		} else {
			WMT_PLAT_PR_ERR("EMI mapping fail\n");
			return -1;
		}
	} else {
		if (*addr) {
			iounmap(*addr);
			*addr = NULL;
		}
	}
	return 0;
}

static INT32 consys_reset_emi_coredump(UINT8 __iomem *addr)
{
	if (!addr) {
		WMT_PLAT_PR_ERR("get virtual address fail\n");
		return -1;
	}
	WMT_PLAT_PR_INFO("Reset EMI(0xF0068000 ~ 0xF0068400) and (0xF0070400 ~ 0xF0078400)\n");
	/* reset 0xF0068000 ~ 0xF0068400 (1K) */
	memset_io(addr, 0, 0x400);
	/* reset 0xF0070400 ~ 0xF0078400 (32K)  */
	memset_io(addr + CONSYS_EMI_PAGED_DUMP_OFFSET, 0, 0x8000);
	return 0;
}

static INT32 consys_check_reg_readable(VOID)
{
#if 0 /* not support yet, need confirm by DE */
	INT32 flag = 0;
	UINT32 value = 0;

	/*check connsys clock and sleep status*/
	CONSYS_REG_WRITE(conn_reg.mcu_conn_hif_on_base, CONSYS_CLOCK_CHECK_VALUE);
	udelay(1000);
	value = CONSYS_REG_READ(conn_reg.mcu_conn_hif_on_base);
	if ((value & CONSYS_HCLK_CHECK_BIT) &&
	    (value & CONSYS_OSCCLK_CHECK_BIT) &&
	    ((value & CONSYS_SLEEP_CHECK_BIT) == 0))
		flag = 1;
	if (!flag)
		WMT_PLAT_PR_ERR("connsys clock check fail 0x18007000(0x%x)\n", value);

	return flag;
#endif

	return 0;
}

static VOID consys_ic_clock_fail_dump(VOID)
{
	WMT_PLAT_PR_ERR("CONN_HIF_TOP_MISC=0x%08x CONN_HIF_BUSY_STATUS=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HIF_TOP_MISC),
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HIF_BUSY_STATUS));

	CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_HIF_DBG_IDX, 0x3333);
	WMT_PLAT_PR_ERR("Write CONSYS_HIF_DBG_IDX to 0x3333\n");

	WMT_PLAT_PR_ERR("CONSYS_HIF_DBG_PROBE=0x%08x CONN_HIF_TOP_MISC=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HIF_DBG_PROBE),
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HIF_TOP_MISC));
	WMT_PLAT_PR_ERR("CONN_HIF_BUSY_STATUS=0x%08x CONN_HIF_PDMA_BUSY_STATUS=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HIF_BUSY_STATUS),
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HIF_PDMA_BUSY_STATUS));

	CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_HIF_DBG_IDX, 0x2222);
	WMT_PLAT_PR_ERR("Write CONSYS_HIF_DBG_IDX to 0x2222\n");

	WMT_PLAT_PR_ERR("CONSYS_HIF_DBG_PROBE=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HIF_DBG_PROBE));

	CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_HIF_DBG_IDX, 0x3333);
	WMT_PLAT_PR_ERR("Write CONSYS_HIF_DBG_IDX to 0x3333\n");

	WMT_PLAT_PR_ERR("CONSYS_HIF_DBG_PROBE=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HIF_DBG_PROBE));

	CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_HIF_DBG_IDX, 0x4444);
	WMT_PLAT_PR_ERR("Write CONSYS_HIF_DBG_IDX to 0x4444\n");

	WMT_PLAT_PR_ERR("CONSYS_HIF_DBG_PROBE=0x%08x CONN_MCU_EMI_CONTROL=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_HIF_DBG_PROBE),
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_SW_IRQ_OFFSET));
	WMT_PLAT_PR_ERR("CONN_MCU_CLOCK_CONTROL=0x%08x CONN_MCU_BUS_CONTROL=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_CLOCK_CONTROL),
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_BUS_CONTROL));

	CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_DEBUG_SELECT, 0x003e3d00);
	WMT_PLAT_PR_ERR("Write CONSYS_DEBUG_SELECT to 0x003e3d00\n");

	WMT_PLAT_PR_ERR("CONN_MCU_DEBUG_STATUS=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_DEBUG_STATUS));

	CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_DEBUG_SELECT, 0x00403f00);
	WMT_PLAT_PR_ERR("Write CONSYS_DEBUG_SELECT to 0x00403f00\n");

	WMT_PLAT_PR_ERR("CONN_MCU_DEBUG_STATUS=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_DEBUG_STATUS));

	CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_DEBUG_SELECT, 0x00424100);
	WMT_PLAT_PR_ERR("Write CONSYS_DEBUG_SELECT to 0x00424100\n");

	WMT_PLAT_PR_ERR("CONN_MCU_DEBUG_STATUS=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_DEBUG_STATUS));

	CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_DEBUG_SELECT, 0x00444300);
	WMT_PLAT_PR_ERR("Write CONSYS_DEBUG_SELECT to 0x00444300\n");

	WMT_PLAT_PR_ERR("CONN_MCU_DEBUG_STATUS=0x%08x\n",
		CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_DEBUG_STATUS));
}

static INT32 consys_is_connsys_reg(UINT32 addr)
{
	if (addr > 0x18000000 && addr < 0x180FFFFF) {
		if (addr >= 0x18007000 && addr <= 0x18007FFF)
			return 0;

		return 1;
	}

	return 0;
}

static VOID consys_resume_dump_info(VOID)
{
	stp_dbg_poll_cpupcr(5, 0, 1);

	if (conn_reg.mcu_cfg_on_base != 0 &&
	    conn_reg.mcu_top_misc_on_base != 0 &&
	    mtk_consys_check_reg_readable()) {
		CONSYS_REG_WRITE(conn_reg.mcu_cfg_on_base + 0x104, 0x1);
		CONSYS_REG_WRITE(conn_reg.mcu_top_misc_on_base + 0x320, 0x80000001);
		CONSYS_REG_WRITE(conn_reg.mcu_top_misc_on_base + 0x310, 0x3);
		WMT_PLAT_PR_INFO("0x180c1340: 0x%x\n",
				CONSYS_REG_READ(conn_reg.mcu_top_misc_on_base + 0x340));
		CONSYS_REG_WRITE(conn_reg.mcu_cfg_on_base + 0x104, 0x0);
	}
}
