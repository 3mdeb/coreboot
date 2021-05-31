## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
bootblock-y += rom_media.c
romstage-y += cbmem.c
romstage-y += romstage.c
romstage-y += rom_media.c
romstage-y += vpd.c
ramstage-y += rom_media.c

TALOS_DTB = talos-2.dtb
TALOS_DTS = talos-2.dts
MB_DIR = src/mainboard/raptor-cs/talos-2
$(obj)/$(TALOS_DTB):
	dtc -I dts -O dtb $(MB_DIR)/$(TALOS_DTS) -o $(obj)/$(TALOS_DTB)

# dtc -I dts -O dtb device-tree.dts -o device-tree-blob
cbfs-files-y += $(TALOS_DTB)
$(TALOS_DTB)-file := $(obj)/$(TALOS_DTB)
$(TALOS_DTB)-type := raw
