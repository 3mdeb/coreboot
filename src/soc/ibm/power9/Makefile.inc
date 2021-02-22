## SPDX-License-Identifier: GPL-2.0-only

ifeq ($(CONFIG_CPU_IBM_POWER9),y)

bootblock-y += bootblock.c
postcar-y += exit_car.S
romstage-y += istep_8_9.c
romstage-y += istep_8_10.c
romstage-y += i2c.c

ramstage-y += chip.c
ramstage-y += timer.c

endif
