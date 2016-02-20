#include "MPContext.h"
#include "MPDebug.h"
#include <algorithm>
#include <vector>

#define STREAM_NOT_FOUND 0xFFFF

namespace MP
{
	Context::Context()
	{
		m_begun = false;

		m_formatContext = 0;
		m_audioContext = 0;
		m_videoContext = 0;
	}

	bool Context::Begin(const std::string& filename)
	{
		Assert(m_begun == false);

		bool result = true;

		m_begun = true;
		m_filename = filename;
		m_eof = false;

		// Initialize libavcodec by registering all codecs.
		av_register_all();
		#if !defined(DEBUG)
		av_log_set_level(AV_LOG_QUIET);
		#endif

		// Display libavcodec info.
		Debug::Print("libavcodec: version: %x.", avcodec_version());
		Debug::Print("libavcodec: build: %x.", LIBAVCODEC_BUILD);

		// Open file using AV codec.
		if (av_open_input_file(&m_formatContext, filename.c_str(), 0, 0, 0) != 0)
		{
			Assert(0);
			return false;
		}

		size_t audioStreamIndex;
		size_t videoStreamIndex;

		if (GetStreamIndices(audioStreamIndex, videoStreamIndex) != true)
		{
			// TODO.
			Assert(0);
			return false;
		}

		if (audioStreamIndex != STREAM_NOT_FOUND)
		{
			// Initialize audio stream/context.
			m_audioContext = new AudioContext;
			m_audioContext->Initialize(this, audioStreamIndex);
		}

		if (videoStreamIndex != STREAM_NOT_FOUND)
		{
			// Initialize video stream/context.
			m_videoContext = new VideoContext;
			m_videoContext->Initialize(this, videoStreamIndex);
		}

		// Reset time.
		m_time = 0.0;

		return result;
	}

	bool Context::End()
	{
		Assert(m_begun == true);

		bool result = true;

		m_begun = false;

		// Destroy audio context.
		if (m_audioContext)
		{
			m_audioContext->Destroy();
			delete m_audioContext;
			m_audioContext = 0;
		}

		// Destroy video context.
		if (m_videoContext)
		{
			m_videoContext->Destroy();
			delete m_videoContext;
			m_videoContext = 0;
		}

		// Close file using AV codec.
		if (m_formatContext)
		{
			av_close_input_file(m_formatContext);
			m_formatContext = 0;
		}

		return result;
	}

	bool Context::HasAudioStream()
	{
		if (m_audioContext != 0)
			return true;
		else
			return false;
	}

	bool Context::HasVideoStream()
	{
		if (m_videoContext != 0)
			return true;
		else
			return false;
	}

	bool Context::HasReachedEOF()
	{
		return m_eof;
	}

	size_t Context::GetVideoWidth()
	{
		Assert(m_begun == true);

		if (m_videoContext)
			return m_videoContext->m_codecContext->width;
		else
			return 0;
	}

	size_t Context::GetVideoHeight()
	{
		Assert(m_begun == true);

		if (m_videoContext)
			return m_videoContext->m_codecContext->height;
		else
			return 0;
	}

	size_t Context::GetAudioFrameRate()
	{
		Assert(m_begun == true);

		if (m_audioContext)
			return m_audioContext->m_codecContext->sample_rate;
		else
			return 0;
	}

	size_t Context::GetAudioChannelCount()
	{
		Assert(m_begun == true);

		if (m_audioContext)
			return m_audioContext->m_codecContext->channels;
		else
			return 0;
	}

	bool Context::RequestAudio(int16_t* out_samples, size_t frameCount, bool& out_gotAudio)
	{
		Assert(m_begun == true);
		Assert(m_audioContext);

		bool result = true;

		if (m_audioContext->RequestAudio(out_samples, frameCount, out_gotAudio) != true)
			result = false;

		return result;
	}

	bool Context::RequestVideo(double time, VideoFrame** out_frame, bool& out_gotVideo)
	{
		Assert(m_begun == true);
		Assert(m_videoContext);

		bool result = true;

		if (m_videoContext->RequestVideo(time, out_frame, out_gotVideo) != true)
			result = false;

		return result;
	}

