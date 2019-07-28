// Decoder for device payload encoder "PACKED"
// copy&paste to TTN Console -> Applications -> PayloadFormat -> Decoder

function Decoder(bytes, port) {

    var decoded = {};

    if (bytes.length === 0) {
        return {};
    }

    if (port === 1) {
        // only wifi counter data, no gps
        if (bytes.length === 2) {
            return decode(bytes, [uint16], ['wifi']);
        }
        // wifi + ble counter data, no gps
        if (bytes.length === 4) {
            return decode(bytes, [uint16, uint16], ['wifi', 'ble']);
        }
        // combined wifi counter and gps data
        if (bytes.length === 15) {
            return decode(bytes, [uint16, latLng, latLng, uint8, hdop, altitude], ['wifi', 'latitude', 'longitude', 'sats', 'hdop', 'altitude']);
        }
        // combined wifi + ble counter and gps data
        if (bytes.length === 17) {
            return decode(bytes, [uint16, uint16, latLng, latLng, uint8, hdop, altitude], ['wifi', 'ble', 'latitude', 'longitude', 'sats', 'hdop', 'altitude']);
        }
    }

    if (port === 2) {
        // device status data
        if (bytes.length === 17) {
            return decode(bytes, [uint16, uptime, uint8, uint32, uint8, uint8], ['voltage', 'uptime', 'cputemp', 'memory', 'reset0', 'reset1']);
        }
    }

    if (port === 3) {
        // device config data      
        return decode(bytes, [uint8, uint8, int16, uint8, uint8, uint8, uint8, bitmap1, bitmap2, version], ['lorasf', 'txpower', 'rssilimit', 'sendcycle', 'wifichancycle', 'blescantime', 'rgblum', 'flags', 'payloadmask', 'version']);
    }

    if (port === 4) {
        // gps data      
        return decode(bytes, [latLng, latLng, uint8, hdop, altitude], ['latitude', 'longitude', 'sats', 'hdop', 'altitude']);
    }

    if (port === 5) {
        // button pressed      
        return decode(bytes, [uint8], ['button']);
    }

    if (port === 6) {
        // beacon proximity alarm      
        return decode(bytes, [int8, uint8], ['rssi', 'beacon']);
    }

    if (port === 7) {
        // BME680 sensor data     
        return decode(bytes, [float, pressure, ufloat, ufloat], ['temperature', 'pressure', 'humidity', 'air']);
    }

    if (port === 8) {
        // battery voltage      
        return decode(bytes, [uint16], ['voltage']);
    }

    if (port === 9) {
        // timesync request
        if (bytes.length === 1) {
            decoded.timesync_seqno = bytes[0];
            return decoded;
        }
        // epoch time answer
        if (bytes.length === 5) {
            return decode(bytes, [uint32, uint8], ['time', 'timestatus']);
        }
    }

}


// ----- contents of /src/decoder.js --------------------------------------------
// https://github.com/thesolarnomad/lora-serialization/blob/master/src/decoder.js

var bytesToInt = function (bytes) {
    var i = 0;
    for (var x = 0; x < bytes.length; x++) {
        i |= (bytes[x] << (x * 8));
    }
    return i;
};

var version = function (bytes) {
    if (bytes.length !== version.BYTES) {
        throw new Error('version must have exactly 10 bytes');
    }
    return String.fromCharCode.apply(null, bytes).split('\u0000')[0];
};
version.BYTES = 10;

var uint8 = function (bytes) {
    if (bytes.length !== uint8.BYTES) {
        throw new Error('uint8 must have exactly 1 byte');
    }
    return bytesToInt(bytes);
};
uint8.BYTES = 1;

var uint16 = function (bytes) {
    if (bytes.length !== uint16.BYTES) {
        throw new Error('uint16 must have exactly 2 bytes');
    }
    return bytesToInt(bytes);
};
uint16.BYTES = 2;

var uint32 = function (bytes) {
    if (bytes.length !== uint32.BYTES) {
        throw new Error('uint32 must have exactly 4 bytes');
    }
    return bytesToInt(bytes);
};
uint32.BYTES = 4;

var uint64 = function (bytes) {
    if (bytes.length !== uint64.BYTES) {
        throw new Error('uint64 must have exactly 8 bytes');
    }
    return bytesToInt(bytes);
};
uint64.BYTES = 8;

var int8 = function (bytes) {
    if (bytes.length !== int8.BYTES) {
        throw new Error('int8 must have exactly 1 byte');
    }
	var value = +(bytesToInt(bytes));
    if (value > 127) {
        value -= 256;
    }
    return value;
};
int8.BYTES = 1;

