## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(SOC_RAPTOR_POWER9),y)

bootblock-y += bootblock.c
romstage-y += romstage.c

endif
