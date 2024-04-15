#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <chrono>
#include <iomanip>

#include <pthread.h>

#include "MQTTClient.h"
#include "ADXL345.h"

using namespace std;
using namespace exploringRPi;

#define CPU_TEMP "/sys/class/thermal/thermal_zone0/temp"

#define ADDRESS "tcp://192.168.247.209:1883"
#define CLIENTID "rpi1"
#define AUTHMETHOD "maro"
#define AUTHTOKEN "admin"
#define TOPIC "ee513/Data"
#define QOS 1
#define TIMEOUT 10000L

static int currentQOS = 0;

float getCPUTemperature() { // get the CPU temperature
    int cpuTemp; // store as an int
    fstream fs;
    fs.open(CPU_TEMP, fstream::in); // read from the file
    fs >> cpuTemp;
    fs.close();
    return (((float)cpuTemp)/1000);
}


int main(int argc, char* argv[]) {

    char str_payload[150]; 

    ADXL345 sensor(1,0x53); 
    sensor.setResolution(ADXL345::NORMAL);
    sensor.setRange(ADXL345::PLUSMINUS_4_G);

    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    MQTTClient_willOptions lastwill = MQTTClient_willOptions_initializer;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;
    int rc;

    //last will message
    lastwill.topicName = TOPIC;
    lastwill.message = "LAST WILL: Publisher disconnected abruptly."; //payload in string form, lastwill.payload for binary 
    lastwill.qos = QOS;
    lastwill.retained = 0; // 0 means the message is not stored on the server
    opts.will = &lastwill;

    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        cout << "Failed to connect, return code " << rc << endl;
        return -1;
    }
    



    while (true) {  

        sensor.readSensorState();
        
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // Current time
        std::tm now_time = *std::localtime(&time_t);
        std::stringstream ss;
        ss << std::put_time(&now_time, "%FT%T"); 
        sprintf(str_payload, "{\"d\":{\"timestamp\":\"%s\",\"CPU_TEMP\":%f,\"ADXL345\":{\"Pitch\":%f,\"Roll\":%f,\"Acceleration\":[%d,%d,%d]}}}", ss.str().c_str(), getCPUTemperature(), sensor.getPitch(), sensor.getRoll(), sensor.getAccelerationX(), sensor.getAccelerationY(), sensor.getAccelerationZ());
        pubmsg.payload = str_payload;
        pubmsg.payloadlen = strlen(str_payload);

        // Demonstrate different QoS levels by cycling through them

        currentQOS = (currentQOS + 1) % 3; // Cycle through QoS levels 0, 1, 2
        pubmsg.qos = currentQOS;

        pubmsg.qos = currentQOS;
        pubmsg.retained = 0;
        MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
        cout << "Waiting for up to " << (int)(TIMEOUT/1000) <<
        " seconds for publication of " << str_payload <<
        " \non topic: " << TOPIC << " for ClientID: " << CLIENTID << " |QOS: " << currentQOS << endl;
        rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
        cout << "Message with token " << (int)token << " delivered." << endl;
        cout << "-----------------------------------------------" << endl;
        cout << "Waiting 2 seconds before sending the next message" << endl;
        cout << "-----------------------------------------------" << endl;
        sleep(2);
    }


    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;


}