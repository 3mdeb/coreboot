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

#include <cf9_reset.h>
#include <string.h>
#include <arch/io.h>
#include <cpu/x86/msr.h>
#include <console/console.h>
#include <cpu/intel/model_206ax/model_206ax.h>
#include <southbridge/intel/common/gpio.h>
#include <superio/smsc/sch5545/sch5545.h>
#include <superio/smsc/sch5545/sch5545_emi.h>

#include "sch5545_ec.h"

#define TCC_TEMPERATURE				95

#define GPIO_CHASSIS_ID0			1
#define GPIO_CHASSIS_ID1			17
#define GPIO_CHASSIS_ID2			37
#define GPIO_FRONT_PANEL_CHASSIS_DET_L		70

static uint16_t emi_bar;

enum {
	TDP_16 = 0x10,
	TDP_32 = 0x20,
	TDP_COMMON = 0xff,
};

struct ec_val_reg {
	uint8_t val;
	uint16_t reg;
};

typedef struct ec_val_reg_tdp {
	uint8_t val;
	uint16_t reg;
	uint8_t tdp;
} ec_chassis_tdp_t;

static const uint32_t ec_fw[] = {
	0x43534d53, 0x4d494255, 0x00183c00, 0x00020003,
	0x00062d3c, 0x0000174d, 0x31bb6ee5, 0x00000000,
	0x35484353, 0x5f353435, 0x4d494255, 0x5f32305f,
	0x30304333, 0x3030305f, 0x00000033, 0x79706f43,
	0x68676972, 0x63282074, 0x30322029, 0x53203930,
	0x646e6174, 0x20647261, 0x7263694d, 0x7953206f,
	0x6d657473, 0x6f432073, 0x726f7072, 0x6f697461,
	0x202d206e, 0x43534d53, 0x0000002e, 0x080ac009,
	0x00120011, 0x7f207f19, 0x0000ff0f, 0x00050201,
	0x0c0b0403, 0x00000006, 0x01020202, 0xff0802ff,
	0x04040404, 0x00000004, 0x880c882d, 0x6038b908,
	0xb8617810, 0x78107fe0, 0xd116c0f1, 0x00801185,
	0x11dfe826, 0x080d8080, 0xd800001f, 0x00021985,
	0x00801185, 0x0051081d, 0x00801187, 0x780fb861,
	0x00021987, 0x00801187, 0x082ee892, 0xf0100000,
	0x00801188, 0x780fb861, 0x00021988, 0x00801188,
	0x11fbe886, 0x19888080, 0xf1f10002, 0x7ee0c0d1,
	0x008002fc, 0xd325c0f1, 0x00801385, 0x0050083b,
	0x00910885, 0x00801384, 0x000008e2, 0x8920d120,
	0x00de090b, 0x8080135a, 0x080fb884, 0xd11d0410,
	0x08418922, 0x13890041, 0x09390081, 0xd900001f,
	0x00421b85, 0xd016f01a, 0x08118800, 0x135a00de,
	0xe0808080, 0xf207d800, 0x808013fb, 0x00021b88,
	0x1b85d802, 0xd00f0002, 0xa822d910, 0x7ee0c0d1,
	0x06f30809, 0xa900d10b, 0xa902d10a, 0xe80c8901,
	0x808013fa, 0x00021b87, 0x1b85d801, 0xd8000002,
	0xf1eea901, 0x78e0f1ec, 0x008002fc, 0x008005a7,
	0x00803c19, 0x24867408, 0xf21e92fe, 0x8920d10f,
	0x21867904, 0xf4088aff, 0x8920d10d, 0x20867824,
	0xf21287fe, 0x1185d10b, 0xda010080, 0x0091080b,
	0x7fe0d009, 0x11faa841, 0x19878080, 0x19850002,
	0x7ee00082, 0x78e07ee0, 0x008005a7, 0x008005a8,
	0x008002fc, 0x00803c19, 0xd900e080, 0x7408f6d8,
	0x93fc2486, 0xf414d9ff, 0x0f802032, 0xcd2000f0,
	0x2044b844, 0x683b0102, 0x61597a5c, 0x2044792f,
	0x79540082, 0xb8c0792f, 0x792f7916, 0x70287fe0,
	0x7308c0f1, 0x0275089d, 0x2740da00, 0x20337400,
	0x201400ca, 0x78000280, 0x34050505, 0x3939440a,
	0xd0250046, 0x087d6068, 0xf043015e, 0x0191090f,
	0x00600e32, 0xc0d1d801, 0x092b7ee0, 0x09270212,
	0x694f0353, 0x0a0b7a4f, 0xe20100d2, 0x0e167a4f,
	0xd8000060, 0x00510829, 0x8810d018, 0x00820821,
	0x0911f027, 0xd01600d3, 0x08156028, 0xf021015e,
	0x0151093f, 0x881bd013, 0x015f0837, 0xf1ddd800,
	0x8818d010, 0x015e081d, 0x0ddaf013, 0x70480060,
	0x00510811, 0xbbc0d009, 0xe3058810, 0x00c30813,
	0xf1cb7048, 0x00600dbe, 0x08f9d801, 0xd8018051,
	0x78e0f1c3, 0x00800119, 0x008002a4, 0x00800118,
	0x00800100, 0xc5e1c0f1, 0x8907d10d, 0xe001d20d,
	0xa907780f, 0x09138a24, 0xdd010023, 0x0866dd00,
	0x70a90020, 0x8a08f009, 0x8fc3208c, 0x8a08f205,
	0x780fe001, 0x0761aa08, 0x70a9cf2f, 0x00803c04,
	0x00800384, 0xd900d007, 0x804218d9, 0x804218da,
	0x00421890, 0x00421891, 0x00421892, 0x78e07ee0,
	0x008002fc, 0x8803d006, 0x0050080d, 0x00d1080d,
	0x000000dd, 0x000002fd, 0x78e07ee0, 0x00803c04,
	0xc5e1c0f1, 0x8d25d51a, 0xd803e987, 0x0baed906,
	0xda00d56f, 0x8d25f018, 0x792fb961, 0x0819ad25,
	0x8d0a0051, 0x8fc3208c, 0x8d0af20e, 0x780fe001,
	0xf00aad0a, 0x208c8d09, 0xf2068fc3, 0xe0018d09,
	0xad09780f, 0xd800d10b, 0xa903a904, 0x080f8901,
	0xd8040051, 0xd801a905, 0x8d03a906, 0x780fe001,
	0x0e86a914, 0xd913d1ef, 0xcf0f06ad, 0x00800384,
	0x00803c04, 0x0e2ac0f1, 0xd801cf2f, 0x9224d212,
	0xe101d312, 0x8ba67930, 0x0d31b224, 0xaa031063,
	0x9225de00, 0xe101b2c4, 0x79308ba7, 0x10630d0f,
	0x0f62b225, 0xb2c5ffef, 0x8b23f00b, 0xaa34e101,
	0xd1ef0e36, 0xf005d913, 0xd1ef0d9a, 0x0651d813,
	0x78e0cf0f, 0x00803c04, 0x00800384, 0x0dd2c0f1,
	0xd515cf0f, 0xd6158d01, 0x00510823, 0xe88f8d06,
	0x908016d9, 0x7810e040, 0x908216da, 0x7a05b808,
	0xd8037a50, 0xd56f0ac6, 0xd900d906, 0x908016de,
	0x1e8dad26, 0xd8011002, 0xad24ad02, 0x168bad25,
	0xad231080, 0x780fe001, 0x0dcead14, 0xd913d1ef,
	0xcf0f05ed, 0x00803c04, 0x008002fc, 0xd900d00a,
	0xa823a820, 0xb025b024, 0xa824a826, 0xd107a825,
	0xa904d808, 0xa903d815, 0xa906d896, 0xa907d801,
	0x78e07ee0, 0x00803c04, 0x00800384, 0xc5e1c0f1,
	0x00200a9e, 0xe8897508, 0xffcf0e5e, 0xd900d00d,
	0xa825a824, 0xed0ef013, 0x10500d23, 0x10900d1b,
	0x10d10d1b, 0xd007d900, 0x0020096a, 0xf007a827,
	0xffcf0f9e, 0x0e52f003, 0x057dffcf, 0x78e0cf0f,
	0x00803c04, 0xd110c0f1, 0x08398900, 0xd8010050,
	0x8903a900, 0xd30de896, 0x13ddda00, 0xa9428080,
	0x00021b8d, 0x0a3ab144, 0xb1450020, 0x0dfae885,
	0xc0d1ffcf, 0xd8027ee0, 0xd1ef0d0e, 0xf1fbd913,
	0x78e0f1f9, 0x00803c04, 0x008002fc, 0x0cb2c0f1,
	0xd913cf2f, 0xde00d50f, 0x0ceeadc0, 0x70c9d1ef,
	0x00200a62, 0x8d03d813, 0x00900823, 0xb5c4d10a,
	0x808011dd, 0x198db5c5, 0x0dae0002, 0xadc3ffef,
	0xadc5adc4, 0xd1ef0c02, 0x04e5d813, 0x78e0cf0f,
	0x00803c04, 0x008002fc, 0x0c62c0f1, 0x7508cf0f,
	0x09be7648, 0xdf000020, 0x0d7ee888, 0xd025ffcf,
	0xa8e5a8e4, 0xf04370e9, 0x8800d023, 0x00d10811,
	0x8802d022, 0xdd09b8e0, 0x10a125ca, 0x12950d6d,
	0x73002740, 0x034d2033, 0x780078b4, 0x25252b2b,
	0x2e2b2b25, 0xd5170505, 0xe9848d21, 0xe8068d04,
	0x00510915, 0xe8888d05, 0xad03d803, 0xd1ef0bb6,
	0xf01cd813, 0x8817d012, 0x002008aa, 0x2540ad0c,
	0xa6001300, 0x8d12d900, 0xae04ae26, 0xad278d13,
	0xd803ae05, 0x0cc2f00b, 0x0811ffcf, 0xd8020051,
	0x0d32f005, 0xd800ffef, 0x042dd800, 0x78e0cf0f,
	0x00803c04, 0x0080062c, 0x00800384, 0x008002c0,
	0x0baec0f1, 0xd614cf0f, 0x8e18d514, 0xd801b8e0,
	0x002120ca, 0xd42f0db6, 0xe885ad11, 0xffcf0d6a,
	0x8d02f017, 0xd800ad01, 0x083ab504, 0xb5050020,
	0x172c8e17, 0x25407005, 0xad0c1301, 0x8d528d11,
	0x0fd68d73, 0x244ad46f, 0xd8020000, 0x03d1ad03,
	0x78e0cf0f, 0x008002c0, 0x00803c04, 0x00061db8,
	0x8a01d227, 0xd027e894, 0x78348a24, 0xe1018860,
	0xaa6d8801, 0xaa0e792f, 0x090baa24, 0xd80001d1,
	0xd840aa04, 0xd803aa12, 0xaa137fe0, 0x096d8a25,
	0x27400155, 0x20337380, 0x78340041, 0x0b037800,
	0x001f1d12, 0xaa0dd801, 0xd840aa05, 0xd802aa12,
	0xaa137fe0, 0xaa05d802, 0xaa12d881, 0x7fe0d801,
	0xd011aa13, 0x080d8800, 0x8a2d00d0, 0xa835d00f,
	0xaa0dd810, 0xf1ead803, 0xf1efd804, 0x8800d00a,
	0x00d0080b, 0xd0098a2d, 0xd80fa836, 0xd8ffaa0d,
	0xd800aa0e, 0xf1c7aa05, 0x78e07ee0, 0x00803c04,
	0x0006187c, 0x0080062c, 0x008002c0, 0xc5e1c0f1,
	0x8900d115, 0xd315e88c, 0xb144da00, 0x808013dd,
	0x1b8db145, 0xa9430002, 0xf01bd800, 0x8805d010,
	0x00df082d, 0x12ddd20d, 0xdd008080, 0x00021a8d,
	0xb1a5b1a4, 0x128ba9a3, 0xa9a20080, 0x780fe001,
	0x0aa6a914, 0xd913d1ef, 0xf00370a9, 0x02c9d801,
	0x78e0cf0f, 0x00803c04, 0x008002fc, 0x00800100,
	0x05d30813, 0x204ed104, 0x81000782, 0x00802010,
	0x7ee0a100, 0x00800640, 0x0a26c0f1, 0x7608cf0f,
	0x75288802, 0xe08ce003, 0xf46ed803, 0x08d78e0b,
	0x0a020011, 0x70c9d26f, 0x0a7a70c9, 0x71a9d26f,
	0x8812d035, 0x781cda01, 0x00012053, 0x7834d802,
	0xad2bd900, 0xad2cd9ff, 0xad2dd904, 0x8120d12f,
	0x8962ad4e, 0x8960ad6f, 0xad12ad51, 0xbbc1ad53,
	0x80032342, 0x006223ca, 0xbb87bb05, 0x8963ad70,
	0x7b7bad55, 0x8966ad74, 0x8964ad76, 0xad19ad58,
	0xbbc1ad5a, 0x80032342, 0x006223ca, 0xad77bb05,
	0xad5c8967, 0xad7b7b7b, 0xad7d896a, 0xad5f8968,
	0x10021d20, 0x10821d21, 0x2342bbc1, 0x23ca8003,
	0xbb050062, 0x896bad7e, 0x10821d23, 0x1d227b7b,
	0x896e10c2, 0x10c21d24, 0x1d26896c, 0x1d271082,
	0x1d281002, 0xbbc11082, 0x80032342, 0x006223ca,
	0x1d25bb05, 0x890f10c2, 0x1d29781b, 0xd8271002,
	0xd802f007, 0x090271c9, 0x72a9d26f, 0x01a1d809,
	0xad02cf2f, 0x008002c0, 0x00800560, 0xd00ac0f1,
	0x0081104a, 0x00d1091b, 0x88439020, 0xb9c6793d,
	0x793bbaa0, 0xb980a840, 0xd2af0d46, 0xc0d1a823,
	0x78e07ee0, 0x00800b64, 0x08f6c0f1, 0x7608cf0f,
	0x093f7548, 0xd0180051, 0x10010e0d, 0xd2af09fe,
	0xf02870c9, 0xd115d800, 0x10021e4b, 0x10021e4d,
	0x10021e4c, 0x1e4ad802, 0x89141002, 0x10041e4e,
	0x11001650, 0x8915e896, 0x10041e50, 0x09cef012,
	0xd00bd28f, 0x10010e1d, 0x20539604, 0xf20a81be,
	0xb8c6e588, 0x21cad903, 0x0bfe0122, 0xda00d2af,
	0xcf0f00fd, 0x00800b64, 0x008003bc, 0x008009c0,
	0xd900e08d, 0xd406f64a, 0x8a0c609a, 0x8fc3208c,
	0xd104f204, 0x79048925, 0x70287fe0, 0x0006188c,
	0x00800100, 0x084ec0f1, 0xdd10cf0f, 0x0f0ad640,
	0x8e01d36f, 0x8800701a, 0x011f081d, 0xe0018e01,
	0x080d780f, 0xae010432, 0xae01d800, 0x0de1bd61,
	0xf06c9055, 0x0fdad801, 0x703acfaf, 0x8000230a,
	0x8e00f264, 0xb823da00, 0x01c1204f, 0xab29e001,
	0x01012053, 0x0d3270c9, 0xdd020060, 0xab0bd80a,
	0xabaad02c, 0x04421b0c, 0xab4e8804, 0x8e01ab0d,
	0x68e1ab50, 0x17a0abef, 0x24327004, 0x0f0f0001,
	0x10011411, 0xe9a72080, 0xd724f024, 0xef9c670f,
	0x20821001, 0xbac3e180, 0x008121ca, 0xab52abb1,
	0x080dab33, 0xabb40333, 0xab34d903, 0x00002414,
	0xab159008, 0x0f802004, 0xff000000, 0xab16b848,
	0xf00fda0e, 0xf40de781, 0x20801001, 0xe983b8c3,
	0x1b117108, 0xab520442, 0xab34ab13, 0x8e00da0c,
	0x20537168, 0xd8010090, 0x0f5a7308, 0x240acfaf,
	0x8e400400, 0x20412040, 0x01fc2286, 0xae207945,
	0x00510809, 0x14421e02, 0xcecf07ad, 0x008005a0,
	0x008002dc, 0x00803c20, 0x0000e198, 0xda04c0f1,
	0x0450090f, 0xa82cd903, 0xc0d17048, 0x886c7ee0,
	0x23cce380, 0xd98380a2, 0x8830f5f7, 0x8c43218c,
	0x882ff40b, 0x8c03218c, 0x882ef407, 0x8fc3218c,
	0x882df403, 0xd984e904, 0xf00ea82c, 0xa84cda00,
	0xd93eeb88, 0xe00ea82d, 0xcf2f082e, 0xf012d91f,
	0x00900b09, 0xf1dbd804, 0xa82dd930, 0xa82ed904,
	0xa82fd903, 0xa831a850, 0x080ee012, 0xd91bcf2f,
	0xf1cdd824, 0xc5e1c0f1, 0xd9007508, 0x20f4d024,
	0x0d130340, 0xd0231111, 0x0c4a880f, 0x7108d14f,
	0xe58ff03b, 0x000a0072, 0xbb0edb03, 0x00c22004,
	0xf2337270, 0xeaa0b8ca, 0x9160d11b, 0x7bbad21b,
	0x0b296219, 0x8960001f, 0x808111ff, 0x6179bb08,
	0x63abd317, 0x0b357930, 0xe58c0051, 0x0f8120ca,
	0x02280000, 0xf0126209, 0x89218900, 0x6119b808,
	0xf00c7930, 0x000072d7, 0xf4084000, 0x00f070c7,
	0x0b2ac400, 0x7108d28f, 0x60a8d009, 0x21d3e081,
	0x29c00021, 0x06b10221, 0x7028ceef, 0x0000e1a8,
	0x008001fc, 0x008005a4, 0x00800100, 0x0000e198,
	0x0e1ac0f1, 0x7708cecf, 0xd36f0ad2, 0x8d237528,
	0x780fb848, 0x7030e905, 0x11ab26ca, 0x8d24f693,
	0x7030e905, 0x116b26ca, 0x8d25f68d, 0x7030e905,
	0x126926ca, 0x8d26f607, 0x7030e904, 0xf603de08,
	0x70e9de01, 0xd3af099e, 0x8d0071a9, 0x80fe2053,
	0x06357ec0, 0x70c9ceef, 0x0dbec0f1, 0x701acecf,
	0xd1327628, 0xd36f0a76, 0xb848610d, 0x0d9d7a0f,
	0x8e031011, 0x7210e805, 0x11ab25ca, 0x8e24f693,
	0x7230e905, 0x116b25ca, 0x8e25f68d, 0x7230e905,
	0x126925ca, 0x8e26f607, 0x7230e904, 0xf603dd08,
	0x8e22dd01, 0x8e61e936, 0xbb65bbc3, 0x01550b65,
	0x730f2740, 0x10c32733, 0x7f007f74, 0x0c2a1e15,
	0x8e050003, 0x0d4de826, 0x78221211, 0x25ca7210,
	0xf0201269, 0xe81e8e06, 0x10510d39, 0x72107822,
	0x122925ca, 0x8e04f016, 0x0d29e814, 0x60381051,
	0x25ca7210, 0xf00e116b, 0x0d19e80c, 0x60381151,
	0x25ca7210, 0xf00611ab, 0xdd01e281, 0x10a225ca,
	0x08d2700a, 0x71c9d3af, 0x20538e00, 0x7dc080fe,
	0xceef055d, 0x78e070a9, 0x0000e198, 0x0cdec0f1,
	0x7608cecf, 0x0fce7528, 0x204aceef, 0x77082100,
	0x13d00d0d, 0xae0cd803, 0xf047700a, 0x13d30f0b,
	0x60e8d023, 0xd880e805, 0xd804ae0c, 0x212ff03d,
	0x0b7623c7, 0x702ad36f, 0x09727508, 0x702ad36f,
	0x1502701a, 0x224a1093, 0x1d022000, 0x702a1482,
	0xd36f09ea, 0x8d6071a9, 0x14c21d02, 0xbbc38d21,
	0x14821e0c, 0x00c22153, 0x01042942, 0xd903e789,
	0x00aa21ca, 0xae6eae2d, 0xae108d20, 0x11021e11,
	0xb926ae52, 0xae2fb9c0, 0x14021e13, 0x2f812004,
	0xff000000, 0xae34b948, 0xd90c8d41, 0xbac3b804,
	0xad017845, 0x04917028, 0x78e0cecf, 0x0000e198,
	0x0c32c0f1, 0x7508cecf, 0x0f1a7628, 0x204aceef,
	0x77082100, 0x14100e0d, 0xad0cd803, 0xf03b700a,
	0x14130f11, 0x13d30f15, 0x60e8d01c, 0x0050080d,
	0xad0cd880, 0xf02fd804, 0x0abe78ef, 0x701ad36f,
	0x88007608, 0x00d12053, 0x13d10f19, 0x214cd014,
	0x8800a000, 0x8e2278c0, 0xad308e41, 0xf012ae02,
	0x092a700a, 0x71c9d36f, 0x21538e21, 0x218600c2,
	0xb94403fc, 0x8e61ad30, 0xbbc36834, 0xae217965,
	0xad2cd900, 0xad2dd901, 0x14421d0e, 0xad51ad0f,
	0x0405d809, 0x78e0cecf, 0x0000e198, 0x00803c19,
	0x090fda04, 0xd9030310, 0x7048a82c, 0xda00f00d,
	0xd905a84c, 0xa84ea82d, 0xa850a84f, 0xa852a851,
	0xa854a853, 0x7ee0d80c, 0x090fda04, 0xd9030490,
	0x7048a82c, 0x8830f023, 0xd981e181, 0x8831f5fa,
	0x21cce180, 0xd98380a2, 0xda00f5f4, 0xa84da84c,
	0xa84fa84e, 0xd905a850, 0xa852a831, 0xa833d9ff,
	0xa834d9f0, 0xa835d9f1, 0xa836d9c6, 0xa837d9c8,
	0xa838d9da, 0xa839d941, 0x7ee0d811, 0x0b22c0f1,
	0xd221cecf, 0xde008a40, 0x00510a09, 0x01540809,
	0xf03870c9, 0x73022740, 0x00002233, 0x7a007a14,
	0xf81e1e03, 0x890a00f8, 0x01422053, 0x8ffc2086,
	0xb8a78909, 0xf410a909, 0x7028ea07, 0x00900a13,
	0xd34f0b8e, 0x0b9ef01e, 0x7028d36f, 0x0beaf01a,
	0xf018d34f, 0xa90cd801, 0xf014d804, 0x097ad50b,
	0x8d01d36f, 0xb9a48820, 0x8d01a820, 0x780fe001,
	0x080bad01, 0xadc20432, 0x0daeadc1, 0xd800d30f,
	0xcecf030d, 0x008005a6, 0x008005a0, 0x83cb208c,
	0xf20bd900, 0x8b0b208c, 0x8f8220cc, 0x02ed0000,
	0x7fe0f20a, 0xd207d006, 0xe2818a40, 0x07c220e1,
	0x004220ca, 0x6108d104, 0x78e07ee0, 0x0000ffff,
	0x0080062d, 0x00800100, 0x0a52c0f1, 0x7508cecf,
	0x0d327728, 0x7648ceef, 0xdb0ee680, 0x042122ca,
	0x03e121ca, 0x046222ca, 0x03e223ca, 0x042221ca,
	0x20ca7750, 0xf44000e2, 0x04130811, 0x03d30811,
	0x620ad21f, 0x10800e09, 0xf036d880, 0x10510e0b,
	0x0a258d4e, 0x656a0051, 0xe282657f, 0x0f8920ca,
	0x00810000, 0x00090050, 0x0a11652a, 0x653e00b2,
	0x03d00a09, 0xf020d802, 0xd36f089e, 0x8820780f,
	0x21868f40, 0x794503fc, 0xea85a820, 0xb9878820,
	0xe281f007, 0x80a222cc, 0x8820f404, 0xa820b9a7,
	0x09138e20, 0xb9c00093, 0xb9068840, 0x7945baa6,
	0xd800a820, 0xceef0211, 0x78e0ad0c, 0x0000e198,
	0x0f4ac0f1, 0xda00ffef, 0xc0d1d804, 0x78e07ee0,
	0x0f3ac0f1, 0xda01ffef, 0xc0d1d804, 0x78e07ee0,
	0x208ae080, 0x20ca0307, 0x00000f82, 0xd10901d8,
	0xd0096109, 0x001e090d, 0x080f8812, 0xf007001e,
	0x080b880f, 0xd800001f, 0xd801f002, 0x78e07ee0,
	0x00800100, 0x008002c0, 0x78e07ee0, 0x0936c0f1,
	0x7708cecf, 0x711ae08f, 0x002d00ea, 0x2740dd00,
	0x20337400, 0x201403ca, 0x78000280, 0x08080808,
	0x08086b08, 0x57575708, 0x6a6a6a67, 0x2700d43e,
	0x11001311, 0x0d17208d, 0xd63c1233, 0x108016f0,
	0x03402010, 0x10021ef0, 0x16f1f009, 0x25421080,
	0x20101201, 0x1ef10040, 0x0f171002, 0x160f1111,
	0x090d9081, 0xd0320213, 0xf003602d, 0x0a86ddff,
	0x70a9d4ef, 0x00100873, 0xd32f0d82, 0xe83570e9,
	0xd4ef0a8e, 0xe88c70a9, 0xffef083a, 0xe80870a9,
	0x08d270e9, 0x71a9ffaf, 0x0050080d, 0x108016d3,
	0x01de0847, 0x20811100, 0x02130913, 0x108016f0,
	0x0040200f, 0x10021ef0, 0x16f1f01c, 0xb9681080,
	0x0040200f, 0x10021ef1, 0x0f76f014, 0xe813d14f,
	0x8816d018, 0x015f080d, 0x8808d017, 0x009e0817,
	0x60e8d016, 0xdd00e806, 0xd015f005, 0xe8038800,
	0x1000dd01, 0x20532080, 0x091f00c1, 0x10000090,
	0xbae72082, 0x006121ca, 0x81e222d1, 0xe581f205,
	0x21cad900, 0x208600e2, 0x782503fc, 0xceef0079,
	0x20021800, 0x0006188c, 0x008002fc, 0x0080016e,
	0x00800330, 0x008001e0, 0x008004a7, 0x00803c00,
	0x0ff2c0f1, 0xd045ce8f, 0xe0818800, 0x204af483,
	0x214a2000, 0x750a2400, 0xd32f0e9e, 0x760870a9,
	0x0ba270a9, 0x71c9d36f, 0xbfc38ee0, 0x13d10d11,
	0xe780d03b, 0x78c08800, 0xf0188e41, 0x13110d53,
	0x8913d138, 0x8953e780, 0xbaa1b8c0, 0x8953a953,
	0x7a657b1b, 0x8953a953, 0x00002052, 0xbaa0e001,
	0x78c0a953, 0xbac38e41, 0xb9e48e20, 0x902127cc,
	0xb9e7f43b, 0x81a221d1, 0x080ff237, 0xd3290081,
	0x0b678b76, 0x0d1d01de, 0xdb1013d1, 0x0c2ef011,
	0x70a9d32f, 0x70a9701a, 0xd32f0cb2, 0xf1e471c9,
	0x63abd321, 0xdb01e380, 0x7b12f203, 0xe2807b6f,
	0x80c120cc, 0xea03f20e, 0xb984e80c, 0xd11aae20,
	0x61a9d31a, 0xac4063bc, 0x7bb4e984, 0x04041b10,
	0xe80bea02, 0x13d10d09, 0xf007ae01, 0x21868e21,
	0x790503fc, 0x0d0bae21, 0xae0213d1, 0x8e21f006,
	0xb9c3b804, 0xae017825, 0x20512142, 0xa000214c,
	0x0718e501, 0x7dafffed, 0x8802d009, 0x09f8e080,
	0x0745d301, 0x78e0ce8f, 0x008005a6, 0x00803c19,
	0x008003bc, 0x0000e198, 0x00803c20, 0x008005a0,
	0xc1a1c0f1, 0xd12f0c96, 0x0813718b, 0x00000f81,
	0xd800ffff, 0xc0d1c0a1, 0xc1207ee0, 0x090db9c4,
	0x092203d1, 0xf1f80000, 0xffcf0c16, 0x78e0f1f4,
	0x0e9ec0f1, 0xc1a1ce8f, 0x0c627628, 0x718bd12f,
	0x8d0c208c, 0xf259dd00, 0x00090060, 0x830c208c,
	0x0038f253, 0x208c0009, 0xf25a83cb, 0x208cf60e,
	0xf46186c7, 0x6099d432, 0xa9c0d032, 0xd132881b,
	0x001f0897, 0xf050a9a0, 0x8b0b208c, 0x8f8220cc,
	0x02ed0000, 0xf04ff24a, 0x870c208c, 0xf606f235,
	0x850c208c, 0xf047f231, 0x890c208c, 0x8f8220cc,
	0x032c0000, 0xf03ff229, 0x870d208c, 0xf612f225,
	0x810d208c, 0xf606f221, 0x8f0c208c, 0xf033f21d,
	0x830d208c, 0x8f8220cc, 0x03540000, 0xf02bf215,
	0x8d0d208c, 0xf60af211, 0x890d208c, 0x8f8220cc,
	0x036c0000, 0xf01ff209, 0x8f0d208c, 0x8f8220cc,
	0x03840000, 0x2653f417, 0xe18110c1, 0x80a221cc,
	0x11a126d3, 0xd801f00a, 0xf006a900, 0x8920d10b,
	0x00500909, 0xf00870a9, 0x6099d405, 0xd800a9c0,
	0xd007f002, 0xceaf0619, 0x78e0c0a1, 0x00800100,
	0x008002c0, 0x008005a6, 0x0080062d, 0x0000ffff,
	0x0d8ac0f1, 0xd900ceaf, 0x620fd20f, 0x7200244a,
	0x03c020a8, 0x71cf7328, 0xe2380000, 0x91c07976,
	0x08119122, 0x08150380, 0x6b210040, 0xf008792f,
	0xd0066229, 0xf004a820, 0x88e0d004, 0xceaf05b9,
	0x78e070e9, 0x00800100, 0x00803c1c, 0x1164d108,
	0x19718080, 0xd1070002, 0x0080112c, 0x001e080f,
	0x008011d3, 0x19d3b880, 0x7ee00002, 0x00f0cc9c,
	0x008002fc, 0x8a16d20c, 0x24cab8e5, 0x79c073e2,
	0x02a220e8, 0x008070cf, 0xdbf003f4, 0xe1017836,
	0xa878792f, 0x080d8a16, 0xd004011e, 0xa838d9f0,
	0x78e07ee0, 0x008003bc, 0x008003f4, 0x0f82c0f1,
	0xd80ed0ef, 0x0816d039, 0xd139d3af, 0x080ed039,
	0xd139d3af, 0x0806d039, 0xd139d3af, 0x0ffed039,
	0xd139d36f, 0x0ff6d039, 0xd139d36f, 0x0feed039,
	0xd139d36f, 0x0fe6d039, 0xd139d36f, 0x0fded039,
	0xd139d36f, 0x0fd6d039, 0xd139d36f, 0x0fced039,
	0xd139d36f, 0x0fc6d039, 0xd139d36f, 0x0fbed039,
	0xd139d36f, 0x0fb6d039, 0xd139d36f, 0x0faed039,
	0xd139d36f, 0x0fa6d039, 0xd139d36f, 0x0f9ed039,
	0xd139d36f, 0x0f96d039, 0xd139d36f, 0x0f8ed039,
	0xd139d36f, 0x0f86d039, 0xd139d36f, 0x0f7ed039,
	0xd139d36f, 0x0f76d039, 0xd139d36f, 0x0f6ed039,
	0xd139d36f, 0x0f66d039, 0xd139d36f, 0x0f5ed039,
	0xd139d36f, 0x0f56d039, 0xd139d36f, 0x0f4ed039,
	0xd139d36f, 0xff4f0e9a, 0xd900d038, 0xd038a833,
	0xc0d1a820, 0x78e07ee0, 0x00009d10, 0x00061cdc,
	0x00009ca8, 0x00062f08, 0x00009c64, 0x00062cdc,
	0x00009c7c, 0x00062d04, 0x00009c70, 0x000618b8,
	0x00009cc0, 0x000619b4, 0x0000921c, 0x000626fc,
	0x000095e0, 0x000628dc, 0x00008ed8, 0x00062354,
	0x0000955c, 0x00062a20, 0x000093d0, 0x00062880,
	0x00009458, 0x00062890, 0x00009cd8, 0x00062b80,
	0x00009c94, 0x00062b50, 0x00000500, 0x000618a8,
	0x00008d24, 0x000621c4, 0x00009030, 0x0006252c,
	0x00008e28, 0x000622dc, 0x000091bc, 0x000626a8,
	0x0000950c, 0x000628d8, 0x00008f68, 0x00062458,
	0x000090f0, 0x000625e0, 0x00006e44, 0x000620fc,
	0x00007364, 0x00062128, 0x000061c8, 0x00061ff8,
	0x00009198, 0x00062680, 0x008003bc, 0x00803c19,
	0xd0ef05bd, 0x78e0d812, 0x0b16c0f1, 0x7508ce8f,
	0x0dd6d80f, 0x71a9d0ef, 0x0d17d60a, 0x0da21051,
	0xd810d0ef, 0xff4f0e02, 0xae00d801, 0x0d92f008,
	0xd811d0ef, 0x0e3ad800, 0xae00ff6f, 0xce8f0351,
	0x00803c00, 0x10c0224a, 0xcfaf0385, 0x1100234a,
};

