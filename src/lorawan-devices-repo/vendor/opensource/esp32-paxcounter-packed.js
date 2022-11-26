// Decoder for device payload encoder "PACKED"
// copy&paste to TTN Console V3 -> Applications -> Payload formatters -> Uplink -> Javascript
// modified for The Things Stack V3 by Caspar Armster, dasdigidings e.V.

function decodeUplink(input) {
    var data = {};

    if (input.fPort === 1) {
        // only wifi counter data, no gps
        if (input.bytes.length === 2) {
            data = decode(input.bytes, [uint16], ['wifi']);
        }
        // wifi + ble counter data, no gps
        if (input.bytes.length === 4) {
            data = decode(input.bytes, [uint16, uint16], ['wifi', 'ble']);
        }
        // combined wifi + ble + SDS011
        if (input.bytes.length === 8) {
            data = decode(input.bytes, [uint16, uint16, uint16, uint16], ['wifi', 'ble', 'PM10', 'PM25']);
        }
        // combined wifi counter and gps data, used by https://opensensemap.org
        if (input.bytes.length === 10) {
            data = decode(input.bytes, [latLng, latLng, uint16], ['latitude', 'longitude', 'wifi']);
        }
        // combined wifi + ble counter and gps data, used by https://opensensemap.org
        if (input.bytes.length === 12) {
            data = decode(input.bytes, [latLng, latLng, uint16, uint16], ['latitude', 'longitude', 'wifi', 'ble']);
        }
        // combined wifi counter and gps data
        if (input.bytes.length === 15) {
            data = decode(input.bytes, [uint16, latLng, latLng, uint8, hdop, altitude], ['wifi', 'latitude', 'longitude', 'sats', 'hdop', 'altitude']);
        }
        // combined wifi + ble counter and gps data
        if (input.bytes.length === 17) {
            data = decode(input.bytes, [uint16, uint16, latLng, latLng, uint8, hdop, altitude], ['wifi', 'ble', 'latitude', 'longitude', 'sats', 'hdop', 'altitude']);
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
        // device status data
        if (input.bytes.length === 20) {
            data = decode(input.bytes, [uint16, uptime, uint8, uint32, uint8, uint32], ['voltage', 'uptime', 'cputemp', 'memory', 'reset0', 'restarts']);
        }
    }

    if (input.fPort === 3) {
        // device config data      
        data = decode(input.bytes, [uint8, uint8, int16, uint8, uint8, uint8, uint16, bitmap1, bitmap2, version], ['loradr', 'txpower', 'rssilimit', 'sendcycle', 'wifichancycle', 'blescantime', 'sleepcycle', 'flags', 'payloadmask', 'version']);
    }

    if (input.fPort === 4) {
        // gps data      
        if (input.bytes.length === 8) {
            data = decode(input.bytes, [latLng, latLng], ['latitude', 'longitude']);
        } else {
            data = decode(input.bytes, [latLng, latLng, uint8, hdop, altitude], ['latitude', 'longitude', 'sats', 'hdop', 'altitude']);
        }
    }

    if (input.fPort === 5) {
        // button pressed      
        data = decode(input.bytes, [uint8], ['button']);
    }

    if (input.fPort === 7) {
        // BME680 sensor data     
        data = decode(input.bytes, [float, pressure, ufloat, ufloat], ['temperature', 'pressure', 'humidity', 'air']);
    }

    if (input.fPort === 8) {
        // battery voltage      
        data = decode(input.bytes, [uint16], ['voltage']);
    }

    if (input.fPort === 9) {
        // timesync request
        if (input.bytes.length === 1) {
            data.timesync_seqno = input.bytes[0];
        }
        // epoch time answer
        if (input.bytes.length === 5) {
            data = decode(input.bytes, [uint32, uint8], ['time', 'timestatus']);
        }
    }
  
    data.bytes = input.bytes; // comment out if you do not want to include the original payload
    data.port = input.fPort; // comment out if you do not want to inlude the port

    return {
        data: data,
        warnings: [],
        errors: []
    };
}

function encodeDownlink(input) {
  return {
    data: {
      bytes: input.bytes
    },
    warnings: ["Encoding of downlink is not supported by the JS decoder."],
    errors: []
  }
}

function decodeDownlink(input) {
  return {
    data: {
      bytes: input.bytes
    },
    warnings: ["Decoding of downlink is not supported by the JS decoder."],
    errors: []
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
    return ['adr', 'screensaver', 'screen', 'countermode', 'blescan', 'antenna', 'reserved', 'reserved']
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
    return ['battery', 'sensor3', 'sensor2', 'sensor1', 'gps', 'bme', 'reserved', 'counter']
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