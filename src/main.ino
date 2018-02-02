#include "Thread.h"
#include "ThreadController.h"

//Velocidade serial
#define serial_baund 115200

//Definir tempo de chamada das threads
#define tempo_debug 500

//Instanciando Threads
Thread DEBUG_SERIAL;

//Instanciando Thread controller
ThreadController MAIN_THREAD;

void configThread(){
	DEBUG_SERIAL.setInterval(tempo_debug);
	DEBUG_SERIAL.onRun(debugSerial);

	MAIN_THREAD.add(&DEBUG_SERIAL);
}

void configSerial(){
	Serial.begin(serial_baund);
	delay(100);
	Serial.println("Serial iniciado");
}

void debugSerial(){
	Serial.println("Saida serial: ");
}

void setup(){
	configSerial();
	configThread();
}

void loop(){
	MAIN_THREAD.run();
}