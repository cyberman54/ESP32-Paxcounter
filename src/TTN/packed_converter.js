// Converter for device payload encoder "PACKED"
// copy&paste to TTN Console -> Applications -> PayloadFormat -> Converter

function Converter(decoded, port) {

    var converted = decoded;
    var pax = 0;

    if (port === 1) {
      if('wifi' in converted){
          pax += converted.wifi
      }
       
      if('ble' in converted){
          pax += converted.ble
      } 
        converted.pax = pax;
    }

    if (port === 2) {
        if('voltage' in converted)
        converted.voltage /= 1000;
    }

    return converted;
}