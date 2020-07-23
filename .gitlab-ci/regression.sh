#!/bin/bash

git clone https://gitlab.com/3mdeb/rte/open-firmware-rte.git
cd open-firmware-rte

# legacy or mainline?
if [[ $FIRMWARE_VERSION =~ v4\.0.* ]]; then
    export FIRMWARE="l"
else
    export FIRMWARE="m"
fi

# bash -x ./regression.sh
ls ./regressions.sh
