#include <emscripten.h>
#include <opus.h>
#include <iostream>
#include <vector>
#include <algorithm>
//#include <iomanip>
//#include <unordered_map>
//#include <sanitizer/lsan_interface.h>

typedef std::string String;
typedef std::vector<float> Float32Array;

class Decoder {
public:
	size_t channels;
	size_t current_decoded_size;
	std::vector<Float32Array*> channel_data;

	Decoder(int _channels, long int _samplerate) : dec(NULL), samplerate(_samplerate), channels(_channels) {
        int err;
        dec = opus_decoder_create(samplerate, channels, &err);

        buffer_size = 120 / 1000.0 * samplerate * channels; // 120ms max
        buffer = Float32Array(buffer_size);
        channel_data = std::vector<Float32Array*>(channels);
        current_decoded_size = 0;

        if (dec == NULL)
            std::cerr << "[libopusjs] error while creating opus decoder (errcode " << err << ")" << std::endl;
    }

//	std::time_t clock = std::time(nullptr);
//	std::chrono::time_point<std::chrono::high_resolution_clock> start;
//	std::unordered_map<std::string, int64_t> micros;
//	void measure(std::string name) {
//		auto end = std::chrono::high_resolution_clock::now();
//		auto time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
//		if (micros.count(name) != 0) {
//			micros[name] += time_taken;
//		} else {
//			micros.emplace(name, time_taken);
//		}
//
//		if (std::time(nullptr) - clock > 0) {
//			uint64_t total = 0;
//			for (auto& value : micros) {
//				total += value.second;
//			}
//
//			for (auto& value : micros) {
//				std::cout << std::fixed << std::setprecision(2) << value.first << ": " << value.second / 1e9 << "s (" << (double) value.second / total * 100  << "%) ";
//			}
//			std::cout << std::endl;
//			clock = std::time(nullptr);
//		}
//
//		start = std::chrono::high_resolution_clock::now();
//	}

    bool decode(const char* data, size_t size) {
        bool ok = false;

        if (dec != NULL) {
            int ret_size = 0;
            auto packet = std::string(data, size);

			if (packet.size() > 0)
                ret_size = opus_decode_float(
					dec,
					(const unsigned char *) packet.c_str(),
					packet.size(),
					buffer.data(),
					buffer_size / channels,
					0
                );

            if (ret_size > 0) {
                if (current_decoded_size != ret_size) {
                	// Reinit the output arrays.
                	current_decoded_size = ret_size;
					for (size_t channel_index = 0; channel_index < channels; channel_index++) {
						channel_data[channel_index] = new Float32Array(ret_size, 0);
					}
                }

				for (size_t channel_index = 0; channel_index < channels; channel_index++) {
					Float32Array& channel_buffer = *channel_data[channel_index];
					for (size_t i = 0; i < ret_size; i++) {
						channel_buffer[i] = buffer[i * 2 + channel_index];
					}
				}

                ok = true;
            }
        }

        return ok;
    }

    ~Decoder() {
        if (dec) {
			opus_decoder_destroy(dec);
		}

        for (int i = 0; i < channel_data.size(); i++) {
        	delete channel_data[i];
        }
    }

private:
    long int samplerate;
    OpusDecoder *dec;
    long int buffer_size;
    Float32Array buffer;
};

extern "C" {
// Decoder

EMSCRIPTEN_KEEPALIVE Decoder* Decoder_new(int channels, long int samplerate) {
    return new Decoder(channels, samplerate);
}

EMSCRIPTEN_KEEPALIVE void Decoder_delete(Decoder * self) {
    delete self;
}

EMSCRIPTEN_KEEPALIVE bool Decoder_decode(Decoder* self, const char *data, size_t size) {
    return self->decode(data, size);
}

EMSCRIPTEN_KEEPALIVE Float32Array* Decoder_get_channel_data(Decoder* self, size_t index) {
	if (index >= self->channels) {
		return nullptr;
	}

	return self->channel_data[index];
}

EMSCRIPTEN_KEEPALIVE size_t Decoder_get_channel_count(Decoder* self) {
	return self->channels;
}

// Float32Array

EMSCRIPTEN_KEEPALIVE size_t Float32Array_size(Float32Array *self) {
	return self->size();
}

EMSCRIPTEN_KEEPALIVE Float32Array* Float32Array_new() {
    return new std::vector<float>();
}

EMSCRIPTEN_KEEPALIVE const float* Float32Array_data(Float32Array* self) {
    return &(*self)[0];
}

EMSCRIPTEN_KEEPALIVE void Float32Array_delete(Float32Array* self) {
    delete self;
}

//EMSCRIPTEN_KEEPALIVE void doLeakCheck() {
//	std::cout << "Running leak check..." << std::endl;
//	__lsan_do_recoverable_leak_check();
//}

}

