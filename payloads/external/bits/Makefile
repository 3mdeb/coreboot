TAG-$(CONFIG_BITS_MASTER)=origin/master
TAG-$(CONFIG_BITS_STABLE)=19da7046a7303f1de8b53165eea1a6f486757c03
TAG-$(CONFIG_BITS_REVISION)=$(CONFIG_BITS_REVISION_ID)

project_git_repo=https://github.com/3mdeb/bits.git
project_dir=$(pwd)/bits

MAKEOVERRIDES :=


all: build

bits:
	echo "    Cloning BITS from Git"
	git clone $(project_git_repo) $(project_dir)
	git clone https://github.com/3mdeb/grub.git $(project_dir)/deps/grub

fetch: bits
ifeq ($(TAG-y),)
	echo "Error: The specified tag is invalid"
ifeq ($(CONFIG_BITS_REVISION),y)
	echo "Error: There is no revision specified for BITS"
	false
endif
	false
endif
	cd bits; git show $(TAG-y) >/dev/null 2>&1 ;	if [ $$? -ne 0 ] || \
		[ "$(TAG-y)" = "origin/master" ]; then \
		echo "    Fetching new commits from the BITS git repo"; \
		git fetch; fi

checkout: fetch
	echo "    Checking out BITS revision $(TAG-y)"
	cd bits; git checkout master; git branch -D coreboot 2>/dev/null; \
	git checkout -b coreboot $(TAG-y); git submodule update --init --checkout;
	cd bits/deps/grub && git checkout master && \
	git branch -f coreboot origin/grub_for_bits && git checkout coreboot

build: checkout
	echo "    MAKE       BITS $(TAG-y)"
	$(MAKE) -C bits bits_grub_env MAKEFLAGS="w"
	mkdir -p bits/deps/grub/build/grub-core
	cd bits/deps/grub && ln -sf $(CURDIR)/bits/build/grub/grub-core/contrib-deps grub-core/contrib-deps
	cd bits/deps/grub && ln -sf $(CURDIR)/bits/build/bits/boot build/boot_memdisk
	cd bits/deps/grub && GRUB_CONTRIB=$(CURDIR)/bits/rc ./autogen.sh
	cd bits/deps/grub && ./bits-build.sh
	$(MAKE) -C bits/depc/grub/build bits_payload.elf

clean:
	test -d bits/build && rm -rf bits/build || exit 0

distclean:
	rm -rf bits

print-repo-info:
	echo "$(project_git_repo) $(project_dir)"

.PHONY: checkout config build clean distclean clone fetch print-repo-info
