## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
bootblock-y += rom_media.c
romstage-y += cbmem.c
romstage-y += romstage.c
romstage-y += rom_media.c
romstage-y += vpd.c
ramstage-y += rom_media.c

LINUX_DTB = talos-ii.dtb
$(obj)/$(LINUX_DTB):
	dtc -p 4096 -I dts -O dtb -o $(obj)/$(LINUX_DTB) -i $(MB_DIR) $(MB_DIR)/$(patsubst %.dtb,%.dts,$(LINUX_DTB))
# dtc -I dts -O dtb device-tree.dts -o device-tree-blob

cbfs-files-y += $(LINUX_DTB)
$(LINUX_DTB)-file := $(obj)/$(LINUX_DTB)
$(LINUX_DTB)-type := raw
