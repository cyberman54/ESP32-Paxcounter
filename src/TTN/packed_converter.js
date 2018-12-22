// Converter for device payload encoder "PACKED"
// copy&paste to TTN Console -> Applications -> PayloadFormat -> Converter

function Converter(decoded, port) {

    var converted = decoded;
    var pax = 0;

    if (port === 1) {

        for (var x in converted) {
            pax += converted[x];
            }
            
    converted.pax = pax;

    }

    if (port === 2) {
        converted.voltage /= 1000;
    }


    return converted;
}