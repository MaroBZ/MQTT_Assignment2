#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include "json-c/json.h"
#include <wiringPi.h>
#include <unistd.h>

#define ADDRESS "tcp://192.168.247.209:1883"
#define CLIENTID "ADXL_SUBSCRIBER"
#define AUTHMETHOD "maro"
#define AUTHTOKEN "admin"
#define TOPIC "ee513/Data"
#define PAYLOAD "Hello World!"
#define QOS 1
#define TIMEOUT 10000L

#define SENSOR_LED_PIN 4

using namespace std;

volatile MQTTClient_deliveryToken deliveredtoken;



void delivered(void *context, MQTTClient_deliveryToken dt) { // Callback function for message delivery
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}


void gpioSetup() { // Set up the GPIO pins
    wiringPiSetupGpio();
    pinMode(SENSOR_LED_PIN, OUTPUT); // Set the LED pin as an output
}

void actuateSensor_LED(float pitch, float roll) { // Threshold chosen randomly just for demonstration
    if (pitch > 45 || pitch < -45 || roll > 45 || roll < -45) {
        printf("--->>Pitch or Roll threshold exceeded, turning on LED<<---\n");
        digitalWrite(SENSOR_LED_PIN, HIGH); // Turn on LED
    } else {
        if (digitalRead(SENSOR_LED_PIN) == HIGH) {
            printf("--->>Pitch and Roll are within threshold, turning off LED<<---\n");
            digitalWrite(SENSOR_LED_PIN, LOW); // Turn off LED
        }
        digitalWrite(SENSOR_LED_PIN, LOW); // Turn off LED
    }
}


void parse_adxl(json_object *jobj) {
    json_object *jd, *jtimestamp, *jadxl345, *jpitch, *jroll, *jacceleration;
    if (json_object_object_get_ex(jobj, "d", &jd) && json_object_object_get_ex(jd, "timestamp", &jtimestamp) && json_object_object_get_ex(jd, "ADXL345", &jadxl345)) 
    { // json_object_object_get_ex returns true if the key is found
        printf("Data received at time: %s\n", json_object_get_string(jtimestamp));
        printf("Pitch: %f\n", json_object_get_double(json_object_object_get(jadxl345, "Pitch")));
        printf("Roll: %f\n", json_object_get_double(json_object_object_get(jadxl345, "Roll")));
        jacceleration = json_object_object_get(jadxl345, "Acceleration");
        printf("Acceleration: [%d, %d, %d]\n",
            json_object_get_int(json_object_array_get_idx(jacceleration, 0)), //json_object_array_get_idx returns the element at the specified index of an array, in this case the Acceleration array
            json_object_get_int(json_object_array_get_idx(jacceleration, 1)),
            json_object_get_int(json_object_array_get_idx(jacceleration, 2)));
        printf("-------------------------------\n");
        // Actuate the LED based on the pitch and roll values
        actuateSensor_LED(json_object_get_double(json_object_object_get(jadxl345, "Pitch")), json_object_get_double(json_object_object_get(jadxl345, "Roll")));
    }
}


int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) { // Callback function for message arrival
    char* payloadptr;
    printf("Message arrived\n");
    printf(" topic: %s\n", topicName);
    payloadptr = (char*) message->payload;

    json_object * jobj = json_tokener_parse(payloadptr); // Parse the JSON object

    parse_adxl(jobj);


    MQTTClient_freeMessage(&message); 
    MQTTClient_free(topicName); 
    return 1;

}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf(" cause: %s\n", cause);
}

int main(int argc, char* argv[]) {

    int ch;
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_DEFAULT, NULL);

    opts.keepAliveInterval = 20;
    opts.cleansession = 0;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }

    gpioSetup(); // Initialize the GPIO pins

    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
    "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    

    do {
        ch = getchar();
    } 
    while(ch!='Q' && ch!='q');

    digitalWrite(SENSOR_LED_PIN, LOW);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}