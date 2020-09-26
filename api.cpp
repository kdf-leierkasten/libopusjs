#include <emscripten.h>
#include <opus.h>
#include <iostream>
#include <vector>
#include <algorithm>

typedef std::string String;
typedef std::vector<float> Float32Array;
typedef std::vector<Float32Array*> ChannelData;

class Decoder {
public:
    Decoder(int _channels, long int _samplerate) : dec(NULL), samplerate(_samplerate), channels(_channels) {
        int err;
        dec = opus_decoder_create(samplerate, channels, &err);

        buffer_size = 120 / 1000.0 * samplerate * channels; // 120ms max
        buffer = Float32Array(buffer_size);

        channel_buffers = std::vector<Float32Array*>(channels);
        for (size_t i = 0; i < channels; i++) {
			channel_buffers[i] = new Float32Array();
        }

        if (dec == NULL)
            std::cerr << "[libopusjs] error while creating opus decoder (errcode " << err << ")" << std::endl;
    }

    void input(const char *data, size_t size) {
        packets.emplace_back(data, size);
    }

    bool output(ChannelData* channel_data) {
        bool ok = false;

        if (packets.size() > 0 && dec != NULL) {
            int ret_size = 0;

            //extract samples from packets
            std::string &packet = packets[0];

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
					Float32Array* cbuf = channel_buffers[channel_index];
					cbuf->clear();
					cbuf->reserve(size + 1 / channels);
					for (int i = channel_index; i < size; i += channels) {
						cbuf->push_back(buffer[i]);
					}
					channel_data->push_back(cbuf);
                }

                ok = true;
            }

            packets.erase(packets.begin());
        }

        return ok;
    }

    ~Decoder() {
        if (dec) {
			opus_decoder_destroy(dec);
		}

        for (int i = 0; i < channel_buffers.size(); i++) {
        	delete channel_buffers[i];
        }
    }

private:
    long int samplerate;
    int channels;
    OpusDecoder *dec;
    std::vector<std::string> packets;
    long int buffer_size;
    Float32Array buffer;
    std::vector<Float32Array*> channel_buffers;
};

extern "C" {
// Decoder

EMSCRIPTEN_KEEPALIVE Decoder* Decoder_new(int channels, long int samplerate) {
    return new Decoder(channels, samplerate);
}

EMSCRIPTEN_KEEPALIVE void Decoder_delete(Decoder * self) {
    delete self;
}

EMSCRIPTEN_KEEPALIVE void Decoder_input(Decoder * self, const char *data, size_t size) {
	self->input(data, size);
}

EMSCRIPTEN_KEEPALIVE bool Decoder_output(Decoder* self, ChannelData* channel_data) {
    return self->output(channel_data);
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

// ChannelData
EMSCRIPTEN_KEEPALIVE ChannelData* ChannelData_new() {
	return new std::vector<Float32Array*>();
}

EMSCRIPTEN_KEEPALIVE size_t ChannelData_size(ChannelData* self) {
	return self->size();
}

EMSCRIPTEN_KEEPALIVE void ChannelData_clear(ChannelData* self) {
return self->clear();
}

EMSCRIPTEN_KEEPALIVE Float32Array* ChannelData_get(ChannelData* self, size_t index) {
	if (index >= self->size()) {
		return nullptr;
	}

	return (*self)[index];
}

EMSCRIPTEN_KEEPALIVE void ChannelData_delete(ChannelData* self) {
	delete self;
}

}

