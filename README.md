# ESP8266 Based Smoke Detector

## First Initialization of Projects

### NodeJS and Web

1. First we need to downloads all the dependecies specified in NodeJS's package.json

```
npm install
```
2. Now you can run the ExpressJS server using this command

```
node app.js
```
3. Now you can access it on ```your_ip_here:5000```

### Arduino IDE
1. First install the text editor that are compatible with PlatformIO plugins one of which is Atom IDE

2. install couple of libraries (via PlatformIO Libraries) to compile the code.
	- ArduinoJSON
	- DHTesp
	- WiFiClientSecure
	- PubSubClient
	- TroykaMQ

3. Attach your ESP8266 to your computer and upload the code.
