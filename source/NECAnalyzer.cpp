#include "NECAnalyzer.h"
#include "NECAnalyzerSettings.h"
#include "NECConstants.h"
#include <AnalyzerChannelData.h>

NECAnalyzer::NECAnalyzer()
:	Analyzer(),  
	mSettings( new NECAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

NECAnalyzer::~NECAnalyzer()
{
	KillThread();
}

U16 NECAnalyzer::GetWord()
{
	U16 data = 0;
	
	for (int i = 0; i < 16; i++)
	{
		pulse_type pt = GetPulseType();
		data >>= 1;//shift it down!
		if (NEC_BIT1 == pt)
		{
			data |= 0x8000; //put a bit in the top
			mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel);
		}
		else
		{
			mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel);
		}
	}
	return data;
}


void NECAnalyzer::WorkerThread()
{
	NECAnalyzer::pulse_type pulse;
	pulse = NEC_LEADING_PULSE;
	mResults.reset( new NECAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );

	mSampleRateHz = GetSampleRate();

	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );

	U32 samples_per_tick = mSampleRateHz / NEC_CARRIER_FREQ_HZ;

	//if( mSerial->GetBitState() == BIT_HIGH )
	//get to first edge
	mSerial->AdvanceToNextEdge();
	
	for( ; ; )
	{

		U64 starting_sample = mSerial->GetSampleNumber();
		NECAnalyzer::pulse_type pt = GetPulseType();
		Frame frame;

		if (pt == NEC_LEADING_PULSE)
		{
			frame.mData1 = (U64)pt;
			frame.mFlags = 0;
			frame.mStartingSampleInclusive = starting_sample;
			frame.mEndingSampleInclusive = mSerial->GetSampleNumber();

			mResults->AddFrame(frame);
			mResults->CommitResults();

			//we have found a leading pulse now expecting 16 addr bits
			U64 addrstart_sample = mSerial->GetSampleNumber()+1; //can't overlap frames
			U16 addr = GetWord();
			//check parity
			if ((addr & 0xFF) == ((~addr >> 8) & 0xFF))
			{
				frame.mData1 = (U64)NEC_ADDR;
				frame.mData2 = addr & 0xFF;
			}
			else
			{
				frame.mData1 = (U64)NEC_ADDRERR;
				frame.mData2 = addr;
			}
			
			frame.mFlags = 0;
			frame.mStartingSampleInclusive = addrstart_sample;
			frame.mEndingSampleInclusive = mSerial->GetSampleNumber();
			mResults->AddFrame(frame);
			mResults->CommitResults();


			//Data
			U64 datastart_sample = mSerial->GetSampleNumber() + 1; //can't overlap frames
			U16 data = GetWord();
			if ((data & 0xFF) == ((~data >> 8) & 0xFF))
			{
				frame.mData1 = (U64)NEC_DATA;
				frame.mData2 = data & 0xFF;
			}
			else
			{
				frame.mData1 = (U64)NEC_DATAERR;
				frame.mData2 = data;
			}

			frame.mFlags = 0;
			frame.mStartingSampleInclusive = datastart_sample;
			frame.mEndingSampleInclusive = mSerial->GetSampleNumber();
			mResults->AddFrame(frame);
			mResults->CommitResults();

			datastart_sample = mSerial->GetSampleNumber() + 1;//can't overlap
			//expecting STOP
			pt = GetPulseType();
			if (pt == NEC_STOP)
			{
				frame.mData1 = (U64)NEC_STOP;
			}
			else
			{
				frame.mData1 = (U64)NEC_ERROR;
			}
			frame.mFlags = 0;
			frame.mStartingSampleInclusive = datastart_sample;
			frame.mEndingSampleInclusive = mSerial->GetSampleNumber();
			mResults->AddFrame(frame);
			mResults->CommitResults();
		}
		else if (pt == NEC_REPEAT)
		{
			frame.mData1 = (U64)NEC_REPEAT;
			frame.mFlags = 0;
			frame.mStartingSampleInclusive = starting_sample;
			frame.mEndingSampleInclusive = mSerial->GetSampleNumber();
			mResults->AddFrame(frame);
			mResults->CommitResults();
		}

		//for( U32 i=0; i<8; i++ )
		//{
		//	//let's put a dot exactly where we sample this bit:
		//	mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );

		//	if( mSerial->GetBitState() == BIT_HIGH )
		//		data |= mask;

		//	mSerial->Advance( samples_per_bit );

		//	mask = mask >> 1;
		//}


		//we have a byte to save. 
		





		ReportProgress( frame.mEndingSampleInclusive );
	}
}

