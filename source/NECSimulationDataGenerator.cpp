#include "NECSimulationDataGenerator.h"
#include "NECAnalyzerSettings.h"
#include "NECConstants.h"
#include <AnalyzerHelpers.h>


NECSimulationDataGenerator::NECSimulationDataGenerator()
:	mSerialText( "My first analyzer, woo hoo!" ),
	mStringIndex( 0 )
{
}

NECSimulationDataGenerator::~NECSimulationDataGenerator()
{
}

void NECSimulationDataGenerator::Initialize( U32 simulation_sample_rate, NECAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mSerialSimulationData.SetChannel( mSettings->mInputChannel );
	mSerialSimulationData.SetSampleRate( simulation_sample_rate );
	mSerialSimulationData.SetInitialBitState( BIT_LOW );
}

U32 NECSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mSerialSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		CreateLeaderCode();
		SendByte(0x11);
		SendByte(0x30);
		SendEnd();
		SendRepeat(true);
		SendRepeat(false);
		//CreateSerialByte();
	}

	*simulation_channel = &mSerialSimulationData;
	return 1;
}

//! Transmits either a the carrier frequency or blank space.
void NECSimulationDataGenerator::NEC_Transmit(U32 time_us, bool carrier)
{
	ClockGenerator c;
	c.Init(NEC_CARRIER_FREQ_HZ, mSimulationSampleRateHz);
	
	U32 number_of_ticks = NEC_CARRIER_FREQ_HZ * time_us / 1E6;
	for (U32 i = 0; i<number_of_ticks; i++)
	{
		if (carrier)
		{
			mSerialSimulationData.TransitionIfNeeded(BIT_HIGH);
		}
		
		mSerialSimulationData.Advance(c.AdvanceByHalfPeriod(0.5));
		mSerialSimulationData.TransitionIfNeeded(BIT_LOW);
		mSerialSimulationData.Advance(c.AdvanceByHalfPeriod(0.5));
	}
}

void NECSimulationDataGenerator::SendRepeat(bool firstrepeat)
{
	//repeats should happen 108ms after the start of the 
	ClockGenerator c;
	c.Init(NEC_CARRIER_FREQ_HZ, mSimulationSampleRateHz);

	//advance by 108-67.5ms = 40.5ms
	if (firstrepeat)
	{
		mSerialSimulationData.Advance(c.AdvanceByTimeS(40.5e-3));
	}
	else
	{
		//repeat repeat
		mSerialSimulationData.Advance(c.AdvanceByTimeS(96.19e-3));
	}
	NEC_Transmit(NEC_LEADER_ON_TIME_us, true);
	NEC_Transmit(NEC_REPEAT_OFF_TIME_us, false);
	SendEnd();

}

void NECSimulationDataGenerator::SendEnd()
{
	ClockGenerator c;
	c.Init(NEC_CARRIER_FREQ_HZ, mSimulationSampleRateHz);

	//carrier pulse is same for Bit 0 and Bit 1
	NEC_Transmit(NEC_BIT_ON_TIME_us, true);

}

void NECSimulationDataGenerator::SendBit(bool bit)
{
	ClockGenerator c;
	c.Init(NEC_CARRIER_FREQ_HZ, mSimulationSampleRateHz);

	//carrier pulse is same for Bit 0 and Bit 1
	NEC_Transmit(NEC_BIT_ON_TIME_us, true);

	if (bit)
	{
		NEC_Transmit(NEC_BIT1_OFF_TIME_us, false);
	}
	else
	{
		NEC_Transmit(NEC_BIT0_OFF_TIME_us, false);
	}
}

void NECSimulationDataGenerator::CreateLeaderCode()
{
	U32 samples_per_bit = mSimulationSampleRateHz / NEC_CARRIER_FREQ_HZ;
	//leave 1ms at the beginning for gap
	ClockGenerator c;
	c.Init(NEC_CARRIER_FREQ_HZ, mSimulationSampleRateHz);
	mSerialSimulationData.Advance(c.AdvanceByTimeS(0.01));
	mSerialSimulationData.TransitionIfNeeded(BIT_LOW);
	//mSerialSimulationData.Advance(c.AdvanceByTimeS(1E-3));
	U8 byte = mSerialText[mStringIndex];
	mStringIndex++;
	if (mStringIndex == mSerialText.size())
		mStringIndex = 0;

	//we're currenty high
	//let's move forward a little
	
	NEC_Transmit(NEC_LEADER_ON_TIME_us, true);
	NEC_Transmit(NEC_LEADER_OFF_TIME_us, false);

}
void NECSimulationDataGenerator::SendByte(const U8 byte)
{
	U8 data;
	data = byte;

	U8 mask = 0x1;
	for (U32 i = 0; i < 8; i++)
	{
		if ((data & mask) != 0)
			SendBit(true);
		else
			SendBit(false);
		mask = mask << 1;
	}

	data = ~byte;
	mask = 0x1;
	for (U32 i = 0; i < 8; i++)
	{
		if ((data & mask) != 0)
			SendBit(true);
		else
			SendBit(false);
		mask = mask << 1;
	}
	
	mSerialSimulationData.TransitionIfNeeded( BIT_LOW ); //we need to end high


}
