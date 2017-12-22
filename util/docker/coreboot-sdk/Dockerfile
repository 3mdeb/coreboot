# This dockerfile is not meant to be used directly by docker.  The
# {{}} variables are replaced with values by the makefile.  Please generate
# the docker image for this file by running:
#
#   make coreboot-sdk
#
# Variables can be updated on the make command line or left blank to use
# the default values set by the makefile.
#
#  SDK_VERSION is used to name the version of the coreboot sdk to use.
#              Typically, this corresponds to the toolchain version.  This
#              is used to identify this docker image.
#  DOCKER_COMMIT is the coreboot Commit-ID to build the toolchain from.

FROM debian:stable
MAINTAINER Martin Roth <martin@coreboot.org>

RUN \
	apt-get -qqy install debian-archive-keyring && \
	apt update && apt -y upgrade
RUN \
	apt-get -qq update && \
	apt-get -qqy install \
		bc \
		bison \
		bzip2 \
		ccache \
		cmake \
		curl \
		dh-autoreconf \
		diffutils \
		doxygen \
		flex \
		flashrom  \
		g++ \
		gawk \
		gcc \
		git \
		gnat-6 \
		libelf-dev \
		libfreetype6-dev \
		libftdi-dev \
		libglib2.0-dev \
		libgmp-dev \
		libgmp-dev \
		libisl-dev \
		liblzma-dev \
		libncurses5-dev \
		libpci-dev \
		libreadline-dev \
		libssl1.0-dev \
		libusb-1.0-0-dev \
		libusb-dev \
		libxml2-dev \
		libyaml-dev \
		m4 \
		make \
		nasm \
		openssl \
		p7zip-full \
		patch \
		pkg-config \
		python \
		qemu \
		screen \
		shellcheck \
		socat \
		subversion \
		sudo \
		texlive-latex-base \
		unifont \
		unrar-free \
		uuid-dev \
		vim-common \
		wget \
		xz-utils \
		zlib1g-dev \
	&& apt-get clean

RUN useradd -m coreboot && echo "coreboot:coreboot" | chpasswd && adduser coreboot sudo

RUN \
	cd /root && \
	git clone https://review.coreboot.org/coreboot && \
	cd coreboot/util/crossgcc && \
	git checkout {{DOCKER_COMMIT}} && \
	make {{CROSSGCC_PARAM}} \
		BUILD_LANGUAGES=c,ada CPUS=$(nproc) DEST=/opt/xgcc && \
	cd /root/coreboot && \
	make iasl CPUS=$(nproc) DEST=/opt/xgcc && \
	cd /root && \
	rm -rf coreboot

RUN mkdir /home/coreboot/.ccache && \
	chown coreboot:coreboot /home/coreboot/.ccache && \
	mkdir /home/coreboot/cb_build && \
	chown coreboot:coreboot /home/coreboot/cb_build && \
	echo "export PATH=$PATH:/opt/xgcc/bin" >> /home/coreboot/.bashrc && \
	echo "export SDK_VERSION={{SDK_VERSION}}" >> /home/coreboot/.bashrc && \
	echo "export SDK_COMMIT={{DOCKER_COMMIT}}" >> /home/coreboot/.bashrc

RUN wget https://codeload.github.com/IntelFsp/FSP/zip/BayTrail -O /tmp/FSP-BayTrail.zip && \
	mkdir -p /home/coreboot/intel/fsp/baytrail && \
	chown -R coreboot:coreboot /home/coreboot/intel && \
	cd /home/coreboot/intel/fsp/baytrail && \
	7z e /tmp/FSP-BayTrail.zip && \
	rm -rf /tmp/FSP-BayTrail.zip 

RUN wget https://cloud.3mdeb.com/index.php/s/0z5R4zMp605s7WK/download -O /tmp/ucode_baytrail.tar.gz && \
	mkdir -p /home/coreboot/intel/cpu/baytrail/microcode && \
	chown -R coreboot:coreboot /home/coreboot/intel/cpu && \
	cd /home/coreboot/intel/cpu/baytrail/microcode && \
	tar xvf /tmp/ucode_baytrail.tar.gz && \
	rm -rf /tmp/ucode_baytrail.tar.gz

VOLUME /home/coreboot/.ccache

ENV PATH $PATH:/opt/xgcc/bin
ENV SDK_VERSION={{SDK_VERSION}}
ENV SDK_COMMIT={{DOCKER_COMMIT}}
USER coreboot
