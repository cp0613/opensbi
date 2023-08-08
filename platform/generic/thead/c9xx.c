/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Samuel Holland <samuel@sholland.org>
 */

#include <platform_override.h>
#include <thead_c9xx.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_pmu.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/irqchip/fdt_irqchip_plic.h>

static void thead_c9xx_pmu_ctr_enable_irq(uint32_t ctr_idx)
{
	unsigned long mip_val;

	if (ctr_idx >= SBI_PMU_HW_CTR_MAX)
		return;

	mip_val = csr_read(CSR_MIP);
	/**
	 * Clear out the OF bit so that next interrupt can be enabled.
	 * This should be done only when the corresponding overflow interrupt
	 * bit is cleared. That indicates that software has already handled the
	 * previous interrupts or the hardware yet to set an overflow interrupt.
	 * Otherwise, there will be race conditions where we may clear the bit
	 * the software is yet to handle the interrupt.
	 */
	if (!(mip_val & THEAD_C9XX_MIP_MOIP))
		csr_clear(THEAD_C9XX_CSR_MCOUNTEROF, BIT(ctr_idx));

	/**
	 * SSCOFPMF uses the OF bit for enabling/disabling the interrupt,
	 * while the C9XX has designated enable bits.
	 * So enable per-counter interrupt on C9xx here.
	 */
	csr_set(THEAD_C9XX_CSR_MCOUNTERINTEN, BIT(ctr_idx));
}

static void thead_c9xx_pmu_ctr_disable_irq(uint32_t ctr_idx)
{
	csr_clear(THEAD_C9XX_CSR_MCOUNTERINTEN, BIT(ctr_idx));
}

static int thead_c9xx_pmu_irq_bit(void)
{
	return THEAD_C9XX_MIP_MOIP;
}

const struct sbi_pmu_device thead_c9xx_pmu_device = {
	.name = "thead_pmu",
	.hw_counter_enable_irq = thead_c9xx_pmu_ctr_enable_irq,
	.hw_counter_disable_irq = thead_c9xx_pmu_ctr_disable_irq,
	.hw_counter_irq_bit = thead_c9xx_pmu_irq_bit,
};

static int thead_c9xx_extensions_init(const struct fdt_match *match,
				     struct sbi_hart_features *hfeatures)
{
	sbi_pmu_set_device(&thead_c9xx_pmu_device);

	csr_write(THEAD_C9XX_CSR_MCOUNTERWEN, 0xffffffff);

	return 0;
}

static const struct fdt_match thead_c9xx_match[] = {
	{ .compatible = "thead,c9xx" },
	{ },
};

const struct platform_override thead_c9xx = {
	.match_table	= thead_c9xx_match,
	.extensions_init = thead_c9xx_extensions_init,
};
