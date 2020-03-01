// Converter for device payload encoder "PLAIN"
// copy&paste to TTN Console -> Applications -> PayloadFormat -> Converter

function Converter(decoded, port) {

  var converted = decoded;
  var pax = 0;

  if (port === 1) {

    if ('wifi' in converted) {
      pax += converted.wifi
    }
    if ('ble' in converted) {
      pax += converted.ble
    }
    converted.pax = pax;

    if (converted.hdop) {
      converted.hdop /= 100;
      converted.latitude /= 1000000;
      converted.longitude /= 1000000;
    }

  }

  return converted;
}