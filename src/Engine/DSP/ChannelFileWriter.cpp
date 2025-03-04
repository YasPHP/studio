/****************************************************************************
**
** Copyright 2019 neuromore co
** Contact: https://neuromore.com/contact
**
** Commercial License Usage
** Licensees holding valid commercial neuromore licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and neuromore. For licensing terms
** and conditions see https://neuromore.com/licensing. For further
** information use the contact form at https://neuromore.com/contact.
**
** neuromore Public License Usage
** Alternatively, this file may be used under the terms of the neuromore
** Public License version 1 as published by neuromore co with exceptions as 
** appearing in the file neuromore-class-exception.md included in the 
** packaging of this file. Please review the following information to 
** ensure the neuromore Public License requirements will be met: 
** https://neuromore.com/npl
**
****************************************************************************/

// include precompiled header
#include <Engine/Precompiled.h>

// include required files
#include "ChannelFileWriter.h"
#include "../Core/LogManager.h"
#include "Channel.h"

using namespace Core;


bool ChannelFileWriter::Write(EFormat format, const Array<Channel<double>*>& inChannels, FILE* outFile, int handle, double phyiscalMin, double phyiscalMax)
{
	LogInfo("Saving channels in %s format...", GetFormatName(format));

	// count samples and channels
	const uint32 numChannels = inChannels.Size();
	uint32 numSamples = 0;
	for (uint32 i = 0; i < numChannels; ++i)
		numSamples += inChannels[i]->GetNumSamples();

	LogDetailedInfo("Writing a total of %i samples in %i channels.", numSamples, numChannels);

	bool success = true;

	WriteHeader(format, inChannels, outFile, handle, phyiscalMin, phyiscalMax);

	LogDetailedInfo("Done.");

	return success;
}

bool ChannelFileWriter::WriteHeader(EFormat format, const Core::Array<Channel<double>*>& channels, FILE* file, int handle, double phyiscalMin, double phyiscalMax)
{
	// call the right write method
	switch (format)
	{
		// CSV
		case FORMAT_CSV_SIMPLE:
		case FORMAT_CSV_TIMESTAMP:
		{	
			const bool useTimestamps = (format == FORMAT_CSV_TIMESTAMP ? true : false);
			return WriteHeaderCSV(channels, useTimestamps, file);
		}
		case FORMAT_EDF_PLUS: 
		{
			return WriteHeaderEDF(channels, handle, phyiscalMin, phyiscalMax);
		}
		// RAW (no header)
		//case FORMAT_RAW_DOUBLE:	return true;
		default:	return false;
	}
}

bool ChannelFileWriter::WriteSamples(EFormat format, const Core::Array<Channel<double>*>& channels, uint32 numSamples, FILE* file, const int handle)
{
	// call the right write method
	switch (format)
	{
		// CSV
		case FORMAT_CSV_SIMPLE:
		case FORMAT_CSV_TIMESTAMP:
		{	
			const uint32 numDigits = 15;
			const bool useTimestamps = (format == FORMAT_CSV_TIMESTAMP ? true : false);
			return WriteSamplesCSV(channels, numSamples, useTimestamps, numDigits, file);
		}	
		case FORMAT_EDF_PLUS:
		{
			return WriteSamplesEDF(channels, numSamples, handle);
		}
		// RAW
		//case FORMAT_RAW_DOUBLE:
		//	return WriteSamplesRawDouble(channels, numSamples, file);

		default: return false;
	}
}


