## SPDX-License-Identifier: GPL-2.0-only

skiboot_dir=$(CURDIR)/talos-skiboot
skiboot_git_repo=https://git.raptorcs.com/git/talos-skiboot
skiboot_revision=9858186353f2203fe477f316964e03609d12fd1d
dts=talos_ii_device-tree_stripped.dts
skiboot=skiboot.lid
uimage=skiboot.uimage
config=config.its

unexport $(COREBOOT_EXPORTS)

all: $(CURDIR)/$(uimage)

$(CURDIR)/$(uimage): $(skiboot_dir)/$(skiboot) $(CURDIR)/fdt.bin
	cp $(skiboot_dir)/$(skiboot) $(CURDIR)/$(skiboot)
	mkimage -f $(config) $(uimage)

$(CURDIR)/fdt.bin:
	dtc -I dts -O dtb -o fdt.bin $(dts)

$(skiboot_dir)/$(skiboot): $(skiboot_dir)
	$(MAKE) -C $(skiboot_dir) -j`nproc` CROSS=powerpc64-linux-gnu- DEBUG=1

clean:
	rm -rf $(skiboot_dir)
	# $(MAKE) -C $(skiboot_dir) CROSS=powerpc64-linux-gnu- distclean
	rm $(skiboot)
	rm $(uimage)

distclean:
	rm -rf $(skiboot_dir)
	rm $(skiboot)
	rm $(uimage)

$(skiboot_dir):
	git clone $(skiboot_git_repo) $(skiboot_dir)
	cd $(skiboot_dir) && git checkout $(skiboot_revision)