bool NECAnalyzer::NeedsRerun()
{
	return false;
}

bool NECAnalyzer::InTolerance(U64 actual, U64 expected, float tolerance_pc)
{
	//length < expected * 1.1
	// and length > expected * 0.9
	U64 fraction = (U64)((float)expected * tolerance_pc);
	return (actual < (expected + fraction)) && (actual > (expected - fraction));
}

NECAnalyzer::pulse_type NECAnalyzer::GetPulseType()
{
	//work out what sort of pulse we see.
	//pulse is defined by two things:
	// carrier length
	// space length
	U64 carrierstart = mSerial->GetSampleNumber();
	U64 twoticks = (2 * mSampleRateHz) / NEC_CARRIER_FREQ_HZ;
	while (1)
	{
		U64 startSample = mSerial->GetSampleNumber();
		U64 nextedge = mSerial->GetSampleOfNextEdge();
		//either we go around again, or we return and we want to be at the start next time
		mSerial->AdvanceToNextEdge(); 
		if ((nextedge - startSample) > twoticks)
		{
			//if we are within two ticks, we are definitely another carrier
			//so if it's more, we have likely reached the blank, 
			//we now have the info to work out what pulse it is

			U64 ontime_us = ((startSample - carrierstart) * 1e6) / mSampleRateHz;
			U64 offtime_us = ((nextedge - startSample) * 1e6) / mSampleRateHz;

			//given these could be a little off from perfect, we'll allow a tolerance

			//carrier is either a leader or bit. 
			//Repeats have leader and different space followed by another pulse
			if (InTolerance(ontime_us, NEC_LEADER_ON_TIME_us, NEC_DECODE_TOLERANCE_pc))
			{
				if (InTolerance(offtime_us, NEC_LEADER_OFF_TIME_us, NEC_DECODE_TOLERANCE_pc))
				{
					return NEC_LEADING_PULSE;
				}
				else if (InTolerance(offtime_us, NEC_REPEAT_OFF_TIME_us, NEC_DECODE_TOLERANCE_pc))
				{ 
					//recurse to check for a stop bit. 
					if (GetPulseType() == NEC_STOP)
					{
						return NEC_REPEAT;
					}
					
				}
			}
			if (InTolerance(ontime_us, NEC_BIT_ON_TIME_us, NEC_DECODE_TOLERANCE_pc))
			{
				//could be bit 0 or 1
				if (InTolerance(offtime_us, NEC_BIT0_OFF_TIME_us, NEC_DECODE_TOLERANCE_pc))
				{
					return NEC_BIT0;
				}
				else if (InTolerance(offtime_us, NEC_BIT1_OFF_TIME_us, NEC_DECODE_TOLERANCE_pc))
				{
					return NEC_BIT1;
				}
				else
				{
					return NEC_STOP;
				}
			}
			return NEC_ERROR;
		}

	}
}


U32 NECAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 NECAnalyzer::GetMinimumSampleRateHz()
{
	return NEC_CARRIER_FREQ_HZ * 4;//go four times
}

const char* NECAnalyzer::GetAnalyzerName() const
{
	return "NEC";
}

const char* GetAnalyzerName()
{
	return "NEC";
}

Analyzer* CreateAnalyzer()
{
	return new NECAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}