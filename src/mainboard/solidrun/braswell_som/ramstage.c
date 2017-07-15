/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Intel Corporation
 * Copyright (C) 2017 Andreas Galauner <andreas@galauner.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <soc/ramstage.h>
#include <boardid.h>
#include "onboard.h"

// We configure a few video-related pins using the FSP here, because otherwise Video Init in the FSP seems to fail
// All other GPIOs are later configured by coreboot routines as defined in gpio.c

/* N71:HV_DDI0_DDC_SCL    >>  0        2        0         00          0xFED8D458    0x00920301     0xFED8D45C    0x05C00000
   N66:HV_DDI0_DDC_SDA    >>  0        2        0         00          0xFED8D430    0x00920301     0xFED8D434    0x05C00000
   N61:HV_DDI0_HPD        >>  0        1        0         00          0xFED8D408    0x00110300     0xFED8D40C    0x05C00020
   N64:HV_DDI1_HPD        >>  0        1        0         00          0xFED8D420    0x00110301     0xFED8D424    0x05C00020
   N67:HV_DDI2_DDC_SCL    >>  0        1        0         00          0xFED8D438    0x00910301     0xFED8D43C    0x04C00000
   N62:HV_DDI2_DDC_SDA    >>  0        1        0         00          0xFED8D410    0x00910301     0xFED8D414    0x04C00000
   N68:HV_DDI2_HPD        >>  0        1        0         00          0xFED8D440    0x00110301     0xFED8D444    0x05C00020
   N65:PANEL0_BKLTCTL     >>  0        1        0         00          0xFED8D428    0x00010300     0xFED8D42C    0x05C00000
   N60:PANEL0_BKLTEN      >>  0        1        0         00          0xFED8D400    0x00010300     0xFED8D404    0x05C00000
   N72:PANEL0_VDDEN       >>  0        1        0         00          0xFED8D460    0x00010300     0xFED8D464    0x05C00000
   N63:PANEL1_BKLTCTL     >>  0        1        0         00          0xFED8D418    0x00010300     0xFED8D41C    0x05C00000
   N70:PANEL1_BKLTEN      >>  0        1        0         00          0xFED8D450    0x00010300     0xFED8D454    0x05C00000
   N69:PANEL1_VDDEN       >>  0        1        0         00          0xFED8D448    0x00010300     0xFED8D44C    0x05C00000*/

#define NORTH 0x01

static BL_GPIO_PAD_INIT video_gpio_init_table[] = {
    { .Name = NULL /*L"N71:HV_DDI0_DDC_SCL"*/,   .Confg0 = 0x00920301, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00000, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5458, .Misc = 0 },
    { .Name = NULL /*L"N66:HV_DDI0_DDC_SDA"*/,   .Confg0 = 0x00920301, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00000, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5430, .Misc = 0 },
    { .Name = NULL /*L"N61:HV_DDI0_HPD"*/,       .Confg0 = 0x00110300, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00020, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5408, .Misc = 0 },
    { .Name = NULL /*L"N64:HV_DDI1_HPD"*/,       .Confg0 = 0x00110301, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00020, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5420, .Misc = 0 },
    { .Name = NULL /*L"N67:HV_DDI2_DDC_SCL"*/,   .Confg0 = 0x00910301, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x04C00000, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5438, .Misc = 0 },
    { .Name = NULL /*L"N62:HV_DDI2_DDC_SDA"*/,   .Confg0 = 0x00910301, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x04C00000, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5410, .Misc = 0 },
    { .Name = NULL /*L"N68:HV_DDI2_HPD"*/,       .Confg0 = 0x00110301, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00020, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5440, .Misc = 0 },
    { .Name = NULL /*L"N65:PANEL0_BKLTCTL"*/,    .Confg0 = 0x00010300, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00000, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5428, .Misc = 0 },
    { .Name = NULL /*L"N60:PANEL0_BKLTEN"*/,     .Confg0 = 0x00010300, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00000, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5400, .Misc = 0 },
    { .Name = NULL /*L"N72:PANEL0_VDDEN"*/,      .Confg0 = 0x00010300, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00000, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5460, .Misc = 0 },
    { .Name = NULL /*L"N63:PANEL1_BKLTCTL"*/,    .Confg0 = 0x00010300, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00000, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5418, .Misc = 0 },
    { .Name = NULL /*L"N70:PANEL1_BKLTEN"*/,     .Confg0 = 0x00010300, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00000, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5450, .Misc = 0 },
    { .Name = NULL /*L"N69:PANEL1_VDDEN"*/,      .Confg0 = 0x00010300, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0x05C00000, .Confg1Changes = 0xFFFFFFFF, .Community = NORTH, .MmioAddr = 0x5448, .Misc = 0 },
    { .Name = NULL/*L""*/,                       .Confg0 = 0xFFFFFFFF, .Confg0Changes = 0xFFFFFFFF, .Confg1 = 0xFFFFFFFF, .Confg1Changes = 0xFFFFFFFF, .Community = 0xFFFFFFFF, .MmioAddr = 0xFFFFFFFF, .Misc = 0xFFFFFFFF },
};

void mainboard_silicon_init_params(SILICON_INIT_UPD *params)
{
	params->ChvSvidConfig = SVID_PMIC_CONFIG;
	params->PMIC_I2CBus = BCRD2_PMIC_I2C_BUS;

    //enable turbo mode
    params->PcdTurboMode = 1;

    //params->GpioFamilyInitTablePtr = ;
    params->GpioPadInitTablePtr = video_gpio_init_table;
}