	bool Context::FillBuffers()
	{
		bool result = true;

		bool full = false;
		
		do
		{
			if (m_audioContext)
				if (m_audioContext->IsQueueFull())
					full = true;

			if (m_videoContext)
				if (m_videoContext->IsQueueFull())
					full = true;

			if (full == false)
			{
				if (NextPacket() != true)
				{
					result = false;
					Assert(0);
				}
			}
		} while (full == false && m_eof == false);

		return result;
	}

	bool Context::SeekToStart()
	{
		bool result = true;

		std::vector<size_t> streams;

		if (m_audioContext)
		{
			streams.push_back(m_audioContext->GetStreamIndex());
			m_audioContext->m_packetQueue.Clear();
			//m_audioContext->m_audioBuffer.Clear();
		}
		if (m_videoContext)
		{
			streams.push_back(m_videoContext->GetStreamIndex());
			m_videoContext->m_packetQueue.Clear();
			//m_videoContext->m_videoBuffer.Clear();
		}

		for (size_t i = 0; i < streams.size(); ++i)
		{
			if (av_seek_frame(m_formatContext, streams[i], 0, 0*AVSEEK_FLAG_BYTE | 0*AVSEEK_FLAG_ANY) < 0)
				result = false;
		}

		return result;
	}

	AVFormatContext* Context::GetFormatContext()
	{
		return m_formatContext;
	}

	bool Context::GetStreamIndices(size_t& out_audioStreamIndex, size_t& out_videoStreamIndex)
	{
		out_audioStreamIndex = STREAM_NOT_FOUND;
		out_videoStreamIndex = STREAM_NOT_FOUND;

		// Get stream info.
		if (av_find_stream_info(m_formatContext) < 0)
		{
			Assert(0);
			return false;
		}

		// Show stream info.
		dump_format(m_formatContext, 0, m_filename.c_str(), false);

		// Find the first audio & video streams and use those during rendering.
		for (size_t i = 0; i < static_cast<size_t>(m_formatContext->nb_streams); ++i)
		{
			if (out_audioStreamIndex == STREAM_NOT_FOUND)
				if (m_formatContext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
					out_audioStreamIndex = i;

			if (out_videoStreamIndex == STREAM_NOT_FOUND)
				if (m_formatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
					out_videoStreamIndex = i;
		}

		if (out_audioStreamIndex == STREAM_NOT_FOUND)
			Debug::Print("* No audio stream found.");

		if (out_videoStreamIndex == STREAM_NOT_FOUND)
			Debug::Print("* No video stream found.");

		return true;
	}

	bool Context::NextPacket()
	{
		bool result = true;

		// Read packet.
		AVPacket packet;

		if (ReadPacket(packet))
		{
			// Process packet.
			if (ProcessPacket(packet) != true)
				result = false;
		}
		else
		{
			// Couldn't read another packet- EOF reached.
			if (m_eof == false)
			{
				Debug::Print("Reached EOF.");

				m_eof = true;
			}
			else
			{
				Debug::Print("Attempt to read beyond EOF.");
			}
		}

		return result;
	}

	bool Context::ProcessPacket(AVPacket& packet)
	{
		bool result = true;

		if (m_audioContext != 0)
			if (packet.stream_index == m_audioContext->GetStreamIndex())
				m_audioContext->AddPacket(packet);

		if (m_videoContext != 0)
			if (packet.stream_index == m_videoContext->GetStreamIndex())
				m_videoContext->AddPacket(packet);

		// FIXME: Is this required?
		// Free packet.
		//av_free_packet(&packet);

		return result;
	}

	bool Context::ReadPacket(AVPacket& out_packet)
	{
		// Read packet.
		out_packet.data = 0;

		if (av_read_frame(m_formatContext, &out_packet) < 0)
			return false;

		if (out_packet.data == 0)
		{
			Assert(0);
			return false;
		}

		return true;
	}
};