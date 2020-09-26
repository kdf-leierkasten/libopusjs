#include <emscripten.h>
#include <opus.h>
#include <iostream>
#include <vector>
#include <algorithm>

typedef std::string String;
typedef std::vector<float> Float32Array;

class Decoder {
public:
	size_t channels;
	std::vector<Float32Array*> channel_data;

	Decoder(int _channels, long int _samplerate) : dec(NULL), samplerate(_samplerate), channels(_channels) {
        int err;
        dec = opus_decoder_create(samplerate, channels, &err);

        buffer_size = 120 / 1000.0 * samplerate * channels; // 120ms max
        buffer = Float32Array(buffer_size);

        channel_data = std::vector<Float32Array*>(channels);
        for (size_t i = 0; i < channels; i++) {
			channel_data[i] = new Float32Array();
        }

        if (dec == NULL)
            std::cerr << "[libopusjs] error while creating opus decoder (errcode " << err << ")" << std::endl;
    }

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
                size_t size = ret_size * channels;

				for (size_t channel_index = 0; channel_index < channels; channel_index++) {
					Float32Array* cbuf = channel_data[channel_index];
					cbuf->clear();
					cbuf->reserve(size + 1 / channels);
					for (int i = channel_index; i < size; i += channels) {
						cbuf->push_back(buffer[i]);
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

EMSCRIPTEN_KEEPALIVE const float *Float32Array_data(Float32Array* self) {
    return &(*self)[0];
}

EMSCRIPTEN_KEEPALIVE void Float32Array_delete(Float32Array* self) {
    delete self;
}

}