static const struct ec_val_reg ec_gpio_init_table[] = {
	/*
	 * Probably some early GPIO initialization, seting GPIIO functions.
	 * The LSBs in third column match the GPIO config registers offsets for
	 * non-default GPIOs.
	 */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x08cc }, /* GP063 (def) /KBDRST#*/
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x08d0 }, /* GP064 (def) /A20M */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x089c }, /* GP047/TXD1 (def)  */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x0878 }, /* GP036 (def) /SMBCLK1 */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x0880 }, /* GP040 (def) / SMBDAT1 */
	{ EC_GPIO_OD | EC_GPIO_FUNC1, 0x0884 }, /* GP041 (def) / IO_PME# */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x08e4 }, /* GP071 (def) / IO_SMI# */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x08e0 }, /* GP070 (def) / SPEAKER */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x0848 }, /* GP022 (def) / PWM1 */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x084c }, /* GP023 (def) / PWM2 */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x0850 }, /* GP024 (def) / PWM3 */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x083c }, /* GP017 - TACH1 (def) */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x0840 }, /* GP020 - TACH2 (def) */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x0844 }, /* GP021 - TACH3 (def) */
	{ EC_GPIO_PP | EC_GPIO_FUNC1, 0x0814 }, /* GP005 (def) / PECI_REQ# */
};

