// Converter for device payload encoder "PACKED"
// copy&paste to TTN Console -> Applications -> PayloadFormat -> Converter

function Converter(decoded, port) {

    var converted = decoded;

    if (port === 1) {
        converted.pax = converted.ble + converted.wifi;
    }

    if (port === 2) {
        converted.voltage /= 1000;
        converted.uptime /= 60;
    }

    return converted;
}