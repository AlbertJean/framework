#pragma once

#include "binaural.h"

namespace binaural
{
	struct Binauralizer
	{
		struct SampleLocation
		{
			float elevation;
			float azimuth;
			
			SampleLocation()
				: elevation(0.f)
				, azimuth(0.f)
			{
			}
		};
		
		struct SampleBuffer
		{
			static const int kBufferSize = AUDIO_BUFFER_SIZE * 2;
			
			float samples[kBufferSize];
			int nextWriteIndex;
			int nextReadIndex;
			
			SampleBuffer()
				: nextWriteIndex(0)
				, nextReadIndex(0)
			{
			}
		};
		
		HRIRSampleSet * sampleSet;
		
		SampleBuffer sampleBuffer;
		
		float overlapBuffer[AUDIO_BUFFER_SIZE];
		
		SampleLocation sampleLocation;
		
		HRTF hrtfs[2];
		int nextHrtfIndex;

		AudioBuffer_Real audioBufferL;
		AudioBuffer_Real audioBufferR;
		int nextReadLocation;
		
		Mutex * mutex;
		
		Binauralizer()
			: sampleSet(nullptr)
			, sampleBuffer()
			, overlapBuffer()
			, sampleLocation()
			, hrtfs()
			, nextHrtfIndex(0)
			, audioBufferL()
			, audioBufferR()
			, nextReadLocation(AUDIO_BUFFER_SIZE)
			, mutex(nullptr)
		{
			memset(overlapBuffer, 0, sizeof(overlapBuffer));
			memset(hrtfs, 0, sizeof(hrtfs));
		}
		
		void init(HRIRSampleSet * _sampleSet, Mutex * _mutex)
		{
			sampleSet = _sampleSet;
			mutex = _mutex;
		}
		
		void setSampleLocation(const float elevation, const float azimuth)
		{
			mutex->lock();
			{
				sampleLocation.elevation = elevation;
				sampleLocation.azimuth = azimuth;
			}
			mutex->unlock();
		}
		
		void provide(const float * __restrict samples, const int numSamples)
		{
			int left = numSamples;
			int done = 0;
			
			while (left != 0)
			{
				if (sampleBuffer.nextWriteIndex == SampleBuffer::kBufferSize)
				{
					sampleBuffer.nextWriteIndex = 0;
				}
				
				const int todo = std::min(left, SampleBuffer::kBufferSize - sampleBuffer.nextWriteIndex);
				
				memcpy(sampleBuffer.samples + sampleBuffer.nextWriteIndex, samples + done, todo * sizeof(float));
				
				sampleBuffer.nextWriteIndex += todo;
				
				left -= todo;
				done += todo;
			}
		}
		
		void fillReadBuffer()
		{
			// move the old audio signal to the start of the overlap buffer
			
			memcpy(overlapBuffer, overlapBuffer + AUDIO_UPDATE_SIZE, (AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE) * sizeof(float));
			
			// generate audio signal
			
			float * __restrict samples = overlapBuffer + AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
			
			int left = AUDIO_UPDATE_SIZE;
			int done = 0;
			
			while (left != 0)
			{
				if (sampleBuffer.nextReadIndex == SampleBuffer::kBufferSize)
				{
					sampleBuffer.nextReadIndex = 0;
				}
				
				const int todo = std::min(left, SampleBuffer::kBufferSize - sampleBuffer.nextReadIndex);
				
				memcpy(samples + done, sampleBuffer.samples + sampleBuffer.nextReadIndex, todo * sizeof(float));
				
				sampleBuffer.nextReadIndex += todo;
				
				left -= todo;
				done += todo;
			}
			
			// compute the HRIR, a blend between three sample points in a Delaunay triangulation of all sample points
			
			HRIRSampleData hrir;
			
			{
				const HRIRSampleData * samples[3];
				float sampleWeights[3];
				
				float elevation;
				float azimuth;
				
				mutex->lock();
				{
					elevation = sampleLocation.elevation;
					azimuth = sampleLocation.azimuth;
				}
				mutex->unlock();
				
				if (sampleSet != nullptr && sampleSet->lookup_3(elevation, azimuth, samples, sampleWeights))
				{
					blendHrirSamples_3(samples, sampleWeights, hrir);
				}
				else
				{
					memset(&hrir, 0, sizeof(hrir));
				}
			}
			
			// compute the HRTF from the HRIR
			
			const HRTF & oldHrtf = hrtfs[1 - nextHrtfIndex];
			HRTF & newHrtf = hrtfs[nextHrtfIndex];
			nextHrtfIndex = (nextHrtfIndex + 1) % 2;
			
			hrirToHrtf(hrir.lSamples, hrir.rSamples, newHrtf.lFilter, newHrtf.rFilter);
			
			// prepare audio signal for HRTF application
			
			AudioBuffer audioBuffer;
			reverseSampleIndices(overlapBuffer, audioBuffer.real);
			memset(audioBuffer.imag, 0, AUDIO_BUFFER_SIZE * sizeof(float));
			
			// apply HRTF
			
			// convolve audio in the frequency domain
			
			AudioBuffer_Real oldAudioBufferL;
			AudioBuffer_Real oldAudioBufferR;
			
			AudioBuffer_Real newAudioBufferL;
			AudioBuffer_Real newAudioBufferR;
			
			convolveAudio_2(
				audioBuffer,
				oldHrtf.lFilter,
				oldHrtf.rFilter,
				newHrtf.lFilter,
				newHrtf.rFilter,
				oldAudioBufferL.samples,
				oldAudioBufferR.samples,
				newAudioBufferL.samples,
				newAudioBufferR.samples);
			
			// ramp from old to new audio buffer
			
			const int offset = AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
			
			rampAudioBuffers(oldAudioBufferL.samples + offset, newAudioBufferL.samples + offset, AUDIO_UPDATE_SIZE, audioBufferL.samples + offset);
			rampAudioBuffers(oldAudioBufferR.samples + offset, newAudioBufferR.samples + offset, AUDIO_UPDATE_SIZE, audioBufferR.samples + offset);
			
			nextReadLocation = AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
		}
		