static const struct ec_val_reg ec_hwm_early_init_table[] = {
	/* Probably some early hardware monitor initialization */
	{ 0xff, 0x0005 },
	{ 0x30, 0x00f0 },
	{ 0x10, 0x00f8 },
	{ 0x00, 0x00f9 },
	{ 0x00, 0x00fa },
	{ 0x00, 0x00fb },
	{ 0x00, 0x00ea },
	{ 0x00, 0x00eb },
	{ 0x7c, 0x00ef },
	{ 0x03, 0x006e },
	{ 0x51, 0x02d0 },
	{ 0x01, 0x02d2 },
	{ 0x12, 0x059a },
	{ 0x11, 0x059e },
	{ 0x14, 0x05a2 },
	{ 0x55, 0x05a3 },
	{ 0x01, 0x02db },
	{ 0x01, 0x0040 },
};

static const struct ec_val_reg ec_hwm_init_seq[] = {
	{ 0xa0, 0x02fc },
	{ 0x32, 0x02fd },
	{ 0x77, 0x0005 },
	{ 0x0f, 0x0018 },
	{ 0x2f, 0x0019 },
	{ 0x2f, 0x001a },
	{ 0x33, 0x008a },
	{ 0x33, 0x008b },
	{ 0x33, 0x008c },
	{ 0x10, 0x00ba },
	{ 0xff, 0x00d1 },
	{ 0xff, 0x00d6 },
	{ 0xff, 0x00db },
	{ 0x00, 0x0048 },
	{ 0x00, 0x0049 },
	{ 0x00, 0x007a },
	{ 0x00, 0x007b },
	{ 0x00, 0x007c },
	{ 0x00, 0x0080 },
	{ 0x00, 0x0081 },
	{ 0x00, 0x0082 },
	{ 0xbb, 0x0083 },
	{ 0xb0, 0x0084 },
	{ 0x88, 0x01a1 },
	{ 0x80, 0x01a4 },
	{ 0x00, 0x0088 },
	{ 0x00, 0x0089 },
	{ 0x02, 0x00a0 },
	{ 0x02, 0x00a1 },
	{ 0x02, 0x00a2 },
	{ 0x04, 0x00a4 },
	{ 0x04, 0x00a5 },
	{ 0x04, 0x00a6 },
	{ 0x00, 0x00ab },
	{ 0x3f, 0x00ad },
	{ 0x07, 0x00b7 },
	{ 0x50, 0x0062 },
	{ 0x46, 0x0063 },
	{ 0x50, 0x0064 },
	{ 0x46, 0x0065 },
	{ 0x50, 0x0066 },
	{ 0x46, 0x0067 },
	{ 0x98, 0x0057 },
	{ 0x98, 0x0059 },
	{ 0x7c, 0x0061 },
	{ 0x00, 0x01bc },
	{ 0x00, 0x01bd },
	{ 0x00, 0x01bb },
	{ 0xdd, 0x0085 },
	{ 0xdd, 0x0086 },
	{ 0x07, 0x0087 },
	{ 0x5e, 0x0090 },
	{ 0x5e, 0x0091 },
	{ 0x5d, 0x0095 },
	{ 0x00, 0x0096 },
	{ 0x00, 0x0097 },
	{ 0x00, 0x009b },
	{ 0x86, 0x00ae },
	{ 0x86, 0x00af },
	{ 0x67, 0x00b3 },
	{ 0xff, 0x00c4 },
	{ 0xff, 0x00c5 },
	{ 0xff, 0x00c9 },
	{ 0x01, 0x0040 },
	{ 0x00, 0x02fc },
	{ 0x9a, 0x02b3 },
	{ 0x05, 0x02b4 },
	{ 0x01, 0x02cc },
	{ 0x4c, 0x02d0 },
	{ 0x01, 0x02d2 },
	{ 0x01, 0x006f },
	{ 0x02, 0x0070 },
	{ 0x03, 0x0071 },
};

