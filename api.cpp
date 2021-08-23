#include <emscripten.h>
#include <opus.h>
#include <iostream>
#include <vector>
#include <algorithm>

#ifdef LIBOPUSJS_TIME
#include <iomanip>
#include <unordered_map>
#include <sanitizer/lsan_interface.h>
#endif

typedef std::vector<float> Float32Array;

class Decoder {
public:
	const size_t channels;
	size_t current_decoded_size;
	std::vector<Float32Array> channel_data;
	
	Decoder(int _channels, int32_t sample_rate) : dec(nullptr), channels(_channels) {
		int err;
		dec = opus_decoder_create(sample_rate, _channels, &err);
		if(dec == nullptr) {
			std::cerr << "[libopusjs] error while creating opus decoder (errcode " << err << ")" << std::endl;
		}
		
		const auto buffer_size = (120 * (unsigned long) sample_rate * channels) / 1000; // 120ms max
		buffer = Float32Array(buffer_size);
		channel_data = std::vector<Float32Array>(channels);
		current_decoded_size = 0;
	}

#ifdef LIBOPUSJS_TIME
	std::time_t clock = std::time(nullptr);
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::unordered_map<std::string, int64_t> micros;
	
	void measure(std::string name) {
		auto end = std::chrono::high_resolution_clock::now();
		auto time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
		if(micros.count(name) != 0) {
			micros[name] += time_taken;
		} else {
			micros.emplace(name, time_taken);
		}
		
		if(std::time(nullptr) - clock > 0) {
			uint64_t total = 0;
			for(auto &value : micros) {
				total += value.second;
			}
			
			for(auto &value : micros) {
				std::cout << std::fixed << std::setprecision(2) << value.first << ": " << value.second / 1e9 << "s (" << (double) value.second / total * 100 << "%) ";
			}
			std::cout << std::endl;
			clock = std::time(nullptr);
		}
		
		start = std::chrono::high_resolution_clock::now();
	}
#endif
	
	bool decode(const char *data, size_t size) {
		if(dec == nullptr || size == 0) {
			return false;
		}
		
		const auto ret_size = opus_decode_float(
				dec,
				(const unsigned char *) data,
				size,
				buffer.data(),
				buffer.size() / channels,
				0
		);
		
		if(ret_size < 0) {
			return false;
		}
		
		if(current_decoded_size != ret_size) {
			// Reinit the output arrays.
			current_decoded_size = ret_size;
			for(size_t channel_index = 0; channel_index < channels; channel_index++) {
				channel_data[channel_index].resize(ret_size, 0);
			}
		}
		
		for(size_t i = 0; i < ret_size; i++) {
			for(size_t channel_index = 0; channel_index < channels; channel_index++) {
				auto &channel_buffer = channel_data[channel_index];
				channel_buffer[i] = buffer[i * channels + channel_index];
			}
		}
		return true;
	}
	
	~Decoder() {
		if(dec) {
			opus_decoder_destroy(dec);
		}
	}

private:
	OpusDecoder *dec;
	Float32Array buffer;
};

extern "C" {
// Decoder

EMSCRIPTEN_KEEPALIVE Decoder *Decoder_new(int channels, long int sample_rate) {
	return new Decoder(channels, sample_rate);
}

EMSCRIPTEN_KEEPALIVE void Decoder_delete(Decoder *self) {
	delete self;
}

EMSCRIPTEN_KEEPALIVE bool Decoder_decode(Decoder *self, const char *data, size_t size) {
	return self->decode(data, size);
}

EMSCRIPTEN_KEEPALIVE Float32Array *Decoder_get_channel_data(Decoder *self, size_t index) {
	if(index >= self->channels) {
		return nullptr;
	}
	
	return &self->channel_data[index];
}

EMSCRIPTEN_KEEPALIVE size_t Decoder_get_channel_count(Decoder *self) {
	return self->channels;
}

// Float32Array

EMSCRIPTEN_KEEPALIVE size_t Float32Array_size(Float32Array *self) {
	return self->size();
}

EMSCRIPTEN_KEEPALIVE Float32Array *Float32Array_new() {
	return new std::vector<float>();
}

EMSCRIPTEN_KEEPALIVE const float *Float32Array_data(Float32Array *self) {
	return &(*self)[0];
}

EMSCRIPTEN_KEEPALIVE void Float32Array_delete(Float32Array *self) {
	delete self;
}

#ifdef LIBOPUSJS_TIME
//EMSCRIPTEN_KEEPALIVE void doLeakCheck() {
//	std::cout << "Running leak check..." << std::endl;
//	__lsan_do_recoverable_leak_check();
//}
#endif
}
