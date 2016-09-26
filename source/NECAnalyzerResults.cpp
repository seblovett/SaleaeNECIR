#include "NECAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "NECAnalyzer.h"
#include "NECAnalyzerSettings.h"
#include <iostream>
#include <fstream>

NECAnalyzerResults::NECAnalyzerResults( NECAnalyzer* analyzer, NECAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

NECAnalyzerResults::~NECAnalyzerResults()
{
}

void NECAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	char number_str[128];
	//AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	switch ((NECAnalyzer::pulse_type)frame.mData1)
	{
	case NECAnalyzer::NEC_LEADING_PULSE:
		AddResultString("Leading Pulse");
		break;
	case NECAnalyzer::NEC_BIT1:
		AddResultString("1");
		break;
	case NECAnalyzer::NEC_BIT0:
		AddResultString("0");
		break;
	case NECAnalyzer::NEC_STOP:
		AddResultString("Stop");
		break;
	case NECAnalyzer::NEC_REPEAT:
		AddResultString("Repeat");
		break;
	case NECAnalyzer::NEC_DATA:
		
		AnalyzerHelpers::GetNumberString(frame.mData2, display_base, 8, number_str, 128);
		AddResultString("Data: ", number_str);
		break;
	case NECAnalyzer::NEC_DATAERR:
		AnalyzerHelpers::GetNumberString(frame.mData2, display_base, 8, number_str, 128);
		AddResultString("Data Error: ", number_str);
		break;
	case NECAnalyzer::NEC_ADDR:

		AnalyzerHelpers::GetNumberString(frame.mData2, display_base, 8, number_str, 128);
		AddResultString("Address: ", number_str);
		break;
	case NECAnalyzer::NEC_ADDRERR:
		AnalyzerHelpers::GetNumberString(frame.mData2, display_base, 8, number_str, 128);
		AddResultString("Address Error: ", number_str);
		break;
	default:
		AddResultString("Err");
		
		break;
	}
}

void NECAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		file_stream << time_str << "," << number_str << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void NECAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );
	ClearResultStrings();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	AddResultString( number_str );
}

void NECAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void NECAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}