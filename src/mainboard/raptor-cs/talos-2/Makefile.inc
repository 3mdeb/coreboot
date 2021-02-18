## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
bootblock-y += rom_media.c
romstage-y += cbmem.c
romstage-y += romstage.c
<<<<<<< HEAD
romstage-y += rom_media.c
romstage-y += vpd.c
ramstage-y += rom_media.c
=======
romstage-y += istep89.c
romstage-y += istep810.c
romstage-y += ../../../cpu/power/power9/scom.c
>>>>>>> add istep 8.10