bool ChannelFileWriter::WriteHeaderCSV(const Array<Channel<double>*>& inChannels, bool useTimestamps, FILE* outFile)
{
	if (outFile == NULL)
		return true;

	const uint32 numChannels = inChannels.Size();

  // LINE 1: session timestamp and channelnames

	// use first column for current time (right now: string we get from server)
	if (useTimestamps == true)
	{
		mTempString = Time::Now().Format("%a %Y-%m-%d %H:%M:%S");
		fwrite(mTempString.AsChar(), 1, mTempString.GetLength(), outFile);
		fwrite(",", 1, 1, outFile);

	}
	// print names
	for (uint32 c = 0; c < numChannels; ++c)
	{
		mTempString = inChannels[c]->GetName();
		fwrite(mTempString.AsChar(), 1, mTempString.GetLength(), outFile);

		// write commas in between values
		if (c < numChannels - 1)
			fwrite(",", 1, 1, outFile);
	}

	// line end
	fwrite("\r\n", 1, 2, outFile);


  // LINE 2: samplerates 

	// use first column as identifier
	if (useTimestamps == true)
		fwrite("samplerate:,", 1, 12, outFile);

	// print samplerates
	for (uint32 c = 0; c < numChannels; ++c)
	{
		mTempString.Format("%.2fHz", inChannels[c]->GetSampleRate());
		fwrite(mTempString.AsChar(), 1, mTempString.GetLength(), outFile);

		// write commas in between values
		if (c < numChannels - 1)
			fwrite(",", 1, 1, outFile);
	}

	// line end
	fwrite("\r\n", 1, 2, outFile);

	return true;
}


bool ChannelFileWriter::WriteSamplesCSV(const Array<Channel<double>*>& inChannels, uint32 numSamples, bool useTimestamps, uint32 numDigits, FILE* outFile)
{
	// get the maximum number of samples in all the channels
	const uint32 numChannels = inChannels.Size();

	// make sure we have at least one channel 
	if (numChannels == 0)
		return false;

	//uint64 minNumSamples = 0;
	//for (uint32 c = 0; c < numChannels; ++c)
	//{
	//	Channel<double>* channel = inChannels[c];

	//	// TODO: skip channels that are not of the double type
	//	if (channel->GetType() != Channel<double>::TYPE_ID)
	//		return false;

	//	minNumSamples = Max<uint64>(minNumSamples, channel->GetNumSamples());
	//}

	//// nothing to do
	//if (numSamples == 0)
	//	return true;

	//// cant write that many samples
	//if (numSamples > minNumSamples || minNumSamples == 0)
	//	return false;


	// write samples
	for (uint32 i = 0; i < numSamples; ++i)
	{
		// write sample timestamp
		if (useTimestamps == true)
		{
			// NOTE we made sure that this vvvv array has at least one channel 
			const uint32 sampleIndex = inChannels[0]->GetSampleCounter() - numSamples + i;
			mTempString.Format("%.9f", inChannels[0]->GetSampleTime(sampleIndex).InSeconds());

			if (outFile != NULL)
			{
				fwrite(mTempString.AsChar(), 1, mTempString.GetLength(), outFile);
				fwrite(",", 1, 1, outFile);
			}
		}

		// write one sample per channel
		for (uint32 c = 0; c < numChannels; ++c)
		{
			Channel<double>* channel = inChannels[c];

			// calc sample index (count from the back)
			const uint32 sampleIndex = channel->GetSampleCounter() - numSamples + i;
			if (channel->IsValidSample(sampleIndex) == true)
			{
				mTempString.Format("%.*f", numDigits, channel->GetSample(sampleIndex));

				if (outFile != NULL)
					fwrite(mTempString.AsChar(), 1, mTempString.GetLength(), outFile);
			}
			// note: don't write a value if sample is not contained in channel

			// write commas in between values
			if (c < numChannels - 1)
			{
				if (outFile != NULL)
					fwrite(",", 1, 1, outFile);
			}

		}

		// line end
		if (outFile != NULL)
			fwrite("\r\n", 1, 2, outFile);
	}

	// success
	return true;
}

bool ChannelFileWriter::WriteSamplesRawDouble(const Core::Array<Channel<double>*>& inChannels, uint32 numSamples, FILE* outFile)
{
	// TODO implement me
	return false;
}

