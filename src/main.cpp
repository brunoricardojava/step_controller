/*
Author:Bruno R. S.
Este código tem o objetivo de controlar um motor de passo_motor
atravez de uma bagina web.
As variaveis usadas para controle do motor são:
String tipo_passo ("full_step";"half_step";"quarter_step";"eighth_step";"sixteenth_step")
String sentido_rotacao ("sentido_horario";"sentido_antihorario")
String status_motor ("start";"stop";"reset")
float passo_motor (tipo float)
float angulo_desejado (tipo float)
*/
#include "Arduino.h" //Adicionei essa biblioteca para garantir as dependencias do framwwork arduino
#include "Thread.h"
#include "ThreadController.h"
#include "WiFiManager.h"
#include "ESP8266WebServer.h"
#include "FS.h"
#include "ESP8266FtpServer.h" //Cria um servidor ftp para atualizar os arquivos web.
#include <WiFiUdp.h>

//Velocidade serial
#define serial_baund 115200

//Definição da porta do server UDP
#define local_udp_port 4210

//Habilitar/desabilitar debug serial_baund
#define debug_serial false

//Habilitar/desabilitar servidor ftp
#define ftp_server 0

//Definir tempo de chamada das threads
#define tempo_debug 1000 //miliSecond
#define tempo_motor 300  //miliSecond
#define tempo_udp 300    //miliSecond
#define tempo_step 5     //miliSecond

//Definição dos pinos do motor de passo_motor
#define driver_MS1 D3
#define driver_MS2 D4
#define driver_MS3 D5

#define driver_enable D2
#define driver_RST D6
#define driver_STEP D7
#define driver_DIR D8

//Instanciando Threads
Thread DEBUG_SERIAL;
Thread START_MOTOR;
Thread UDP_IP;
Thread STEP_PULSE;

//Instanciando Thread controller
ThreadController MAIN_THREAD;

//Variaveis globais
String tipo_passo = "full_step";
String sentido_rotacao = "sentido_horario";
String status_motor = "stop";
float passo_motor = 15.0;
float angulo_desejado = 0.0;
int rot_speed = 100;
int micro_step = 1;

int cont_steps = 0;

char incomingPacket[255];  // buffer for incoming packets

//Variavel para permitir o update dos comandos pro motor
bool command_update = true;

//Instanciando um servidor http
ESP8266WebServer server(80);

//Instanciando server UDP
WiFiUDP Udp;

//Instanciando servidor FtpServer
FtpServer ftpSrv;

//Declatando algumas funções
void startMotor();
void udpIp();
void pulseStep();

void configPin(){
	pinMode(driver_MS1, OUTPUT);
	pinMode(driver_MS2, OUTPUT);
	pinMode(driver_MS3, OUTPUT);
	pinMode(driver_enable, OUTPUT);
	pinMode(driver_DIR, OUTPUT);
	pinMode(driver_RST, OUTPUT);
	pinMode(driver_STEP, OUTPUT);
}

void configSerial(){
	Serial.begin(serial_baund);
	delay(100);
	Serial.println("Serial iniciado");
}

void configWifi(){
	WiFiManager wifiManager;
	wifiManager.autoConnect();
	Udp.begin(local_udp_port);
  	Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), local_udp_port);
	//WifiManager.resetSettings(); //Comando para resetar as configurações salvas pela biblioteca
}