static const ec_chassis_tdp_t ec_hwm_chassis3[] = {
	{ 0x33, 0x0005, TDP_COMMON },
	{ 0x2f, 0x0018, TDP_COMMON },
	{ 0x2f, 0x0019, TDP_COMMON },
	{ 0x2f, 0x001a, TDP_COMMON },
	{ 0x00, 0x0080, TDP_COMMON },
	{ 0x00, 0x0081, TDP_COMMON },
	{ 0xbb, 0x0083, TDP_COMMON },
	{ 0x8a, 0x0085, TDP_16 },
	{ 0x2c, 0x0086, TDP_16 },
	{ 0x66, 0x008a, TDP_16 },
	{ 0x5b, 0x008b, TDP_16 },
	{ 0x65, 0x0090, TDP_COMMON },
	{ 0x70, 0x0091, TDP_COMMON },
	{ 0x86, 0x0092, TDP_COMMON },
	{ 0xa4, 0x0096, TDP_COMMON },
	{ 0xa4, 0x0097, TDP_COMMON },
	{ 0xa4, 0x0098, TDP_COMMON },
	{ 0xa4, 0x009b, TDP_COMMON },
	{ 0x0e, 0x00a0, TDP_COMMON },
	{ 0x0e, 0x00a1, TDP_COMMON },
	{ 0x7c, 0x00ae, TDP_COMMON },
	{ 0x86, 0x00af, TDP_COMMON },
	{ 0x95, 0x00b0, TDP_COMMON },
	{ 0x9a, 0x00b3, TDP_COMMON },
	{ 0x08, 0x00b6, TDP_COMMON },
	{ 0x08, 0x00b7, TDP_COMMON },
	{ 0x64, 0x00ea, TDP_COMMON },
	{ 0xff, 0x00ef, TDP_COMMON },
	{ 0x15, 0x00f8, TDP_COMMON },
	{ 0x00, 0x00f9, TDP_COMMON },
	{ 0x30, 0x00f0, TDP_COMMON },
	{ 0x01, 0x00fd, TDP_COMMON },
	{ 0x88, 0x01a1, TDP_COMMON },
	{ 0x08, 0x01a2, TDP_COMMON },
	{ 0x08, 0x01b1, TDP_COMMON },
	{ 0x94, 0x01be, TDP_COMMON },
	{ 0x94, 0x0280, TDP_16 },
	{ 0x11, 0x0281, TDP_16 },
	{ 0x03, 0x0282, TDP_COMMON },
	{ 0x0a, 0x0283, TDP_COMMON },
	{ 0x80, 0x0284, TDP_COMMON },
	{ 0x03, 0x0285, TDP_COMMON },
	{ 0x68, 0x0288, TDP_16 },
	{ 0x10, 0x0289, TDP_16 },
	{ 0x03, 0x028a, TDP_COMMON },
	{ 0x0a, 0x028b, TDP_COMMON },
	{ 0x80, 0x028c, TDP_COMMON },
	{ 0x03, 0x028d, TDP_COMMON },
};

