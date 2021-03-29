## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_CPU_IBM_POWER9),y)

bootblock-y += bootblock.c
postcar-y += exit_car.S
romstage-y += istep_13_2.c
romstage-y += istep_13_3.c
romstage-y += istep_13_4.c
romstage-y += istep_13_6.c
romstage-y += istep_13_8.c
romstage-y += istep_13_9.c
romstage-y += istep_13_10.c
romstage-y += istep_13_11.c
romstage-y += i2c.c
romstage-y += ccs.c
ramstage-y += chip.c
ramstage-y += timer.c

endif