void debugSerial(){
	Serial.println("Saida serial: ");

	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	Serial.print("Tipo de passo: ");
	Serial.println(tipo_passo);

	Serial.print("Sentido de rotacao: ");
	Serial.println(sentido_rotacao);

	Serial.print("Velocidade de rotacao: ");
	Serial.println(rot_speed);

	Serial.print("Status do motor: ");
	Serial.println(status_motor);

	Serial.print("Passo do motor: ");
	Serial.println(passo_motor);

	Serial.print("Angulo desejado: ");
	Serial.println(angulo_desejado);
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
	//Verifica se tem o campo correspondente
	if (server.hasArg("button_id")== true){ //Check if body received
		Serial.println("Botao de comando precionado");

		String comand_status = server.arg("button_id");
		String success = "1";

		if(comand_status=="start"){//Se o comando for start faz oque deve ser feito, semelhante ao resto da função para outros comandos
			if(command_update){//Condição de atualização de commando
				status_motor = "start";
				Serial.println("Botao START precionado");
			}
			else success = "0";
		}
		else{
			if (comand_status=="stop") {
				status_motor = "stop";
				command_update = true;
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

		//Prepara um objeto json, contendo o comando que chegou e o status dele sendo "1" e "0";
		String json = "{\"buttonComand\":\"" + String(comand_status) + "\",";
  	json += "\"success\":\"" + String(success) + "\"}";

  	server.send(200, "application/json", json);//Envia o arquivo json
  }
}

void paramStep(){
	if (server.hasArg("button_id")== true && command_update){ //Verifica se tem comando e variavel de atualização de comando
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
	else{ //Caso não venha comando ou não passo ser atualizado

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

void paramAngulo(){
	if (server.hasArg("passo_motor")== true && server.hasArg("angulo_desejado")== true && command_update){
		Serial.println("Valor de angulos selecionados");

		passo_motor = server.arg("passo_motor").toFloat();
		angulo_desejado = server.arg("angulo_desejado").toFloat();
		String success = "1";

		String json = "{\"passo_motor\":\"" + String(passo_motor) + "\",";
		json += "\"angulo_desejado\":\"" + String(angulo_desejado) + "\",";
  	json += "\"success\":\"" + String(success) + "\"}";

  	server.send(200, "application/json", json);
  }
	else{
		float passo_motor2 = 0.0;
		float angulo_desejado2 = 0.0;
		String success = "0";

		String json = "{\"sentidoMotor\":\"" + String(passo_motor2) + "\",";
		json += "\"angulo_desejado\":\"" + String(angulo_desejado2) + "\",";
  	json += "\"success\":\"" + String(success) + "\"}";

  	server.send(200, "application/json", json);
	}
}

void paramVelocidade(){
	if (server.hasArg("velocidade_motor")== true && command_update){
		Serial.println("Valor de velocidade selecionados");

		rot_speed = server.arg("velocidade_motor").toFloat();
		String success = "1";

		String json = "{\"velocidadeMotor\":\"" + String(rot_speed) + "\",";
  	json += "\"success\":\"" + String(success) + "\"}";

  	server.send(200, "application/json", json);
  }
	else{
		float velocidade_motor2 = 0.0;
		String success = "0";

		String json = "{\"velocidadeMotor\":\"" + String(velocidade_motor2) + "\",";
  	json += "\"success\":\"" + String(success) + "\"}";

  	server.send(200, "application/json", json);
	}
}

void configSpiffs(){//Inicia sistema de arquivo SPIFFS
  if (!SPIFFS.begin())
  {
    // Serious problem
    Serial.println("SPIFFS Mount failed");
  } else {
    Serial.println("SPIFFS Mount succesfull");
		if(ftp_server) ftpSrv.begin("esp8266", "esp8266"); //Condição para inicio do servidor ftp
  }
  delay(50);
}

void configServer(){
	//Encaminhamento de requesição POST com os seguintes endereços:
	server.on("/test",funcaoTest);
	server.on("/buttonComand",buttonComand);
	server.on("/paramStep",paramStep);
	server.on("/sentidoMotor", sentidoMotor);
	server.on("/paramAngulo",paramAngulo);
	server.on("/velocidadeMotor", paramVelocidade);

	//Pastas de acesso para a pagina web dentro do micro controlador
	server.serveStatic("/img", SPIFFS, "/img");
	server.serveStatic("/", SPIFFS, "/index.html");
	server.serveStatic("/js", SPIFFS, "/js");
	server.serveStatic("/css", SPIFFS, "/css");


	server.begin();//Inicia servidor HTTP
	Serial.println("Server listening");
}

void configThread(){//Configuração das threads
	DEBUG_SERIAL.setInterval(tempo_debug);
	DEBUG_SERIAL.onRun(debugSerial);
	DEBUG_SERIAL.enabled = debug_serial;

	START_MOTOR.setInterval(tempo_motor);
	START_MOTOR.onRun(startMotor);

	UDP_IP.setInterval(tempo_udp);
	UDP_IP.onRun(udpIp);

	STEP_PULSE.setInterval(tempo_step);
	STEP_PULSE.onRun(pulseStep);
	STEP_PULSE.enabled = false;

	MAIN_THREAD.add(&DEBUG_SERIAL);
	MAIN_THREAD.add(&START_MOTOR);
	MAIN_THREAD.add(&UDP_IP);
	MAIN_THREAD.add(&STEP_PULSE);
}

void setup(){
	configSerial();
	configThread();
	configWifi();
	configSpiffs();
	configServer();
	configPin();
}

void loop(){
	MAIN_THREAD.run();
	server.handleClient();
	if(ftp_server) ftpSrv.handleFTP();
}

void udpIp(){
	int packetSize = Udp.parsePacket();
  	if (packetSize){

		// receive incoming UDP packets
		Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
		int len = Udp.read(incomingPacket, 255);
		if (len > 0){
		  incomingPacket[len] = 0;
		}
		Serial.printf("UDP packet contents: %s\n", incomingPacket);
	
  		// send back a reply, to the IP address and port we got the packet from
  		Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  		char ipBuffer[20];
  		WiFi.localIP().toString().toCharArray(ipBuffer, 20);
  		Udp.write(ipBuffer);
  		Udp.endPacket();
  		Serial.println("Sent ip adress to server");
	}
}

void setMicroStep(String tipo) {   //retorna o valor e seta os pinos para o micro passo desejado
  if (tipo == "full_step") {
    micro_step = 1;
    digitalWrite(driver_MS1, LOW);
    digitalWrite(driver_MS2, LOW);
    digitalWrite(driver_MS3, LOW);
  }
  else if (tipo == "half_step") {
    micro_step = 2;
    digitalWrite(driver_MS1, HIGH);
    digitalWrite(driver_MS2, LOW);
    digitalWrite(driver_MS3, LOW);
  }
  else if (tipo == "quarter_step") {
    micro_step = 4;
    digitalWrite(driver_MS1, LOW);
    digitalWrite(driver_MS2, HIGH);
    digitalWrite(driver_MS3, LOW);
  }
  else if (tipo == "eighth_step") {
    micro_step = 8;
    digitalWrite(driver_MS1, HIGH);
    digitalWrite(driver_MS2, HIGH);
    digitalWrite(driver_MS3, LOW);
  }
  else if (tipo == "sixteenth_step") {
    micro_step = 16;
    digitalWrite(driver_MS1, HIGH);
    digitalWrite(driver_MS2, HIGH);
    digitalWrite(driver_MS3, HIGH);
  }
  else {
    micro_step = 1;
    digitalWrite(driver_MS1, LOW);
    digitalWrite(driver_MS2, LOW);
    digitalWrite(driver_MS3, LOW);
  }
  delay(1);
}

void setDir(String sentido_rotacao) {  //Seta a direção de rotação
  if (sentido_rotacao == "sentido_horario") {
    digitalWrite(driver_DIR, HIGH);
  }
  else if (sentido_rotacao == "sentido_antihorario") {
    digitalWrite(driver_DIR, LOW);
  }
  else {
    digitalWrite(driver_DIR, HIGH);
  }
}

int stepsCount(float passo_motor, float angulo_desejado) {
  float steps;
  steps = angulo_desejado / passo_motor;
  steps = steps * micro_step;
  return steps;
}

void startMotor(){
	if((status_motor == "start" && command_update == true) || status_motor == "stop"){
		if(status_motor == "start"){
			command_update = false;
			digitalWrite(driver_enable, LOW);
			delay(1);
			digitalWrite(driver_RST, HIGH);
			delay(1);
			setMicroStep(tipo_passo);
			setDir(sentido_rotacao);
			delay(1);
			STEP_PULSE.setInterval(500/rot_speed);
			STEP_PULSE.enabled = true;
			Serial.println("Habilitado a thread do pulso...");
		}
		else{
			command_update = true;
			Serial.println("Pulso step desligado...");
		}
	}
}

void pulseStep(){
	if(cont_steps <= stepsCount(passo_motor, angulo_desejado)){
		cont_steps += cont_steps;
		digitalWrite(driver_STEP, !digitalRead(driver_STEP));
	}
	else{
		STEP_PULSE.enabled = false;
		cont_steps = 0;
		status_motor = "stop";
		Serial.println("Motor completou o moviemnto");
	}
}
