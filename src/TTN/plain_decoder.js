// Decoder for device payload encoder "PLAIN"
// copy&paste to TTN Console -> Applications -> PayloadFormat -> Decoder

function Decoder(bytes, port) {
  var decoded = {};

  if (port === 1) {
    var i = 0;

    decoded.wifi = (bytes[i++] << 8) | bytes[i++];
    decoded.ble = (bytes[i++] << 8) | bytes[i++];

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

  return decoded;

}