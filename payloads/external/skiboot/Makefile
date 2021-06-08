## SPDX-License-Identifier: GPL-2.0-only

config=config.its
dts=../../../$(CONFIG_SKIBOOT_DEVICE_TREE)
fdt=fdt.bin
skiboot_dir=$(CURDIR)/talos-skiboot
skiboot_git_repo=https://git.raptorcs.com/git/talos-skiboot
skiboot=skiboot.lid
uimage=skiboot.uimage

unexport $(COREBOOT_EXPORTS)

.PHONY: all clean distclean

all: $(CURDIR)/$(uimage)

$(CURDIR)/$(uimage): $(skiboot_dir)/$(skiboot) $(CURDIR)/$(fdt)
	mkimage -f $(config) $(uimage)

$(CURDIR)/$(fdt): $(dts)
	dtc -I dts -O dtb -o $(fdt) $(dts)

$(skiboot_dir)/$(skiboot): $(skiboot_dir)
	$(MAKE) -C $(skiboot_dir) -j`nproc` CROSS=powerpc64-linux-gnu- DEBUG=1

$(skiboot_dir):
	git clone $(skiboot_git_repo) $(skiboot_dir)
	cd $(skiboot_dir) && git checkout $(CONFIG_SKIBOOT_REVISION)

distclean: clean

clean:
	rm -rf $(skiboot_dir)
	rm -f $(skiboot)
	rm -f $(uimage)
	rm -f $(fdt)
