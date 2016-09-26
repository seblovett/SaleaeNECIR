#ifndef NEC_SIMULATION_DATA_GENERATOR
#define NEC_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class NECAnalyzerSettings;

class NECSimulationDataGenerator
{
public:
	NECSimulationDataGenerator();
	~NECSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, NECAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	NECAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void SendByte(U8 byte);
	void CreateLeaderCode();
	void NEC_Transmit(U32 time_s, bool carrier);
	void SendRepeat(bool firstrepeat);
	void SendEnd();
	void SendBit(bool bit);
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //NEC_SIMULATION_DATA_GENERATOR