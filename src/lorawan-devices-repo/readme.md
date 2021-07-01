# lorawan-devices repo for The Things Network & The Things Stack V3

To add bigger payload decoders than 4k (via web ui) we provide the metadata to the lorawan-devices repo on github. For this we create a vendor "opensource" and specify a device "esp32-paxcounter" via this files:

> /
> - index.yaml (include the marked lines at the bottom of the original file)
> 
> /vendor (existing folder in the repo)
> 
> - /opensource (new folder with all the good stuff inside)
> 
> /vendor/opensource
> 
> - index.yaml (name of the device inside)
> - esp32-paxcounter.yaml (metadata for the software)
> - esp32-paxcounter-profile-eu868.yaml (profile for europe)
> - esp32-paxcounter-profile-us915.yaml (profile for north america)
> - esp32-paxcounter-codec.yaml (examples for the payload decoder)
> - esp32-paxcounter-packed_decodeUplink.js (payload decoder as provided before for the web ui, now for the lorawan devices repo)

With these files, we can make a pull request at the lorawan-devices repo: https://github.com/TheThingsNetwork/lorawan-devices and hope to be included in the future. Cool thing about it would be that then the users need only to choose "Open Source Community Projects" from the vendor list and "EXP32-Paxcounter" as device and most of the compex things is taking care of (like choosing the right LoRaWAN version and installing the payload decoder).