static const ec_chassis_tdp_t ec_hwm_chassis4[] = {
	{ 0x33, 0x0005, TDP_COMMON },
	{ 0x2f, 0x0018, TDP_COMMON },
	{ 0x2f, 0x0019, TDP_COMMON },
	{ 0x2f, 0x001a, TDP_COMMON },
	{ 0x00, 0x0080, TDP_COMMON },
	{ 0x00, 0x0081, TDP_COMMON },
	{ 0xbb, 0x0083, TDP_COMMON },
	{ 0x99, 0x0085, TDP_32 },
	{ 0x98, 0x0085, TDP_16 },
	{ 0xbc, 0x0086, TDP_32 },
	{ 0x1c, 0x0086, TDP_16 },
	{ 0x39, 0x008a, TDP_32 },
	{ 0x3d, 0x008a, TDP_16 },
	{ 0x40, 0x008b, TDP_32 },
	{ 0x43, 0x008b, TDP_16 },
	{ 0x68, 0x0090, TDP_COMMON },
	{ 0x5e, 0x0091, TDP_COMMON },
	{ 0x86, 0x0092, TDP_COMMON },
	{ 0xa4, 0x0096, TDP_COMMON },
	{ 0xa4, 0x0097, TDP_COMMON },
	{ 0xa4, 0x0098, TDP_COMMON },
	{ 0xa4, 0x009b, TDP_COMMON },
	{ 0x0c, 0x00a0, TDP_COMMON },
	{ 0x0c, 0x00a1, TDP_COMMON },
	{ 0x72, 0x00ae, TDP_COMMON },
	{ 0x7c, 0x00af, TDP_COMMON },
	{ 0x9a, 0x00b0, TDP_COMMON },
	{ 0x7c, 0x00b3, TDP_COMMON },
	{ 0x08, 0x00b6, TDP_COMMON },
	{ 0x08, 0x00b7, TDP_COMMON },
	{ 0x64, 0x00ea, TDP_COMMON },
	{ 0xff, 0x00ef, TDP_COMMON },
	{ 0x15, 0x00f8, TDP_COMMON },
	{ 0x00, 0x00f9, TDP_COMMON },
	{ 0x30, 0x00f0, TDP_COMMON },
	{ 0x01, 0x00fd, TDP_COMMON },
	{ 0x88, 0x01a1, TDP_COMMON },
	{ 0x08, 0x01a2, TDP_COMMON },
	{ 0x08, 0x01b1, TDP_COMMON },
	{ 0x90, 0x01be, TDP_COMMON },
	{ 0x94, 0x0280, TDP_32 },
	{ 0x11, 0x0281, TDP_32 },
	{ 0x68, 0x0280, TDP_16 },
	{ 0x10, 0x0281, TDP_16 },
	{ 0x03, 0x0282, TDP_COMMON },
	{ 0x0a, 0x0283, TDP_COMMON },
	{ 0x80, 0x0284, TDP_COMMON },
	{ 0x03, 0x0285, TDP_COMMON },
	{ 0xa0, 0x0288, TDP_32 },
	{ 0x0f, 0x0289, TDP_32 },
	{ 0xd8, 0x0288, TDP_16 },
	{ 0x0e, 0x0289, TDP_16 },
	{ 0x03, 0x028a, TDP_COMMON },
	{ 0x0a, 0x028b, TDP_COMMON },
	{ 0x80, 0x028c, TDP_COMMON },
	{ 0x03, 0x028d, TDP_COMMON },
};

static const ec_chassis_tdp_t ec_hwm_chassis5[] = {
	{ 0x33, 0x0005, TDP_COMMON },
	{ 0x2f, 0x0018, TDP_COMMON },
	{ 0x2f, 0x0019, TDP_COMMON },
	{ 0x2f, 0x001a, TDP_COMMON },
	{ 0x00, 0x0080, TDP_COMMON },
	{ 0x00, 0x0081, TDP_COMMON },
	{ 0xbb, 0x0083, TDP_COMMON },
	{ 0x89, 0x0085, TDP_32 },
	{ 0x99, 0x0085, TDP_16 },
	{ 0x9c, 0x0086, TDP_COMMON },
	{ 0x39, 0x008a, TDP_32 },
	{ 0x42, 0x008a, TDP_16 },
	{ 0x6b, 0x008b, TDP_32 },
	{ 0x74, 0x008b, TDP_16 },
	{ 0x5e, 0x0091, TDP_COMMON },
	{ 0x86, 0x0092, TDP_COMMON },
	{ 0xa4, 0x0096, TDP_COMMON },
	{ 0xa4, 0x0097, TDP_COMMON },
	{ 0xa4, 0x0098, TDP_COMMON },
	{ 0xa4, 0x009b, TDP_COMMON },
	{ 0x0c, 0x00a0, TDP_COMMON },
	{ 0x0c, 0x00a1, TDP_COMMON },
	{ 0x7c, 0x00ae, TDP_COMMON },
	{ 0x7c, 0x00af, TDP_COMMON },
	{ 0x9a, 0x00b0, TDP_COMMON },
	{ 0x7c, 0x00b3, TDP_COMMON },
	{ 0x08, 0x00b6, TDP_COMMON },
	{ 0x08, 0x00b7, TDP_COMMON },
	{ 0x64, 0x00ea, TDP_COMMON },
	{ 0xff, 0x00ef, TDP_COMMON },
	{ 0x15, 0x00f8, TDP_COMMON },
	{ 0x00, 0x00f9, TDP_COMMON },
	{ 0x30, 0x00f0, TDP_COMMON },
	{ 0x01, 0x00fd, TDP_COMMON },
	{ 0x88, 0x01a1, TDP_COMMON },
	{ 0x08, 0x01a2, TDP_COMMON },
	{ 0x08, 0x01b1, TDP_COMMON },
	{ 0x90, 0x01be, TDP_COMMON },
	{ 0x94, 0x0280, TDP_32 },
	{ 0x11, 0x0281, TDP_32 },
	{ 0x3c, 0x0280, TDP_16 },
	{ 0x0f, 0x0281, TDP_16 },
	{ 0x03, 0x0282, TDP_COMMON },
	{ 0x0a, 0x0283, TDP_COMMON },
	{ 0x80, 0x0284, TDP_COMMON },
	{ 0x03, 0x0285, TDP_COMMON },
	{ 0x60, 0x0288, TDP_32 },
	{ 0x09, 0x0289, TDP_32 },
	{ 0x98, 0x0288, TDP_16 },
	{ 0x08, 0x0289, TDP_16 },
	{ 0x03, 0x028a, TDP_COMMON },
	{ 0x0a, 0x028b, TDP_COMMON },
	{ 0x80, 0x028c, TDP_COMMON },
	{ 0x03, 0x028d, TDP_COMMON },
};

