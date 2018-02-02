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

//Variaveis globais
String tipo_passo = "full_step";
String sentido_rotacao = "sentido_horario";
String status_motor = "stop";

//Variavel para permitir o update dos comandos pro motor
bool command_update = true;

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


void buttonComand(){
	if (server.hasArg("button_id")== true){ //Check if body received
		Serial.println("Botao de comando precionado");

		String comand_status = server.arg("button_id");
		String success = "1";

		if(comand_status=="start"){
			if(command_update){
				status_motor = "start";
				Serial.println("Botao START precionado");
			}
			else success = "0";
		}
		else{
			if (comand_status=="stop") {
				status_motor = "stop";
				Serial.println("Botao STOP precionado");
			}
			else{
				if (comand_status=="reset") {
					Serial.println("Botao RESET precionado");
				}
				else{
					Serial.print("ID de botao nao valido");
					server.send(200, "text/plain", "Botao nao valido");
					success = "0";
				}
			}
		}

		String json = "{\"buttonComand\":\"" + String(comand_status) + "\",";
  	json += "\"success\":\"" + String(success) + "\"}";

  	server.send(200, "application/json", json);
  }
}

void paramStep(){
	if (server.hasArg("button_id")== true && command_update){ //Check if body received
		Serial.println("Botao de parametro precionado");

		String param_status = server.arg("button_id");
		String success = "1";

		if (param_status=="full_step") {
			tipo_passo = "full_step";
			Serial.println("Angulo de passo setado para Full-Step");
		}
		else{
			if (param_status=="half_step") {
				tipo_passo = "half_step";
				Serial.println("Angulo de passo setado para Half-Step");
			}
			else{
				if (param_status=="quarter_step") {
					tipo_passo = "quarter_step";
					Serial.println("Angulo de passo setado para Quarter-Step");
				}
				else{
					if (param_status=="eighth_step") {
						tipo_passo = "eighth_step";
						Serial.println("Angulo de passo setado para Eighth-Step");
					}
					else{
						if (param_status=="sixteenth_step") {
							tipo_passo = "sixteenth_step";
							Serial.println("Angulo de passo setado para Sixteenth-Step");
						}
						else{
							if (param_status=="sentido_horario") {
								sentido_rotacao = "sentido_horario";
								Serial.println("Sentido de rotacao e horario");
							}
							else{
								if (param_status=="sentido_antihorario") {
									sentido_rotacao = "sentido_antihorario";
									Serial.println("Sentido de rotacao e anti-horario");
								}
								else{
									Serial.print("ID de botao nao valido");
									server.send(200, "text/plain", "Botao nao valido");
									success = "0";
								}
							}
						}
					}
				}
			}
		}

		String json = "{\"buttonParam\":\"" + String(param_status) + "\",";
  	json += "\"success\":\"" + String(success) + "\"}";

  	server.send(200, "application/json", json);

	}
	else{

		String param_status = server.arg("button_id");
		String success = "0";

		String json = "{\"buttonParam\":\"" + String(param_status) + "\",";
		json += "\"success\":\"" + String(success) + "\"}";

		server.send(200, "application/json", json);
	}
}

void sentidoMotor(){
	if (server.hasArg("button_id")== true && command_update){ //Check if body received
		Serial.println("Botao de comando precionado");

		String sentido_motor = server.arg("button_id");
		String success = "1";

		if (sentido_motor=="sentido_horario") {
			sentido_rotacao = "sentido_horario";
			Serial.println("Sentido de rotacao e horario");
		}
		else{
			if (sentido_motor=="sentido_antihorario") {
				sentido_rotacao = "sentido_antihorario";
				Serial.println("Sentido de rotacao e anti-horario");
			}
			else{
				Serial.print("ID de botao nao valido");
				server.send(200, "text/plain", "Botao nao valido");
				success = "0";
			}
		}

		String json = "{\"sentidoMotor\":\"" + String(sentido_motor) + "\",";
  	json += "\"success\":\"" + String(success) + "\"}";

  	server.send(200, "application/json", json);
  }
	else{
		String sentido_motor = server.arg("button_id");
		String success = "0";

		String json = "{\"sentidoMotor\":\"" + String(sentido_motor) + "\",";
  	json += "\"success\":\"" + String(success) + "\"}";

  	server.send(200, "application/json", json);
	}
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
	server.on("/buttonComand",buttonComand);
	server.on("/paramStep",paramStep);
	server.on("/sentidoMotor", sentidoMotor);

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