		void generateInterleaved(
			float * __restrict samples,
			const int numSamples)
		{
			int left = numSamples;
			int done = 0;
			
			while (left != 0)
			{
				if (nextReadLocation == AUDIO_BUFFER_SIZE)
				{
					fillReadBuffer();
				}
				
				const int todo = std::min(left, AUDIO_BUFFER_SIZE - nextReadLocation);
				
			#if ENABLE_SSE
				debugAssert((todo % 4) == 0);
				
				const float4 * __restrict audioBufferL4 = (float4*)audioBufferL.samples + nextReadLocation / 4;
				const float4 * __restrict audioBufferR4 = (float4*)audioBufferR.samples + nextReadLocation / 4;
				float4 * __restrict samples4 = (float4*)samples;
				
				for (int i = 0; i < todo / 4; ++i)
				{
					const float4 l = audioBufferL4[i];
					const float4 r = audioBufferR4[i];
					
					const float4 interleaved1 = _mm_unpacklo_ps(l, r);
					const float4 interleaved2 = _mm_unpackhi_ps(l, r);
					
					samples4[i * 2 + 0] = interleaved1;
					samples4[i * 2 + 1] = interleaved2;
				}
			#else
				for (int i = 0; i < todo; ++i)
				{
					samples[i * 2 + 0] = audioBufferL.samples[nextReadLocation + i];
					samples[i * 2 + 1] = audioBufferR.samples[nextReadLocation + i];
				}
			#endif
				
				samples += todo * 2;
				
				nextReadLocation += todo;
				
				left -= todo;
				done += todo;
			}
		}
		
		void generateLR(
			float * __restrict samplesL,
			float * __restrict samplesR,
			const int numSamples)
		{
		#if 1
			int left = numSamples;
			int done = 0;
			
			while (left != 0)
			{
				if (nextReadLocation == AUDIO_BUFFER_SIZE)
				{
					fillReadBuffer();
				}
				
				const int todo = std::min(left, AUDIO_BUFFER_SIZE - nextReadLocation);
				
				memcpy(samplesL + done, audioBufferL.samples + nextReadLocation, todo * sizeof(float));
				memcpy(samplesR + done, audioBufferR.samples + nextReadLocation, todo * sizeof(float));
				
				nextReadLocation += todo;
				
				left -= todo;
				done += todo;
			}
		#else
			for (int i = 0; i < numSamples; ++i)
			{
				if (nextReadLocation == AUDIO_BUFFER_SIZE)
				{
					fillReadBuffer();
				}
				
				samplesL[i] = audioBufferL.samples[nextReadLocation];
				samplesR[i] = audioBufferR.samples[nextReadLocation];
				
				nextReadLocation++;
			}
		#endif
		}
	};
}
