// Converter for device payload encoder "PLAIN"
// copy&paste to TTN Console -> Applications -> PayloadFormat -> Converter

function Converter(decoded, port) {

  var converted = decoded;

  if (port === 1) {
    converted.pax = converted.ble + converted.wifi;
    if (converted.hdop) {
      converted.hdop /= 100;
      converted.latitude /= 1000000;
      converted.longitude /= 1000000;
    }
  }

  return converted;
}