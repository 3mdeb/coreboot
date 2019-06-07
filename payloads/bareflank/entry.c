/*
 * Bareflank Hypervisor
 * Copyright (C) 2015 Assured Information Security, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <vmm.h>	// TODO: read file from CBFS?
#include <common.h>

#include <bfdebug.h>
#include <bftypes.h>
#include <bfconstants.h>
#include <bfdriverinterface.h>

#include <libpayload.h>
#include <cbfs.h>
#include <lzma.h>

extern void i386_do_exec(void* addr, int argc, char **argv, int *ret);

/* -------------------------------------------------------------------------- */
/* Global                                                                     */
/* -------------------------------------------------------------------------- */

uint64_t g_vcpuid = 0;

struct pmodule_t {
    const char *data;
    uint64_t size;
};

uint64_t g_num_pmodules = 0;
struct pmodule_t pmodules[MAX_NUM_MODULES] = {{0}};

/* -------------------------------------------------------------------------- */
/* Misc Device                                                                */
/* -------------------------------------------------------------------------- */

static int64_t
ioctl_add_module(const char *file, uint64_t len)
{
    char *buf;
    int64_t ret;

    if (g_num_pmodules >= MAX_NUM_MODULES) {
        BFALERT("IOCTL_ADD_MODULE: too many modules have been loaded\n");
        return BF_IOCTL_FAILURE;
    }

    buf = platform_alloc_rw(len);
    if (buf == NULL) {
        BFALERT("IOCTL_ADD_MODULE: failed to allocate memory for the module\n");
        return BF_IOCTL_FAILURE;
    }

    platform_memcpy(buf, file, len);

    ret = common_add_module(buf, len);
    if (ret != BF_SUCCESS) {
        BFALERT("IOCTL_ADD_MODULE: common_add_module failed: %p - %s\n", (void *)ret, ec_to_str(ret));
        goto failed;
    }

    pmodules[g_num_pmodules].data = buf;
    pmodules[g_num_pmodules].size = len;

    g_num_pmodules++;

    return BF_IOCTL_SUCCESS;

failed:

    platform_free_rw(buf, len);

    BFALERT("IOCTL_ADD_MODULE: failed\n");
    return BF_IOCTL_FAILURE;
}

static long
ioctl_load_vmm(void)
{
    int64_t ret;

    ret = common_load_vmm();
    if (ret != BF_SUCCESS) {
        BFALERT("IOCTL_LOAD_VMM: common_load_vmm failed: %p - %s\n", (void *)ret, ec_to_str(ret));
        goto failure;
    }

    return BF_IOCTL_SUCCESS;

failure:

    BFDEBUG("IOCTL_LOAD_VMM: failed\n");
    return BF_IOCTL_FAILURE;
}

static long
ioctl_start_vmm(void)
{
    int64_t ret;

    ret = common_start_vmm();
    if (ret != BF_SUCCESS) {
        BFALERT("IOCTL_START_VMM: common_start_vmm failed: %p - %s\n", (void *)ret, ec_to_str(ret));
        goto failure;
    }

    return BF_IOCTL_SUCCESS;

failure:

    BFDEBUG("IOCTL_START_VMM: failed\n");
    return BF_IOCTL_FAILURE;
}

static void cbfs_run_payload(struct cbfs_payload *pay)
{
    struct cbfs_payload_segment *seg = &pay->segments;
    for (;;) {
        void *src = (void*)pay + ntohl(seg->offset);
        void *dest = (void*)ntohll(seg->load_addr);
        u32 src_len = ntohl(seg->len);
        u32 dest_len = ntohl(seg->mem_len);
        switch (seg->type) {
        case PAYLOAD_SEGMENT_BSS:
            printf("BSS segment %d@%p\n", dest_len, dest);
            memset(dest, 0, dest_len);
            break;
        case PAYLOAD_SEGMENT_ENTRY: {
            printf("Calling addr %p\n", dest);
            i386_do_exec(dest, 0, NULL, NULL);
            return;
        }
        default:
            printf("Segment %x %d@%p -> %d@%p\n", seg->type, src_len, src,
                        dest_len, dest);
            if (seg->compression == ntohl(CBFS_COMPRESS_NONE)) {
                if (src_len > dest_len)
                    src_len = dest_len;
                memcpy(dest, src, src_len);
            } else if (CONFIG_LP_LZMA
                       && seg->compression == ntohl(CBFS_COMPRESS_LZMA)) {
                int ret = ulzman(src, src_len, dest, dest_len);
                if (ret < 0)
                    return;
                src_len = ret;
            } else {
                printf("No support for compression type %x\n", seg->compression);
                return;
            }
            if (dest_len > src_len)
                memset(dest + src_len, 0, dest_len - src_len);
            break;
        }
        seg++;
    }
}

/* -------------------------------------------------------------------------- */
/* Entry / Exit                                                               */
/* -------------------------------------------------------------------------- */

int main(void)
{
    void *payload = NULL;

    printf("\n");
    printf("  ___                __ _           _   \n");
    printf(" | _ ) __ _ _ _ ___ / _| |__ _ _ _ | |__\n");
    printf(" | _ \\/ _` | '_/ -_)  _| / _` | ' \\| / /\n");
    printf(" |___/\\__,_|_| \\___|_| |_\\__,_|_||_|_\\_\\\n");
    printf("\n");
    printf(" Please give us a star on: https://github.com/Bareflank/hypervisor\n");
    printf("\n");

    if (common_init() != BF_SUCCESS) {
        return -1;
    }

    ioctl_add_module((char *)vmm, vmm_len);
    ioctl_load_vmm();
    ioctl_start_vmm();

    payload = cbfs_load_payload(CBFS_DEFAULT_MEDIA, CONFIG_CBFS_PREFIX "/realpayload");

    cbfs_run_payload(payload);

    printf("Should not get here...\n");

    return 0;
}
