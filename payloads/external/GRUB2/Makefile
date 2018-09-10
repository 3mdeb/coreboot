TAG-$(CONFIG_GRUB2_MASTER)=
TAG-$(CONFIG_GRUB2_REVISION)=$(CONFIG_GRUB2_REVISION_ID)
TAG-$(CONFIG_GRUB2_STABLE)=e54c99aaff5e5f6f5d3b06028506c57e66d8ef77
NAME-$(CONFIG_GRUB2_MASTER)=HEAD
NAME-$(CONFIG_GRUB2_REVISION)=$(CONFIG_GRUB2_REVISION_ID)
NAME-$(CONFIG_GRUB2_STABLE)=2.02

project_git_repo=https://github.com/3mdeb/grub.git
project_dir=grub2

unexport HOSTCC CC LD OBJCOPY STRIP
MAKEOVERRIDES :=

all: grub2

checkout:
	echo "    GIT        GRUB2 $(NAME-y)"
	test -d grub2 || \
		git clone $(project_git_repo) $(project_dir)
	cd grub2 && \
		git checkout master && \
		git pull; \
		test -n "$(TAG-y)" && \
			git branch -f coreboot $(TAG-y) && \
			git checkout coreboot || true

grub2/build/config.h: $(CONFIG_DEP) | checkout
	echo "    CONFIG     GRUB2 $(NAME-y)"
	rm -rf grub2/build
	mkdir -p grub2/build/grub-core
	cd grub2 && ./autogen.sh
	cd grub2/build && ../configure CC="$(HOSTCC)" LD="$(LD)" \
	FREETYPE="pkg-config freetype2" \
	BUILD_FREETYPE="pkg-config freetype2" TARGET_CC="$(CC)" \
	TARGET_OBJCOPY="$(OBJCOPY)" TARGET_STRIP="$(STRIP)" \
	CFLAGS=-O2 TARGET_CFLAGS=-Os \
	--with-platform=coreboot --enable-boot-time --disable-werror \
	--disable-device-mapper


config: grub2/build/config.h checkout

config_bits:  $(CONFIG_DEP) | checkout
	echo "    CONFIG     GRUB2 $(NAME-y)"
	rm -rf grub2/build
	mkdir -p grub2/build/grub-core
	ln -sf $(BITS_ROOT)/build/grub/grub-core/contrib-deps grub2/grub-core/contrib-deps
	ln -sf $(BITS_ROOT)/build/bits/boot grub2/build/boot_memdisk
	cd grub2 && GRUB_CONTRIB=$(BITS_ROOT)/rc ./autogen.sh
	cd grub2/build && ../configure CC="$(HOSTCC)" LD="$(LD)" \
	FREETYPE="pkg-config freetype2" \
	BUILD_FREETYPE="pkg-config freetype2" TARGET_CC="$(CC)" \
	TARGET_OBJCOPY="$(OBJCOPY)" TARGET_STRIP="$(STRIP)" \
	CFLAGS=-O2 TARGET_CFLAGS=-Os \
	--target=i386 --with-platform=coreboot --enable-boot-time \
	--disable-werror --disable-device-mapper --disable-nls \
	--disable-grub-mkfont --disable-grub-mount \
	--disable-libzfs --disable-efiemu

grub2: config
	echo "    MAKE       GRUB2 $(NAME-y)"
	$(MAKE) -C grub2/build
	$(MAKE) -C grub2/build default_payload.elf \
		EXTRA_PAYLOAD_MODULES="$(CONFIG_GRUB2_EXTRA_MODULES)"

grub_bits: config_bits
	echo "    MAKE       GRUB2 $(NAME-y)"
	$(MAKE) -C grub2/build
	$(MAKE) -C grub2/build bits_payload.elf

clean:
	test -f grub2/build/Makefile && $(MAKE) -C grub2/build clean || exit 0

distclean:
	rm -rf grub2

print-repo-info:
	echo "$(project_git_repo) $(project_dir)"

.PHONY: checkout config grub2 clean distclean print-repo-info