static const ec_chassis_tdp_t ec_hwm_chassis6[] = {
	{ 0x33, 0x0005, TDP_COMMON },
	{ 0x2f, 0x0018, TDP_COMMON },
	{ 0x2f, 0x0019, TDP_COMMON },
	{ 0x2f, 0x001a, TDP_COMMON },
	{ 0x00, 0x0080, TDP_COMMON },
	{ 0x00, 0x0081, TDP_COMMON },
	{ 0xbb, 0x0083, TDP_COMMON },
	{ 0x99, 0x0085, TDP_32 },
	{ 0x98, 0x0085, TDP_16 },
	{ 0xdc, 0x0086, TDP_32 },
	{ 0x9c, 0x0086, TDP_16 },
	{ 0x3d, 0x008a, TDP_32 },
	{ 0x43, 0x008a, TDP_16 },
	{ 0x4e, 0x008b, TDP_32 },
	{ 0x47, 0x008b, TDP_16 },
	{ 0x6d, 0x0090, TDP_COMMON },
	{ 0x5f, 0x0091, TDP_32 },
	{ 0x61, 0x0091, TDP_16 },
	{ 0x86, 0x0092, TDP_COMMON },
	{ 0xa4, 0x0096, TDP_COMMON },
	{ 0xa4, 0x0097, TDP_COMMON },
	{ 0xa4, 0x0098, TDP_COMMON },
	{ 0xa4, 0x009b, TDP_COMMON },
	{ 0x0e, 0x00a0, TDP_COMMON },
	{ 0x0e, 0x00a1, TDP_COMMON },
	{ 0x7c, 0x00ae, TDP_COMMON },
	{ 0x7c, 0x00af, TDP_COMMON },
	{ 0x98, 0x00b0, TDP_32 },
	{ 0x9a, 0x00b0, TDP_16 },
	{ 0x9a, 0x00b3, TDP_COMMON },
	{ 0x08, 0x00b6, TDP_COMMON },
	{ 0x08, 0x00b7, TDP_COMMON },
	{ 0x64, 0x00ea, TDP_COMMON },
	{ 0xff, 0x00ef, TDP_COMMON },
	{ 0x15, 0x00f8, TDP_COMMON },
	{ 0x00, 0x00f9, TDP_COMMON },
	{ 0x30, 0x00f0, TDP_COMMON },
	{ 0x01, 0x00fd, TDP_COMMON },
	{ 0x88, 0x01a1, TDP_COMMON },
	{ 0x08, 0x01a2, TDP_COMMON },
	{ 0x08, 0x01b1, TDP_COMMON },
	{ 0x97, 0x01be, TDP_32 },
	{ 0x95, 0x01be, TDP_16 },
	{ 0x68, 0x0280, TDP_32 },
	{ 0x10, 0x0281, TDP_32 },
	{ 0xd8, 0x0280, TDP_16 },
	{ 0x0e, 0x0281, TDP_16 },
	{ 0x03, 0x0282, TDP_COMMON },
	{ 0x0a, 0x0283, TDP_COMMON },
	{ 0x80, 0x0284, TDP_COMMON },
	{ 0x03, 0x0285, TDP_COMMON },
	{ 0xe4, 0x0288, TDP_32 },
	{ 0x0c, 0x0289, TDP_32 },
	{ 0x10, 0x0288, TDP_16 },
	{ 0x0e, 0x0289, TDP_16 },
	{ 0x03, 0x028a, TDP_COMMON },
	{ 0x0a, 0x028b, TDP_COMMON },
	{ 0x80, 0x028c, TDP_COMMON },
	{ 0x03, 0x028d, TDP_COMMON },
};

static void ec_read_write_ldn_register(uint16_t ldn, uint8_t *val, uint16_t reg,
				       uint8_t rw_bit)
{
	uint16_t timeout = 0;
	rw_bit &= 1;
	sch5545_emi_ec2h_mailbox_clear();
	sch5545_emi_ec_write16(0x8000, (ldn << 1) | 0x100 | rw_bit);

	sch5545_emi_set_ec_addr(0x8004);

	if (rw_bit)
		outb(*val, emi_bar + SCH5545_EMI_EC_DATA);

	outb(reg & 0xff, emi_bar + SCH5545_EMI_EC_DATA + 2);
	outb((reg >> 8) & 0xff, emi_bar + SCH5545_EMI_EC_DATA + 3);
	sch5545_emi_h2ec_mbox_write(1);

	do {
		timeout++;
		if((sch5545_emi_ec2h_mbox_read() & 1) != 0)
			break;
	} while (timeout < 0xfff);

	sch5545_emi_set_int_src(0x11);
	sch5545_emi_h2ec_mbox_write(0xc0);

	if (!rw_bit)
		*val = inb(emi_bar + SCH5545_EMI_EC_DATA);
}

static void ec_init_gpios(void)
{
	unsigned int i;
	uint8_t val;

	for (i = 0; i < ARRAY_SIZE(ec_gpio_init_table); i++ ) {
		val = ec_gpio_init_table[i].val;
		ec_read_write_ldn_register(EC_GPIO_LDN, &val,
					   ec_gpio_init_table[i].reg,
					   WRITE_OP);
	}
}

static void ec_early_hwm_init(void)
{
	unsigned int i;
	uint8_t val;

	for (i = 0; i < ARRAY_SIZE(ec_hwm_early_init_table); i++ ) {
		val = ec_hwm_early_init_table[i].val;
		ec_read_write_ldn_register(EC_HWM_LDN, &val,
					   ec_hwm_early_init_table[i].reg,
					   WRITE_OP);
	}
}

void sch5545_ec_early_init(void)
{
	emi_bar = sch5545_read_emi_bar(0x2f);

	ec_init_gpios();
	ec_early_hwm_init();
}

static uint8_t send_mbox_msg_with_int(uint8_t mbox_message)
{
	uint8_t int_sts, int_cond;

	sch5545_emi_h2ec_mbox_write(mbox_message);

	do {
		int_sts = sch5545_emi_get_int_src_low();
		int_cond = int_sts & 0x71;
	} while (int_cond == 0);

	sch5545_emi_set_int_src_low(int_cond);

	if ((int_sts & 1) == 0)
		return 0;

	if (sch5545_emi_ec2h_mbox_read() == mbox_message)
		return 1;

	return 0;
}

static uint8_t send_mbox_msg_simple(uint8_t mbox_message)
{
	uint8_t int_sts;

	sch5545_emi_h2ec_mbox_write(mbox_message);

	do {
		int_sts = sch5545_emi_get_int_src_low();
		if ((int_sts & 70) != 0)
			return 0;
	} while ((int_sts & 1) == 0);

	if (sch5545_emi_ec2h_mbox_read() == mbox_message)
		return 1;

	return 0;
}

static void ec_check_mbox_and_int_status(uint8_t int_src, uint8_t mbox_msg)
{
	uint8_t val;

	val = sch5545_emi_ec2h_mbox_read();
	if (val != mbox_msg)
		printk(BIOS_INFO,"EC2H mailbox should return %02x, is %02x\n",
		       mbox_msg, val);

	val = sch5545_emi_get_int_src_low();
	if (val != int_src)
		printk(BIOS_INFO,"EC INT SRC should be %02x, is %02x\n",
		       int_src, val);
	sch5545_emi_set_int_src_low(val);
}

static uint8_t ec_read_write_reg(uint8_t ldn, uint16_t reg, uint8_t *value,
				 uint8_t rw_bit)
{
	uint8_t int_mask_bckup, ret = 0;
	rw_bit &= 1;

	int_mask_bckup = sch5545_emi_get_int_mask_low();
	sch5545_emi_set_int_mask_low(0);

	sch5545_emi_ec_write16(0x8000, (ldn << 1) | 0x100 | rw_bit);
	if (rw_bit)
		sch5545_emi_ec_write32(0x8004, (reg << 16) | *value);
	else
		sch5545_emi_ec_write32(0x8004, reg << 16);

	ret = send_mbox_msg_with_int(1);
	if (ret && !rw_bit)
		*value = sch5545_emi_ec_read8(0x8004);
	else if (ret != 1 && rw_bit)
		printk(BIOS_WARNING, "EC mailbox returned unexpected value"
		       " when writing %02x to %04x\n", *value, reg);
	else if (ret != 1 && !rw_bit)
		printk(BIOS_WARNING, "EC mailbox returned unexpected value"
		       " when reading %04x \n", reg);

	sch5545_emi_set_int_mask_low(int_mask_bckup);

	return ret;
}

int ec_write_32_bulk_with_int(uint16_t addr, uint32_t *data,
				     uint32_t size, uint8_t mbox_msg)
{
	uint8_t status;

	sch5545_emi_ec_write32_bulk(addr, data, size);

	status = send_mbox_msg_with_int(mbox_msg);

	if (status != 1) {
		printk(BIOS_WARNING, "EC mailbox returned unexpected value\n");
		return -1;
	}

	return (int)status;
}


uint16_t sch5545_get_ec_fw_version(void)
{
	uint8_t val;
	uint16_t ec_fw_version;

	/* Read the FW version currently loaded by EC */
	ec_read_write_reg(EC_HWM_LDN, 0x2ad, &val, READ_OP);
	ec_fw_version = (val << 8);
	ec_read_write_reg(EC_HWM_LDN, 0x2ae, &val, READ_OP);
	ec_fw_version |= val;
	ec_read_write_reg(EC_HWM_LDN, 0x2ac, &val, READ_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x2fd, &val, READ_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x2b0, &val, READ_OP);

	return ec_fw_version;
}

void sch5545_update_ec_firmware(uint16_t ec_version)
{
	uint8_t status;
	uint16_t ec_fw_version = ec_fw[3] & 0xffff;

	if (ec_version != ec_fw_version) {
		printk(BIOS_INFO, "SCH5545 EC is not functional, probably due"
				  " to power failure\n");
		printk(BIOS_INFO, "Uploading EC firmware to SCH5545\n");

		if (!send_mbox_msg_simple(0x03)) {
			printk(BIOS_WARNING, "EC didn't accept FW upload start"
			       " signal\n");
			printk(BIOS_WARNING, "EC firmware update failed!\n");
			return;
		}

		sch5545_emi_ec_write32_bulk(0x8100, ec_fw, ARRAY_SIZE(ec_fw));

		status = send_mbox_msg_simple(0x04);
		status += send_mbox_msg_simple(0x06);

		if (status != 2) {
			printk(BIOS_WARNING, "EC firmware update failed!\n");
		}

		if (ec_fw_version != sch5545_get_ec_fw_version()) {
			printk(BIOS_ERR, "EC firmware update failed!\n");
			printk(BIOS_ERR, "The fans will keep running at"
						" maximum speed\n");
		} else {
			printk(BIOS_INFO, "EC firmware update success\n");
			/*
			 * The vendor BIOS does a full reset after EC firmware
			 * update. Most likely becasue the fans are adapting
			 * very slowly after automatic fan control is enabled.
			 * This make huge noise. To avoid it, also do the full
			 * reset. On next boot, it will not be necessary.
			 */
			do_full_reset();
		}
	} else {
		printk(BIOS_INFO, "SCH5545 EC firmware up to date\n");
	}
}