var int16 = function (bytes) {
    if (bytes.length !== int16.BYTES) {
        throw new Error('int16 must have exactly 2 bytes');
    }
	var value = +(bytesToInt(bytes));
    if (value > 32767) {
        value -= 65536;
    }
    return value;
};
int16.BYTES = 2;

var int32 = function (bytes) {
    if (bytes.length !== int32.BYTES) {
        throw new Error('int32 must have exactly 4 bytes');
    }
	var value = +(bytesToInt(bytes));
    if (value > 2147483647) {
        value -= 4294967296;
    }
    return value;
};
int32.BYTES = 4;

var latLng = function (bytes) {
    return +(int32(bytes) / 1e6).toFixed(6);
};
latLng.BYTES = int32.BYTES;

var uptime = function (bytes) {
    return uint64(bytes);
};
uptime.BYTES = uint64.BYTES;

var hdop = function (bytes) {
    return +(uint16(bytes) / 100).toFixed(2);
};
hdop.BYTES = uint16.BYTES;

var altitude = function (bytes) {
    // Option to increase altitude resolution (also on encoder side)
    // return +(int16(bytes) / 4 - 1000).toFixed(1);
    return +(int16(bytes));
};
altitude.BYTES = int16.BYTES;


var float = function (bytes) {
    if (bytes.length !== float.BYTES) {
        throw new Error('Float must have exactly 2 bytes');
    }
    var isNegative = bytes[0] & 0x80;
    var b = ('00000000' + Number(bytes[0]).toString(2)).slice(-8)
        + ('00000000' + Number(bytes[1]).toString(2)).slice(-8);
    if (isNegative) {
        var arr = b.split('').map(function (x) { return !Number(x); });
        for (var i = arr.length - 1; i > 0; i--) {
            arr[i] = !arr[i];
            if (arr[i]) {
                break;
            }
        }
        b = arr.map(Number).join('');
    }
    var t = parseInt(b, 2);
    if (isNegative) {
        t = -t;
    }
    return +(t / 100).toFixed(2);
};
float.BYTES = 2;

var ufloat = function (bytes) {
    return +(uint16(bytes) / 100).toFixed(2);
};
ufloat.BYTES = uint16.BYTES;

var pressure = function (bytes) {
    return +(uint16(bytes) / 10).toFixed(1);
};
pressure.BYTES = uint16.BYTES;

var bitmap1 = function (byte) {
    if (byte.length !== bitmap1.BYTES) {
        throw new Error('Bitmap must have exactly 1 byte');
    }
    var i = bytesToInt(byte);
    var bm = ('00000000' + Number(i).toString(2)).substr(-8).split('').map(Number).map(Boolean);
    return ['adr', 'screensaver', 'screen', 'countermode', 'blescan', 'antenna', 'filter', 'alarm']
        .reduce(function (obj, pos, index) {
            obj[pos] = +bm[index];
            return obj;
        }, {});
};
bitmap1.BYTES = 1;

var bitmap2 = function (byte) {
    if (byte.length !== bitmap2.BYTES) {
        throw new Error('Bitmap must have exactly 1 byte');
    }
    var i = bytesToInt(byte);
    var bm = ('00000000' + Number(i).toString(2)).substr(-8).split('').map(Number).map(Boolean);
    return ['gps', 'alarm', 'bme', 'counter', 'sensor1', 'sensor2', 'sensor3', 'battery']
        .reduce(function (obj, pos, index) {
            obj[pos] = +bm[index];
            return obj;
        }, {});
};
bitmap2.BYTES = 1;

var decode = function (bytes, mask, names) {

    var maskLength = mask.reduce(function (prev, cur) {
        return prev + cur.BYTES;
    }, 0);
    if (bytes.length < maskLength) {
        throw new Error('Mask length is ' + maskLength + ' whereas input is ' + bytes.length);
    }

    names = names || [];
    var offset = 0;
    return mask
        .map(function (decodeFn) {
            var current = bytes.slice(offset, offset += decodeFn.BYTES);
            return decodeFn(current);
        })
        .reduce(function (prev, cur, idx) {
            prev[names[idx] || idx] = cur;
            return prev;
        }, {});
};

if (typeof module === 'object' && typeof module.exports !== 'undefined') {
    module.exports = {
        uint8: uint8,
        uint16: uint16,
        uint32: uint32,
        int8: int8,
        int16: int16,
        int32: int32,
        uptime: uptime,
        float: float,
        ufloat: ufloat,
        pressure: pressure,
        latLng: latLng,
        hdop: hdop,
        altitude: altitude,
        bitmap1: bitmap1,
        bitmap2: bitmap2,
        version: version,
        decode: decode
    };
}