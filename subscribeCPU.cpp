#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include "json-c/json.h"
#include <wiringPi.h>
#include <unistd.h>

#define ADDRESS "tcp://192.168.247.209:1883"
#define CLIENTID "CPU_SUBSCRIBER"
#define AUTHMETHOD "maro"
#define AUTHTOKEN "admin"
#define TOPIC "ee513/Data"
#define PAYLOAD "Hello World!"
#define QOS 1
#define TIMEOUT 10000L

#define CPU_LED_PIN 17

using namespace std;

volatile MQTTClient_deliveryToken deliveredtoken;



void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}


void gpioSetup() {
    wiringPiSetupGpio();
    pinMode(CPU_LED_PIN, OUTPUT);
}

void actuateCPU_LED(float CPUTemp) {
    if (CPUTemp > 40.4) {
        digitalWrite(CPU_LED_PIN, HIGH); // Turn on LED
        printf("--->>CPU Temp is high, turning on LED<<---\n");
    } else {
        if (digitalRead(CPU_LED_PIN) == HIGH) {
            printf("--->>CPU Temp is back to normal, turning off LED<<---\n");
            digitalWrite(CPU_LED_PIN, LOW); // Turn off LED
        }
        digitalWrite(CPU_LED_PIN, LOW); // Turn off LED
    }
}


void parse_cpu(json_object *jobj) {
    json_object *jd, *jtimestamp, *jcpu_temp;
    if (json_object_object_get_ex(jobj, "d", &jd) && json_object_object_get_ex(jd, "timestamp", &jtimestamp) && json_object_object_get_ex(jd, "CPU_TEMP", &jcpu_temp)) {
        printf("CPU Temp: %f at time %s\n", json_object_get_double(jcpu_temp), json_object_get_string(jtimestamp));
        printf("-------------------------------\n");
        actuateCPU_LED(json_object_get_double(jcpu_temp));
    }
}


int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char* payloadptr;
    printf("Message arrived\n");
    printf(" topic: %s\n", topicName);
    payloadptr = (char*) message->payload;

    json_object * jobj = json_tokener_parse(payloadptr);

    parse_cpu(jobj);

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
    
    gpioSetup();
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
    "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    

    do {
        ch = getchar();
    } 
    while(ch!='Q' && ch!='q');
    
    digitalWrite(CPU_LED_PIN, LOW);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}