void sch5545_ec_hwm_early_init(void)
{
	uint8_t val;
	int i;

	printk(BIOS_DEBUG, "%s\n", __func__);

	ec_check_mbox_and_int_status(0x20, 0x01);

	ec_read_write_reg(2, 0xcb, &val, READ_OP);
	printk(BIOS_DEBUG, "%s: 0xcb is %02x\n", __func__, val);
	ec_read_write_reg(2, 0xb8, &val, READ_OP);
	printk(BIOS_DEBUG, "%s: 0xb8 is %02x\n", __func__, val);

	for (i = 0; i < ARRAY_SIZE(ec_hwm_init_seq); i++ ) {
		val = ec_hwm_init_seq[i].val;
		ec_read_write_reg(EC_HWM_LDN, ec_hwm_init_seq[i].reg, &val,
				  WRITE_OP);
	}

	ec_check_mbox_and_int_status(0x01, 0x01);
}
/*
static int sch5545_multiple_read(uint32_t *buf, uint8_t *bytes_read)
{
	uint8_t to_read, i;

	uint32_t status = sch5545_emi_ec_read32(0x8080);
	to_read = (status >> 24) & 0xff;

	if ((status & 0xff) != 0x0c) {
		printk(BIOS_WARNING, "Wrong LDN detected (%x), expected 0xc\n",
			status & 0xff);
		return -1;
	}

	if (*bytes_read >= to_read) {
		for (i = 0; i <= to_read/4; i++) {
			*(buf + i) = sch5545_emi_ec_read32(0x8084 + i*4);
			printk(BIOS_WARNING, "%s: read value %08x\n", __func__,
				*(buf + i));
		}
	} else {
		printk(BIOS_WARNING, "More bytes to read (%x) than"
			" expected (%x)\n", to_read, *bytes_read);
		return -1;
	}

	*bytes_read = to_read;

	if (send_mbox_msg_simple(0x05))
		return 0;

	printk(BIOS_WARNING, "%s: Mailbox returned unexpected value\n",
		__func__);
	return -1;

}
*/
static uint8_t get_sku_temperature_target(void)
{
	msr_t msr;
	uint8_t temp_control_offset;

	msr = rdmsr(MSR_TEMPERATURE_TARGET);
	temp_control_offset = (msr.lo >> 8) & 0xff;

	if (temp_control_offset == 0)
		temp_control_offset = 20;

	return TCC_TEMPERATURE - temp_control_offset;
}

static uint8_t get_sku_tdp_config(void)
{
	msr_t msr;
	uint32_t power_unit, tdp;
	/* Get units */
	msr = rdmsr(MSR_PKG_POWER_SKU_UNIT);
	power_unit = msr.lo & 0xf;

	/* Get power defaults for this SKU */
	msr = rdmsr(MSR_PKG_POWER_SKU);
	tdp = msr.lo & 0x7fff;

	/* These number will determine which settings to use to init EC */
	if ((tdp >> power_unit) < 66)
		return 16;
	else
		return 32;
}

static uint8_t get_chassis_type(void)
{
	uint8_t chassis_id;

	chassis_id  = get_gpio(GPIO_CHASSIS_ID0);
	chassis_id |= get_gpio(GPIO_CHASSIS_ID1) << 1;
	chassis_id |= get_gpio(GPIO_CHASSIS_ID2) << 2;
	chassis_id |= get_gpio(GPIO_FRONT_PANEL_CHASSIS_DET_L) << 3;

	/* This mapping will determine which EC init sequence to use */
	switch (chassis_id)
	{
	case 0x0:
		return 5;
	case 0x8:
		return 4;
	case 0x3:
	case 0xb:
		return 3;
	case 0x1:
	case 0x9:
	case 0x5:
	case 0xd:
		return 6;
	default:
		printk(BIOS_DEBUG, "Unknown chassis ID %x\n", chassis_id);
		break;
	}

	return 0xff;
}

static void prepare_for_hwm_ec_sequence(uint8_t write_only, uint8_t *value)
{
	uint16_t reg;
	uint8_t val;

	if (write_only == 1) {
		val = *value;
		reg = 0x02fc;
	} else {
		if (value != NULL)
			ec_read_write_reg(EC_HWM_LDN, 0x02fc, value, READ_OP);
		val = 0xa0;
		ec_read_write_reg(EC_HWM_LDN, 0x2fc, &val, WRITE_OP);
		val = 0x32;
		reg = 0x02fd;
	}

	ec_read_write_reg(1, reg, &val, WRITE_OP);
}

static void ec_hwm_init_late(const ec_chassis_tdp_t *ec_hwm_sequence,
			     size_t size)
{
	unsigned int i;
	uint8_t val;
	uint8_t tdp_config = get_sku_tdp_config();

	for (i = 0; i < size; i++ ) {
		if (ec_hwm_sequence[i].tdp == tdp_config ||
		    ec_hwm_sequence[i].tdp == TDP_COMMON) {
			val = ec_hwm_sequence[i].val;
			ec_read_write_reg(EC_HWM_LDN, ec_hwm_sequence[i].reg,
					  &val, WRITE_OP);
		}
	}
}

void sch5545_ec_enable_smi(void *unused)
{
	uint8_t val;

	printk(BIOS_DEBUG, "%s\n", __func__);

	sch5545_emi_init(0x2e);
	ec_check_mbox_and_int_status(0x01, 0x01);

	/* SioDashController */
	ec_read_write_reg(EC_HWM_LDN, 0x02d0, &val, READ_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x059e, &val, READ_OP);

	val = sch5545_emi_get_int_mask_low();
	sch5545_emi_set_int_mask_low((val & 0xfe) | 8);

	val = inb(SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN1);
	outb(val | 0x40, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN1);
	outb(0x40, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN2);

	outb(0x01, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN);

	printk(BIOS_DEBUG, "%s: handling SMIs\n", __func__);
	sch5545_ec_handle_serirq_smi();
}

void sch5545_ec_init_late(void)
{
	uint8_t val;
	uint32_t data[5];

	printk(BIOS_DEBUG, "%s\n", __func__);

	/* Do we need this? */
	/* DashBiosManager */
	//sch5545_multiple_read(0x18, 0xa8, 3, 0);
	sch5545_emi_ec_write32(0x8000, 0x0d0010d);
	data[0] = 0xc5229201;
	data[1] = 0x00050100;
	data[2] = 0x00000101;
	ec_write_32_bulk_with_int(0x8004, data, 3, 0x02);

	val = sch5545_emi_get_int_src_low();
	if (val != 0x08)
		printk(BIOS_INFO, "EC INT SRC should be 0x08, is %02x\n", val);

	//sch5545_multiple_write(0x10, 0x00, 4, 0);
	sch5545_emi_ec_write32(0x8000, 0x0d0010d);
	data[0] = 0xc5229201;
	data[1] = 0x00040100;
	data[2] = 0xfff0f101;
	data[3] = 0x00000000;
	ec_write_32_bulk_with_int(0x8004, data, 4, 0x02);

	val = sch5545_emi_get_int_src_low();
	if (val != 0x08)
		printk(BIOS_INFO, "EC INT SRC should be 0x08, is %02x\n", val);

	//sch5545_multiple_write(0x10, 0x00, 4, 0);
	sch5545_emi_ec_write32(0x8000, 0x0d0010d);
	data[0] = 0xc6229201;
	data[1] = 0x00040100;
	data[2] = 0xfff0f101;
	data[3] = 0x00000000;
	ec_write_32_bulk_with_int(0x8004, data, 4, 0x02);

	val = sch5545_emi_get_int_src_low();
	if (val != 0x08)
		printk(BIOS_INFO, "EC INT SRC should be 0x08, is %02x\n", val);

	//sch5545_multiple_write(0x10, 0x10, 4, 0);
	sch5545_emi_ec_write32(0x8000, 0x0d0010d);
	data[0] = 0xc7229201;
	data[1] = 0x00040100;
	data[2] = 0xfff0f101;
	data[3] = 0x00000000;
	ec_write_32_bulk_with_int(0x8004, data, 4, 0x02);
	val = sch5545_emi_get_int_src_low();
	if (val != 0x08)
		printk(BIOS_INFO, "EC INT SRC should be 0x08, is %02x\n", val);

	//val = sch5545_multiple_write(0x10, 0x00, 4, 1);
	sch5545_emi_ec_write32(0x8000, 0x0b0010d);
	data[0] = 0xc0229201;
	data[1] = 0x00010100;
	data[2] = 0xfff0f101;
	data[3] = 0x00000000 | (val << 8);
	ec_write_32_bulk_with_int(0x8004, data, 4, 0x02);
	val = sch5545_emi_get_int_src_low();
	if (val != 0x08)
		printk(BIOS_INFO, "EC INT SRC should be 0x08, is %02x\n", val);

	//sch5545_multiple_write(0x10, 0x00, 4, 0);
	sch5545_emi_ec_write32(0x8000, 0x110010d);
	data[0] = 0xc0229201;
	data[1] = 0x04000101;
	data[2] = 0x09000400;
	data[3] = 0x00000000;
	data[4] = 0x00000900;
	ec_write_32_bulk_with_int(0x8004, data, 5, 0x02);
	val = sch5545_emi_get_int_src_low();
	if (val != 0x08)
		printk(BIOS_INFO, "EC INT SRC should be 0x08, is %02x\n", val);

	//sch5545_multiple_write(0x00, 0x00, 4, 0);
	sch5545_emi_ec_write32(0x8000, 0x070010d);
	data[0] = 0xc8229201;
	data[1] = 0x00058000;
	ec_write_32_bulk_with_int(0x8004, data, 5, 0x02);
	val = sch5545_emi_get_int_src_low();
	if (val != 0x08)
		printk(BIOS_INFO, "EC INT SRC should be 0x08, is %02x\n", val);

	//sch5545_multiple_write(0x10, 0x70, 4, 0);
	sch5545_emi_ec_write32(0x8000, 0x0f0010d);
	data[0] = 0xc8229201;
	data[1] = 0x0d038001;
	data[2] = 0x28152938;
	data[3] = 0x00201806;
	ec_write_32_bulk_with_int(0x8004, data, 4, 0x02);
	val = sch5545_emi_get_int_src_low();
	if (val != 0x08)
		printk(BIOS_INFO, "EC INT SRC should be 0x08, is %02x\n", val);

	//sch5545_multiple_write(0x10, 0x00, 4, 0);
	sch5545_emi_ec_write32(0x8000, 0x0c0010d);
	data[0] = 0xc9229201;
	data[1] = 0x04038001;
	data[2] = 0x28152938;
	data[3] = 0x02010003;
	ec_write_32_bulk_with_int(0x8004, data, 4, 0x02);
	val = sch5545_emi_get_int_src_low();
	if (val != 0x08)
		printk(BIOS_INFO, "EC INT SRC should be 0x08, is %02x\n", val);

	//sch5545_multiple_write(0x10, 0x07, 7, 0);
	sch5545_emi_ec_write32(0x8000, 0x0e0010d);
	data[0] = 0xca229201;
	data[1] = 0x01038001;
	data[2] = 0x00000000;
	data[3] = 0x00000301;
	ec_write_32_bulk_with_int(0x8004, data, 4, 0x02);
	val = sch5545_emi_get_int_src_low();
	if (val != 0x08)
		printk(BIOS_INFO, "EC INT SRC should be 0x08, is %02x\n", val);

	printk(BIOS_DEBUG, "%s: addr %04x, val is %08x\n", __func__, 0x8000,
	       sch5545_emi_ec_read32(0x8000));
	printk(BIOS_DEBUG, "%s: addr %04x, val is %08x\n", __func__, 0x8004,
	       sch5545_emi_ec_read32(0x8004));
	printk(BIOS_DEBUG, "%s: addr %04x, val is %08x\n", __func__, 0x8008,
	       sch5545_emi_ec_read32(0x8008));
	printk(BIOS_DEBUG, "%s: addr %04x, val is %08x\n", __func__, 0x800c,
	       sch5545_emi_ec_read32(0x800c));


	send_mbox_msg_simple(0x05);
	val = 0x10;
	ec_read_write_reg(EC_HWM_LDN, 0x059f, &val, READ_OP);
}

