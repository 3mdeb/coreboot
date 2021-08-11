#include <cpu/power/istep_8.h>
#include <cpu/power/scom.h>

static uint64_t avsCRCcalc(const uint32_t i_avs_cmd)
{
	//Polynomial = x^3 + x^1 + x^0 = 1*x^3 + 0*x^2 + 1*x^1 + 1*x^0
	//           = divisor(1011)
	uint32_t o_crc_value = 0;
	uint32_t l_polynomial = 0xB0000000;
	uint32_t l_msb = 0x80000000;

	o_crc_value = i_avs_cmd & 0xFFFFFFF8;

	while(o_crc_value & 0xFFFFFFF8)
	{
		if (o_crc_value & l_msb)
		{
			o_crc_value = o_crc_value ^ l_polynomial;
			l_polynomial = l_polynomial >> 1;
		}
		else
		{
			l_polynomial = l_polynomial >> 1;
		}
		l_msb = l_msb >> 1;
	}
	return o_crc_value;
}

static uint8_t avsValidateResponse(void)
{
	uint64_t l_data64 = read_scom(OCB_O2SRD0B);
	uint32_t l_rsp_data = ~PPC_BITMASK(0, 31) & l_data64;
	return (~PPC_BITMASK(0, 1) & l_data64) == 0
		&& (~PPC_BITMASK(29, 31) & l_data64) >> 32 == avsCRCcalc(l_rsp_data)
		&& l_rsp_data != 0
		&& l_rsp_data != 0xFFFFFFFF;
}

static void avsPollVoltageTransDone(void)
{
	for(size_t l_count = 0;
		l_count < 0x1000
		&& read_scom(OCB_O2SST0B) & PPC_BIT(0);
		++l_count);
}

static void avsDriveCommand(const uint64_t i_CmdType)
{
	write_scom(OCB_O2SCMD0B, PPC_BIT(1));
	uint64_t l_data64 = PPC_BIT(1);
	l_data64 |= (i_CmdType & 0x3) << 60;
	l_data64 &= ~(PPC_BIT(4) | PPC_BITMASK(9, 12));
	uint64_t l_data64WithoutCRC = l_data64 & PPC_BITMASK(0, 31);
	l_data64 |= (avsCRCcalc(l_data64WithoutCRC) & 0x7) << 32;
	write_scom(OCB_O2SWD0B, l_data64);
	avsPollVoltageTransDone();
}

static void avsIdleFrame(void)
{
	write_scom(OCB_O2SCMD0B, PPC_BIT(1));
	write_scom(OCB_O2SWD0B, 0xFFFFFFFFFFFFFFFF);
	for(size_t l_count = 0; l_count < 0x1000; ++l_count)
	{
		if ((read_scom(OCB_O2SST0B) & PPC_BIT(0)) == 0)
		{
			return;
		}
	}
}

static void p9_setup_evid(void)
{
	uint32_t l_present_voltage_mv;
	avsIdleFrame();
	for (size_t counter = 0; counter < 0x100; ++counter)
	{
		avsDriveCommand(AVS_READ);
		l_present_voltage_mv = (
			read_scom(OCB_O2SRD0B)
			& 0x00FFFF0000000000) >> 40;
		if(avsValidateResponse())
		{
			break;
		}
		avsIdleFrame();
	}
	for(size_t counter = 0; counter < 0x100; ++counter)
	{
		avsDriveCommand(AVS_WRITE);
		if(avsValidateResponse())
		{
			break;
		}
		avsIdleFrame();
	}
}

void istep_8_12(void)
{
	report_istep(8, 12);
	p9_setup_evid();
}
