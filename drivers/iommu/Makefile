obj-$(CONFIG_IOMMU_API) += iommu.o
obj-$(CONFIG_OF_IOMMU)	+= of_iommu.o
obj-$(CONFIG_MSM_IOMMU) += msm_iommu-v0.o msm_iommu_dev-v0.o
ifdef CONFIG_OF
obj-$(CONFIG_MSM_IOMMU) += msm_iommu-v1.o msm_iommu_dev-v1.o msm_iommu_pagetable.o msm_iommu_sec.o
endif
obj-$(CONFIG_MSM_IOMMU_PMON) += msm_iommu_perfmon.o
obj-$(CONFIG_AMD_IOMMU) += amd_iommu.o amd_iommu_init.o
obj-$(CONFIG_AMD_IOMMU_V2) += amd_iommu_v2.o
obj-$(CONFIG_DMAR_TABLE) += dmar.o
obj-$(CONFIG_INTEL_IOMMU) += iova.o intel-iommu.o
obj-$(CONFIG_IRQ_REMAP) += intel_irq_remapping.o irq_remapping.o
obj-$(CONFIG_OMAP_IOMMU) += omap-iommu.o
obj-$(CONFIG_OMAP_IOVMM) += omap-iovmm.o
obj-$(CONFIG_OMAP_IOMMU_DEBUG) += omap-iommu-debug.o
obj-$(CONFIG_TEGRA_IOMMU_GART) += tegra-gart.o
obj-$(CONFIG_TEGRA_IOMMU_SMMU) += tegra-smmu.o
obj-$(CONFIG_EXYNOS_IOMMU) += exynos-iommu.o
