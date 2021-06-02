## SPDX-License-Identifier: GPL-2.0-only

skiboot_dir=$(CURDIR)/talos-skiboot
skiboot_git_repo=https://git.raptorcs.com/git/talos-skiboot
skiboot_revision=9858186353f2203fe477f316964e03609d12fd1d

unexport $(COREBOOT_EXPORTS)

all: $(skiboot_dir)
	cd $(skiboot_dir) && $(MAKE) -j`nproc` CROSS=powerpc64-linux-gnu- DEBUG=1
#	$(error BUILDING SKIBOOT)

clean:
	rm -rf $(skiboot_dir)
	#$(MAKE) -C $(skiboot_dir) CROSS=powerpc64-linux-gnu- distclean

distclean:
	rm -rf $(skiboot_dir)

$(skiboot_dir):
	echo "    Cloning $(project_name) from Git"
	git clone $(skiboot_git_repo) $(skiboot_dir)
	echo "    Checking out $(skiboot_revision)"
	cd $(skiboot_dir) && git checkout $(skiboot_revision)


