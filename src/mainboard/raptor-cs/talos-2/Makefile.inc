## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
bootblock-y += rom_media.c
romstage-y += cbmem.c
romstage-y += romstage.c
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
romstage-y += rom_media.c
romstage-y += vpd.c
ramstage-y += rom_media.c
=======
romstage-y += istep89.c
romstage-y += istep810.c
romstage-y += ../../../cpu/power/power9/scom.c
>>>>>>> add istep 8.10
=======
>>>>>>> move isteps 8.9, 8.10 to soc subdir
=======
romstage-y += ../../../soc/ibm/power9/istep_8_9.c
romstage-y += ../../../soc/ibm/power9/istep_8_10.c
>>>>>>> src/include/cpu/power/istep_8_9.c: remove attributes found in hosboot config files
=======
>>>>>>> src/soc/ibm/power9/istep_8_10.c: Change mask values to PPC_BITMASK() and PPC_BIT()
