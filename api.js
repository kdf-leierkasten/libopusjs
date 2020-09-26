// Decoder

// create decoder
// channels and samplerate should match the encoder options
function Decoder(channels, samplerate) {
	this.dec = Module._Decoder_new.apply(null, arguments);
}

// free decoder memory
Decoder.prototype.destroy = function() {
	Module._Decoder_delete(this.dec);
}

Decoder.prototype.decode = function(packet) {
	let ptr = Module._malloc(packet.length * packet.BYTES_PER_ELEMENT);
	let pdata = new Uint8Array(Module.HEAPU8.buffer, ptr, packet.length * packet.BYTES_PER_ELEMENT);
	pdata.set(new Uint8Array(packet.buffer, packet.byteOffset, packet.length * packet.BYTES_PER_ELEMENT));

	let ok = Module._Decoder_decode(this.dec, ptr, packet.length);
	Module._free(ptr);

	if (!ok) {
		return null;
	}

	let result = []
	let channels = Module._Decoder_get_channel_count(this.dec);
	for (let i = 0; i < channels; i++) {
		let channel_ptr = Module._Decoder_get_channel_data(this.dec, i);
		// Copy the decoded data into javscript Float32Array
		result.push(new Float32Array(Module.HEAPF32.buffer, Module._Float32Array_data(channel_ptr), Module._Float32Array_size(channel_ptr)));
	}

	return result;
}

//export objects
Module.Decoder = Decoder;

//make the module global if not using nodejs
if (Module["ENVIRONMENT"] !== "NODE")
	libopus = Module;
