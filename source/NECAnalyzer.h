#ifndef NEC_ANALYZER_H
#define NEC_ANALYZER_H

#include <Analyzer.h>
#include "NECAnalyzerResults.h"
#include "NECSimulationDataGenerator.h"

class NECAnalyzerSettings;
class ANALYZER_EXPORT NECAnalyzer : public Analyzer
{
public:
	NECAnalyzer();
	virtual ~NECAnalyzer();
	U16 GetWord();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

	enum pulse_type { NEC_LEADING_PULSE, NEC_BIT0, NEC_BIT1, NEC_REPEAT, NEC_STOP, NEC_ERROR, NEC_DATA, NEC_DATAERR, NEC_ADDR, NEC_ADDRERR };


protected: //vars
	
	bool InTolerance(U64 length, U64 expected, float tolerance);
	std::auto_ptr< NECAnalyzerSettings > mSettings;
	std::auto_ptr< NECAnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;

	NECSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;

	pulse_type GetPulseType();
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //NEC_ANALYZER_H
