## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CPU_IBM_POWER9),y)

bootblock-y += bootblock.c
postcar-y += exit_car.S
romstage-y += romstage.c
romstage-y += istep_8_6.c
ramstage-y += chip.c
ramstage-y += timer.c

endif
