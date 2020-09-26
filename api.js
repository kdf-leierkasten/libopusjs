// Decoder

// create decoder
// channels and samplerate should match the encoder options
function Decoder(channels, samplerate) {
	this.dec = Module._Decoder_new.apply(null, arguments);
	this.channel_data = Module._ChannelData_new();
}

// free decoder memory
Decoder.prototype.destroy = function () {
	Module._Decoder_delete(this.dec);
	Module._ChannelData_delete(this.channel_data);
}

// add packet to the decoder buffer
// packet: Uint8Array
Decoder.prototype.input = function (packet) {
	var ptr = Module._malloc(packet.length * packet.BYTES_PER_ELEMENT);
	var pdata = new Uint8Array(Module.HEAPU8.buffer, ptr, packet.length * packet.BYTES_PER_ELEMENT);
	pdata.set(new Uint8Array(packet.buffer, packet.byteOffset, packet.length * packet.BYTES_PER_ELEMENT));

	Module._Decoder_input(this.dec, ptr, packet.length);
	Module._free(ptr);
}

// output the next decoded samples
// return samples (interleaved if multiple channels) as Int16Array (valid until the next output call) or null if there is no output
Decoder.prototype.output = function () {
	let ok = Module._Decoder_output(this.dec, this.channel_data);

	if (!ok) {
		return null;
	}

	let result = []
	for (let i = 0; i < Module._ChannelData_size(this.channel_data); i++) {
		let channel_ptr = Module._ChannelData_get(this.channel_data, i);
		result.push(new Float32Array(Module.HEAPF32.buffer, Module._Float32Array_data(channel_ptr), Module._Float32Array_size(channel_ptr)));
	}
	Module._ChannelData_clear(this.channel_data);

	return result;
}

//export objects
Module.Decoder = Decoder;

//make the module global if not using nodejs
if (Module["ENVIRONMENT"] !== "NODE")
	libopus = Module;
