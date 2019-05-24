// Decoder for device payload encoder "PLAIN"
// copy&paste to TTN Console -> Applications -> PayloadFormat -> Decoder

function Decoder(bytes, port) {
  var decoded = {};

  if (port === 1) {
    var i = 0;

    if (bytes.length >= 2) {
      decoded.wifi = (bytes[i++] << 8) | bytes[i++];
    }

    if (bytes.length === 4 || bytes.length > 15) {
      decoded.ble = (bytes[i++] << 8) | bytes[i++];
    }

    if (bytes.length > 4) {
      decoded.latitude = ((bytes[i++] << 24) | (bytes[i++] << 16) | (bytes[i++] << 8) | bytes[i++]);
      decoded.longitude = ((bytes[i++] << 24) | (bytes[i++] << 16) | (bytes[i++] << 8) | bytes[i++]);
      decoded.sats = bytes[i++];
      decoded.hdop = (bytes[i++] << 8) | (bytes[i++]);
      decoded.altitude = (bytes[i++] << 8) | (bytes[i++]);
    }
  }

  if (port === 2) {
    var i = 0;
    decoded.battery = ((bytes[i++] << 8) | bytes[i++]);
    decoded.uptime = ((bytes[i++] << 56) | (bytes[i++] << 48) | (bytes[i++] << 40) | (bytes[i++] << 32) |
      (bytes[i++] << 24) | (bytes[i++] << 16) | (bytes[i++] << 8) | bytes[i++]);
    decoded.temp = bytes[i++];
    decoded.memory = ((bytes[i++] << 24) | (bytes[i++] << 16) | (bytes[i++] << 8) | bytes[i++]);
    decoded.reset0 = bytes[i++];
    decoded.reset1 = bytes[i++];
  }

  if (port === 4) {
    var i = 0;
    decoded.latitude = ((bytes[i++] << 24) | (bytes[i++] << 16) | (bytes[i++] << 8) | bytes[i++]);
    decoded.longitude = ((bytes[i++] << 24) | (bytes[i++] << 16) | (bytes[i++] << 8) | bytes[i++]);
    decoded.sats = bytes[i++];
    decoded.hdop = (bytes[i++] << 8) | (bytes[i++]);
    decoded.altitude = (bytes[i++] << 8) | (bytes[i++]);
  }

  if (port === 5) {
    var i = 0;
    decoded.button = bytes[i++];
  }

  if (port === 6) {
    var i = 0;
    decoded.rssi = bytes[i++];
    decoded.beacon = bytes[i++];
  }

  if (port === 7) {
    var i = 0;
    decoded.temperature = ((bytes[i++] << 8) | bytes[i++]);
    decoded.pressure = ((bytes[i++] << 8) | bytes[i++]);
    decoded.humidity = ((bytes[i++] << 8) | bytes[i++]);
    decoded.air = ((bytes[i++] << 8) | bytes[i++]);
  }

  if (port === 8) {
    var i = 0;
    if (bytes.length >= 2) {
      decoded.battery = (bytes[i++] << 8) | bytes[i++];
    }
  }

  if (port === 9) {
    // timesync request
    if (bytes.length === 1) {
      decoded.timesync_seqno = bytes[0];
    }
    // epoch time answer
    if (bytes.length === 5) {
      var i = 0;
      decoded.time = ((bytes[i++] << 24) | (bytes[i++] << 16) | (bytes[i++] << 8) | bytes[i++]);
      decoded.timestatus = bytes[i++];
    }
  }
  return decoded;
}
