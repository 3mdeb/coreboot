/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_OCC_H
#define CPU_PPC64_OCC_H

void writeOCCSRAM(uint32_t address, uint64_t *buffer, size_t data_length);
void readOCCSRAM(uint32_t address, uint64_t *buffer, size_t data_length);
void write_occ_command(uint64_t write_data);
void clear_occ_special_wakeups(void);
void occ_start_from_mem(void);

#endif /* CPU_PPC64_OCC_H */
