#include "Arduino.h"
#include "Thread.h"
#include "ThreadController.h"
#include "WiFiManager.h"
#include "ESP8266WebServer.h"
#include "FS.h"
#include "ESP8266FtpServer.h"

//Velocidade serial
#define serial_baund 115200

//Habilitar/desabilitar debug serial_baund
#define debug_serial 0

//Definir tempo de chamada das threads
#define tempo_debug 1000



//Instanciando Threads
Thread DEBUG_SERIAL;

//Instanciando Thread controller
ThreadController MAIN_THREAD;

//Instanciando um servidor http
ESP8266WebServer server(80);

//Instanciando servidor FtpServer
FtpServer ftpSrv;

void configSerial(){
	Serial.begin(serial_baund);
	delay(100);
	Serial.println("Serial iniciado");
}

void configWifi(){
	WiFiManager wifiManager;
	wifiManager.autoConnect();
}

void debugSerial(){
	Serial.println("Saida serial: ");

	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

void funcaoTest(){
	/*if (server.hasArg("plain")== false){ //Check if body received
		server.send(200, "text/plain", "Body not received");
		return;
  }

  String message = "Body received:\n";
         message += server.arg("plain");
         message += "\n";
  server.send(200, "text/plain", message);
  Serial.println(message);
	*/
	//-------------------------------------------------//
	String message = "Number of args received:";
	message += server.args();            //Get number of parameters
	message += "\n";                            //Add a new line

	for (int i = 0; i < server.args(); i++) {

		message += "Arg nº" + (String)i + " -> ";   //Include the current iteration value
		message += server.argName(i) + ": ";     //Get the name of the parameter
		message += server.arg(i) + "\n";              //Get the value of the parameter

	}

	server.send(200, "text/plain", message);       //Response to the HTTP requestP)
}

void configSpiffs(){
  if (!SPIFFS.begin())
  {
    // Serious problem
    Serial.println("SPIFFS Mount failed");
  } else {
    Serial.println("SPIFFS Mount succesfull");
		ftpSrv.begin("esp8266", "esp8266");
  }
  delay(50);
}

void configServer(){
	server.on("/test",funcaoTest);

	server.serveStatic("/img", SPIFFS, "/img");
  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/css", SPIFFS, "/css");

	server.begin();
	Serial.println("Server listening");
}

void configThread(){
	DEBUG_SERIAL.setInterval(tempo_debug);
	DEBUG_SERIAL.onRun(debugSerial);

	if(debug_serial) MAIN_THREAD.add(&DEBUG_SERIAL);
}

void setup(){
	configSerial();
	configThread();
	configWifi();
	configSpiffs();
	configServer();
}

void loop(){
	MAIN_THREAD.run();
	server.handleClient();
	ftpSrv.handleFTP();
}