bool ChannelFileWriter::WriteHeaderEDF(const Core::Array<Channel<double>*>& inChannels, int handle, double phyiscalMin, double phyiscalMax)
{
	const uint32 NUMCHANNELS = inChannels.Size();

	// no valid edf handle, invalid channel count or invalid min/max
	if (handle < 0 || NUMCHANNELS == 0 || NUMCHANNELS > MAXCHANNELS || phyiscalMin >= phyiscalMax)
		return false;
	
	// must have identical sample rate on all channels (multichannels have)
	mSampleRate = (uint32_t)inChannels[0]->GetSampleRate();
	for (uint32 i = 1; i < NUMCHANNELS; ++i) {
		if ((uint32_t)inChannels[i]->GetSampleRate() != mSampleRate)
			return false;
	}
	
	// max samplerate in edfwrite here is 512hz
	if (mSampleRate > MAXSAMPLERATE)
		return false;
	
	// setup edf
	bool success = true;
	for (uint32 i = 0; i < NUMCHANNELS; ++i) {
		success = success && edf_set_label(handle, i, inChannels[i]->GetName()) == 0;
		success = success && edf_set_samplefrequency(handle, i, mSampleRate) == 0;
		success = success && edf_set_physical_dimension(handle, i, "uV") == 0;
		success = success && edf_set_physical_minimum(handle, i, phyiscalMin) == 0;
		success = success && edf_set_physical_maximum(handle, i, phyiscalMax) == 0;
		success = success && edf_set_digital_minimum(handle, i, -32768) == 0;
		success = success && edf_set_digital_maximum(handle, i, 32767) == 0;
	}

	// reset edf block buffer
	mSamples = 0;
	::memset(mBuffer, 0, sizeof(mBuffer));
	return success;
}


bool ChannelFileWriter::WriteSamplesEDF(const Core::Array<Channel<double>*>& inChannels, uint32 numSamples, const int handle)
{
	const uint32 NUMCHANNELS = inChannels.Size();

	if (handle < 0 || NUMCHANNELS == 0 || NUMCHANNELS > MAXCHANNELS || mSampleRate == 0)
		return false;

	if (numSamples == 0)
		return true;

	uint32_t processed = 0;
	while (processed < numSamples)
	{
		const uint32_t missing = mSampleRate - mSamples;
		const uint32_t copylen = std::min(missing, std::min((numSamples-processed), mSampleRate));
		for (uint32 i = 0; i < NUMCHANNELS; ++i) {
			auto* channel = inChannels[i];
			double* buf = &mBuffer[i*mSampleRate+mSamples];
			for (uint32_t j = 0; j < copylen; j++) {
				const uint32 IDX = channel->GetSampleCounter()-numSamples+processed+j;
				if (!channel->IsValidSample(IDX))
					return false;
				buf[j] = channel->GetSample(IDX);
			}
		}
		mSamples += copylen;
		processed += copylen;
		if (mSamples == mSampleRate) {
			mSamples = 0;
			if (edf_blockwrite_physical_samples(handle, mBuffer) != 0)
				return false;
		}
		else if (mSamples > mSampleRate)
			return false;
	}
	return true;
}


const char* ChannelFileWriter::GetFormatName(EFormat format)
{
	switch (format)
	{
		case FORMAT_CSV_SIMPLE:			return "CSV";
		case FORMAT_CSV_TIMESTAMP:		return "CSV with timestamps";
		//case FORMAT_RAW_DOUBLE:		return "Raw 64bit IEEE float";
		case FORMAT_EDF_PLUS:			return "EDF+";
		default:						return "";
	}
}


const char* ChannelFileWriter::GetFormatExtension(EFormat format)
{
	switch (format)
	{
		case FORMAT_CSV_SIMPLE:			return "csv";
		case FORMAT_CSV_TIMESTAMP:		return "csv";
		//case FORMAT_RAW_DOUBLE:		return "nmd";
		case FORMAT_EDF_PLUS:			return "edf";
		default:						return "";
	}
}
