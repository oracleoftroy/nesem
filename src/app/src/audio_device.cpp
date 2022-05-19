#include "ui/audio_device.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <SDL2/SDL_audio.h>
#include <util/logging.hpp>

namespace ui
{
	class AudioDevice::AudioCore final
	{
	public:
		AudioCore(SDL_AudioSpec &spec, bool &success) noexcept
		{
			// TODO: we can open this in the create function
			device = SDL_OpenAudioDevice(nullptr, false, &spec, &this->spec, 0);
			success = device > 0;

			LOG_INFO("Got audio: {} Hz, {} channels, {}-bit {}", this->spec.freq, this->spec.channels, SDL_AUDIO_BITSIZE(this->spec.format),
				SDL_AUDIO_ISFLOAT(this->spec.format)        ? "float"
					: SDL_AUDIO_ISSIGNED(this->spec.format) ? "signed int"
															: "unsigned int");
		}

		void pause(bool value) noexcept
		{
			SDL_PauseAudioDevice(device, value);
		}

		void queue_audio(std::span<float> samples) noexcept
		{
			auto sample_size = size(samples) * sizeof(samples[0]);
			CHECK(std::in_range<Uint32>(sample_size), "too many samples in buffer, truncating");

			if (auto result = SDL_QueueAudio(device, data(samples), static_cast<Uint32>(sample_size));
				result != 0)
				LOG_ERROR("Problem queuing audio: {}", SDL_GetError());
		}

	private:
		SDL_AudioSpec spec;
		SDL_AudioDeviceID device;
	};

	AudioDevice AudioDevice::create(int frequency, int channels, int sample_size) noexcept
	{
		CHECK(std::in_range<Uint8>(channels), "Too many channels, truncating");
		CHECK(std::in_range<Uint16>(sample_size), "sample_size too big, truncating");

		auto spec = SDL_AudioSpec{
			.freq = frequency,
			.format = AUDIO_F32SYS,
			.channels = static_cast<Uint8>(channels),
			.samples = static_cast<Uint16>(sample_size),
		};

		bool opened_successfully = false;
		auto core = std::make_unique<AudioCore>(spec, opened_successfully);

		if (!core || !opened_successfully)
		{
			LOG_WARN("Failed to open audio device: {}", SDL_GetError());
			return {};
		}

		return AudioDevice(std::move(core));
	}

	AudioDevice::operator bool() noexcept
	{
		return static_cast<bool>(core);
	}

	void AudioDevice::pause(bool value) noexcept
	{
		core->pause(value);
	}

	void AudioDevice::queue_audio(std::span<float> samples) noexcept
	{
		core->queue_audio(samples);
	}

	AudioDevice::AudioDevice(std::unique_ptr<AudioCore> &&core) noexcept
		: core(std::move(core))
	{
	}

	AudioDevice::AudioDevice() noexcept = default;
	AudioDevice::~AudioDevice() = default;
	AudioDevice::AudioDevice(AudioDevice &&other) noexcept = default;
	AudioDevice &AudioDevice::operator=(AudioDevice &&other) noexcept = default;
}
