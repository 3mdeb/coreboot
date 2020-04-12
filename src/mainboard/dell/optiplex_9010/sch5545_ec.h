/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2019-2020 3mdeb Embedded Systems Consulting
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdint.h>

#define READ_OP					0
#define WRITE_OP				1

#define EC_HWM_LDN				1
#define EC_GPIO_LDN				2
#define   EC_GPIO_PP			(0 << 0)
#define   EC_GPIO_OD			(1 << 0)
#define   EC_GPIO_FUNC0				(0 << 4)
#define   EC_GPIO_FUNC1				(1 << 4)
#define   EC_GPIO_FUNC2				(2 << 4)
#define   EC_GPIO_FUNC3				(3 << 4)

uint16_t sch5545_get_ec_fw_version(void);
void sch5545_update_ec_firmware(uint16_t ec_version);
void sch5545_ec_early_init(void);
void sch5545_ec_hwm_early_init(void);
void sch5545_ec_update_temperature_tables(void);
void sch5545_ec_hwm_init(void *unused);
