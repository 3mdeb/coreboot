FROM coreboot/coreboot-sdk:1.52

MAINTAINER Piotr Król <piotr.krol@3mdeb.com>

USER root

RUN \
	apt-get -qq update && \
	apt-get -qqy install \
		zip \
		libc6-dev \
	&& apt-get clean

USER coreboot