void sch5545_ec_hwm_init(void *unused)
{
	uint8_t val, val_2fc, chassis_type, fan_target_offset;

	printk(BIOS_DEBUG, "%s\n", __func__);
	sch5545_emi_init(0x2e);

	/* From DxeHwmDriver */
	chassis_type = get_chassis_type();
	fan_target_offset = get_sku_temperature_target();

	ec_read_write_reg(EC_HWM_LDN, 0x0048, &val, READ_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x0048, &val, WRITE_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x0042, &val, READ_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x0048, &val, READ_OP);
	val |= 0x02;
	ec_read_write_reg(EC_HWM_LDN, 0x0048, &val, WRITE_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x0048, &val, READ_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x0048, &val, WRITE_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x0042, &val, READ_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x0048, &val, READ_OP);
	val |= 0x04;
	ec_read_write_reg(EC_HWM_LDN, 0x0048, &val, WRITE_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x0081, &val, READ_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x0027, &val, READ_OP);

	ec_check_mbox_and_int_status(0x00, 0x01);

	prepare_for_hwm_ec_sequence(0, &val_2fc);

	if (chassis_type != 0xff) {
		switch (chassis_type) {
		case 3:
			ec_hwm_init_late(ec_hwm_chassis3, 48);
			printk(BIOS_DEBUG, "Performing HWM init for chassis %d,"
			       " size %lx\n", chassis_type,
			       sizeof(ec_hwm_chassis3));
			break;
		case 4:
			ec_hwm_init_late(ec_hwm_chassis4, 56);
			printk(BIOS_DEBUG, "Performing HWM init for chassis %d,"
			       " size %lx\n", chassis_type,
			       sizeof(ec_hwm_chassis4));
			break;
		case 5:
			ec_hwm_init_late(ec_hwm_chassis3, 54);
			printk(BIOS_DEBUG, "Performing HWM init for chassis %d,"
			       " size %lx\n", chassis_type,
			       sizeof(ec_hwm_chassis5));
			break;
		case 6:
			ec_hwm_init_late(ec_hwm_chassis3, 59);
			printk(BIOS_DEBUG, "Performing HWM init for chassis %d,"
			       " size %lx\n", chassis_type,
			       sizeof(ec_hwm_chassis6));
			break;
		}
	}

	if (CONFIG_MAX_CPUS > 2) {
		val = 0x30;
		ec_read_write_reg(EC_HWM_LDN, 0x009e, &val, WRITE_OP);
		ec_read_write_reg(EC_HWM_LDN, 0x00ea, &val, READ_OP);
		printk(BIOS_DEBUG, "%s: more than 2 cores, 0xea reg is %02x\n",
		       __func__, val);
		ec_read_write_reg(EC_HWM_LDN, 0x00eb, &val, WRITE_OP);
	}

	ec_read_write_reg(EC_HWM_LDN, 0x02fc, &val_2fc, WRITE_OP);

	/* FIXME: check what the variable refers to in DxeHwmDriver */
	if (1) {
		ec_read_write_reg(EC_HWM_LDN, 0x0080, &val, READ_OP);
		val |= 0x60;
		ec_read_write_reg(EC_HWM_LDN, 0x0080, &val, WRITE_OP);
		ec_read_write_reg(EC_HWM_LDN, 0x0081, &val, READ_OP);
		val |= 0x60;
		ec_read_write_reg(EC_HWM_LDN, 0x0081, &val, WRITE_OP);
	}

	ec_read_write_reg(EC_HWM_LDN, 0x00b8, &val, READ_OP);

	if (chassis_type == 4 || chassis_type == 5) {
		ec_read_write_reg(EC_HWM_LDN, 0x00a0, &val, READ_OP);
		val &= 0xfb;
		ec_read_write_reg(EC_HWM_LDN, 0x00a0, &val, WRITE_OP);
		ec_read_write_reg(EC_HWM_LDN, 0x00a1, &val, READ_OP);
		val &= 0xfb;
		ec_read_write_reg(EC_HWM_LDN, 0x00a1, &val, WRITE_OP);
		ec_read_write_reg(EC_HWM_LDN, 0x00a2, &val, READ_OP);
		val &= 0xfb;
		ec_read_write_reg(EC_HWM_LDN, 0x00a2, &val, WRITE_OP);
		val = 0x99;
		ec_read_write_reg(EC_HWM_LDN, 0x008a, &val, WRITE_OP);
		val = 0x47;
		ec_read_write_reg(EC_HWM_LDN, 0x008b, &val, WRITE_OP);
		val = 0x91;
		ec_read_write_reg(EC_HWM_LDN, 0x008c, &val, WRITE_OP);
	}

	ec_read_write_reg(EC_HWM_LDN, 0x0049, &val, READ_OP);
	val &= 0xf7;
	ec_read_write_reg(EC_HWM_LDN, 0x0049, &val, WRITE_OP);

	val = 0x6a;
	if (chassis_type != 3)
		ec_read_write_reg(EC_HWM_LDN, 0x0059, &val, WRITE_OP);
	else
		ec_read_write_reg(EC_HWM_LDN, 0x0057, &val, WRITE_OP);

	ec_read_write_reg(EC_HWM_LDN, 0x0041, &val, READ_OP);
	val |= 0x40;
	ec_read_write_reg(EC_HWM_LDN, 0x0041, &val, WRITE_OP);

	if (chassis_type == 3) {
		ec_read_write_reg(EC_HWM_LDN, 0x0049, &val, READ_OP);
		val |= 0x04;
	} else {
		ec_read_write_reg(EC_HWM_LDN, 0x0049, &val, READ_OP);
		val |= 0x08;
	}
	ec_read_write_reg(EC_HWM_LDN, 0x0049, &val, WRITE_OP);

	val = 0x0e;
	ec_read_write_reg(EC_HWM_LDN, 0x007b, &val, WRITE_OP);
	ec_read_write_reg(EC_HWM_LDN, 0x007c, &val, WRITE_OP);
	val = 0x01;
	ec_read_write_reg(EC_HWM_LDN, 0x007a, &val, WRITE_OP);

	val = inb(SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN2);
	outb(val | 0x40, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN2);
	val = inb(SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN1);
	outb(val | 0x40, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN1);

	sch5545_emi_set_int_mask_low(sch5545_emi_get_int_mask_low() | 2);
}

void sch5545_ec_final(void *unused)
{
	uint8_t val;

	printk(BIOS_DEBUG, "%s\n", __func__);
	/* DirtyShutdownDxe */
	val = inb(SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN1);
	outb(val & 0xbf, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN1);

	ec_read_write_reg(EC_HWM_LDN, 0x000d, &val, READ_OP);
	printk(BIOS_DEBUG, "%s: reg 0xd is %02x\n", __func__, val);
	val = 0x00;
	ec_read_write_reg(EC_HWM_LDN, 0x000d, &val, WRITE_OP);
	val = 0x5c;
	ec_read_write_reg(EC_HWM_LDN, 0x000f, &val, WRITE_OP);
	val = 0x23;
	ec_read_write_reg(EC_HWM_LDN, 0x000f, &val, WRITE_OP);
	val = 0x14;
	ec_read_write_reg(EC_HWM_LDN, 0x000f, &val, WRITE_OP);
	val = 0x08;
	ec_read_write_reg(EC_HWM_LDN, 0x000f, &val, WRITE_OP);
	val = 0x01;
	ec_read_write_reg(EC_HWM_LDN, 0x000f, &val, WRITE_OP);
	val = 0xff;
	ec_read_write_reg(EC_HWM_LDN, 0x0358, &val, WRITE_OP);
	val = 0x08;
	ec_read_write_reg(EC_HWM_LDN, 0x000d, &val, WRITE_OP);

	outb(val, SCH5545_RUNTIME_REG_BASE + SCH5545_RR_SMI_EN1);

	val = 0x80;
	ec_read_write_reg(EC_HWM_LDN, 0x000f, &val, WRITE_OP);
}
