## AWS-IOT ESP8266 device

### Getting Started

#### Register a thing with AWS iot
https://aws.amazon.com/iot-core/

#### Download and convert cert and private key
###### Convert PEM to DER format: (On Windowns you should download openssl first)
`$ openssl x509 -in aaaaaaaaa-certificate.pem.crt.txt -out cert.der -outform DER`

`$ openssl rsa -in aaaaaaaaaa-private.pem.key -out private.der -outform DER`
###### Convert to binary
`$ xxd -c 100000000 -i <cert/key file in DER format>`

#### Create a `secrets.h` file
```c
#define BIN_CRT <binary certificate>
#define BIN_CRT_LEN <binary cert length>
#define BIN_KEY <binary private key>
#define BIN_KEY_LEN <binary key length>
#define BIN_CA <binary CA>
#define BIN_CA_LEN <binary CA length>
#define AWS_ENDPOINT <"foobar.iot.region.amazonaws.com">
#define UPDATE_TOPIC <"$aws/things/yourThingName/shadow/update">
#define DELTA_TOPIC "<custom shadow update request topic>"
#define FW_UPDATE_TOPIC "<custom firmware update topic>"
#define SOLENOID_PIN <pin number>
#define FW_UPDATE_URL <firmware bin url>
```

#### Upload the code with Arduino IDE
After an initial upload of this firmware, subsequent uploads can be OTA via an
http server. Just serve the binary file of the sketch at `FW_UPDATE_URL`
and publish anything to the `FW_UPDATE_TOPIC`.

#### Connect to the AP
- Look for the wifi with an SSID like `ESP-<some id>`
- Visit 192.168.4.1 in your browser
- Input wifi credentials that the device can connect to

