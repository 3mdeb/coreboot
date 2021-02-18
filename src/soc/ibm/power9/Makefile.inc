## SPDX-License-Identifier: GPL-2.0-only

<<<<<<< HEAD
<<<<<<< HEAD
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
romstage-y += istep_13_13.c
romstage-y += istep_14_2.c
romstage-y += i2c.c
romstage-y += ccs.c
=======
# ifeq ($(CPU_IBM_POWER9),y)
=======
ifeq ($(CPU_IBM_POWER9),y)
>>>>>>> cpu/power9: fix directory structure and timer build fault

bootblock-y += bootblock.c
postcar-y += exit_car.S
romstage-y += romstage.c
romstage-y += istep_8_9.c
romstage-y += istep_8_10.c
romstage-y += ../../../cpu/power/power9/scom.c
>>>>>>> move isteps 8.9, 8.10 to soc subdir

ramstage-y += chip.c
ramstage-y += timer.c

endif
