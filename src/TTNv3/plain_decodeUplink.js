// Decoder for device payload encoder "PLAIN"
// copy&paste to TTN Console V3 -> Applications -> Payload formatters -> Uplink -> Javascript
// modified for The Things Stack V3 by Caspar Armster, dasdigidings e.V.

function decodeUplink(input) {
    var data = {};

    if (input.fPort === 1) {
        var i = 0;

        if (input.bytes.length >= 2) {
            data.wifi = (input.bytes[i++] << 8) | input.bytes[i++];
        }
     
        if (input.bytes.length === 4 || input.bytes.length > 15) {
            data.ble = (input.bytes[i++] << 8) | input.bytes[i++];
        }

        if (input.bytes.length > 4) {
            data.latitude = ((input.bytes[i++] << 24) | (input.bytes[i++] << 16) | (input.bytes[i++] << 8) | input.bytes[i++]);
            data.longitude = ((input.bytes[i++] << 24) | (input.bytes[i++] << 16) | (input.bytes[i++] << 8) | input.bytes[i++]);
            data.sats = input.bytes[i++];
            data.hdop = (input.bytes[i++] << 8) | (input.bytes[i++]);
            data.altitude = ((input.bytes[i++] << 8) | (input.bytes[i++]));
        }
        data.pax = 0;
        if ('wifi' in data) {
            data.pax += data.wifi;
        }
        if ('ble' in data) {
            data.pax += data.ble;
        } 
    }

    if (input.fPort === 2) {
        var i = 0;
        data.voltage = ((input.bytes[i++] << 8) | input.bytes[i++]);
        data.uptime = ((input.bytes[i++] << 56) | (input.bytes[i++] << 48) | (input.bytes[i++] << 40) | (input.bytes[i++] << 32) | (input.bytes[i++] << 24) | (input.bytes[i++] << 16) | (input.bytes[i++] << 8) | input.bytes[i++]);
        data.cputemp = input.bytes[i++];
        data.memory = ((input.bytes[i++] << 24) | (input.bytes[i++] << 16) | (input.bytes[i++] << 8) | input.bytes[i++]);
        data.reset0 = input.bytes[i++];
        data.restarts = ((input.bytes[i++] << 24) | (input.bytes[i++] << 16) | (input.bytes[i++] << 8) | input.bytes[i++]);
    }

    if (input.fPort === 4) {
        var i = 0;
        data.latitude = ((input.bytes[i++] << 24) | (input.bytes[i++] << 16) | (input.bytes[i++] << 8) | input.bytes[i++]);
        data.longitude = ((input.bytes[i++] << 24) | (input.bytes[i++] << 16) | (input.bytes[i++] << 8) | input.bytes[i++]);
        data.sats = input.bytes[i++];
        data.hdop = (input.bytes[i++] << 8) | (input.bytes[i++]);
        data.altitude = ((input.bytes[i++] << 8) | (input.bytes[i++]));
    }

    if (input.fPort === 5) {
        var i = 0;
        data.button = input.bytes[i++];
    }

    if (input.fPort === 7) {
        var i = 0;
        data.temperature = ((input.bytes[i++] << 8) | input.bytes[i++]);
        data.pressure = ((input.bytes[i++] << 8) | input.bytes[i++]);
        data.humidity = ((input.bytes[i++] << 8) | input.bytes[i++]);
        data.air = ((input.bytes[i++] << 8) | input.bytes[i++]);
    }

    if (input.fPort === 8) {
        var i = 0;
        if (input.bytes.length >= 2) {
            data.voltage = (input.bytes[i++] << 8) | input.bytes[i++];
        }
    }

    if (input.fPort === 9) {
        // timesync request
        if (input.bytes.length === 1) {
            data.timesync_seqno = input.bytes[0];
        }
        // epoch time answer
        if (input.bytes.length === 5) {
            var i = 0;
            data.time = ((input.bytes[i++] << 24) | (input.bytes[i++] << 16) | (input.bytes[i++] << 8) | input.bytes[i++]);
            data.timestatus = input.bytes[i++];
        }
    }

    if (data.hdop) {
        data.hdop /= 100;
        data.latitude /= 1000000;
        data.longitude /= 1000000;
    }

    data.bytes = input.bytes; // comment out if you do not want to include the original payload
    data.port = input.fPort; // comment out if you do not want to include the port

    return {
        data: data,
        warnings: [],
        errors: []
    };
}