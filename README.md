# MQTT_Assignment2
Connected Embedded Systems Assignment 2

## Compile Publisher:
```
g++ publisher.cpp I2CDevice.cpp ADXL345.cpp -o publisher -lpaho-mqtt3c
```

## Compile Subscribers:
```
$ g++ subscribeADXL.cpp -o subscribeADXL  -lwiringPi -ljson-c -lpaho-mqtt3c
$ g++ subscribeCPU.cpp -o subscribeCPU  -lwiringPi -ljson-c -lpaho-mqtt3c
```
## Dependecies:

### Paho:
```
$ sudo apt install libssl-dev git
$ git clone https://github.com/eclipse/paho.mqtt.c
$ cd paho.mqtt.c/
$ make
$ sudo make install
```
### exploringRpi:
https://github.com/derekmolloy/exploringrpi/tree/master/chp08/i2c/cpp

### json-c:
```
$ git clone https://github.com/json-c/json-c.git
$ mkdir json-c-build
$ cd json-c-build
$ cmake ../json-c
$ make
$ make test
$ make USE_VALGRIND=0 test   # optionally skip using valgrind
$ sudo make install 
```

### wiringPi:
```
$ git clone https://github.com/WiringPi/WiringPi.git
$ cd WiringPi
$ ./build debian
$ mv debian-template/wiringpi-3.0-1.deb .
$ sudo apt install ./wiringpi-3.0-1.deb
```
