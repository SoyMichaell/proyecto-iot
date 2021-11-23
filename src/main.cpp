#include <Arduino.h>
#include <WiFi.h>
#include "DHT.h"
#include <Firebase_ESP_Client.h>
#include "ThingSpeak.h"
#include "ESP32_MailClient.h"

//Firebase
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

const char* API_KEY = "AIzaSyBh6ut-gOvh_V0FNk1yCLAsIYMeZWEpdyc";
const char* DATABASE_URL = "https://casa-79d01-default-rtdb.firebaseio.com/";

//ThingSpeak
unsigned long canalID = 1580456;
const char* escrituraAPI = "14O6KM1OFX4KWQB4";

//Definiciones objetos firebase
SMTPData datosSMTP;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long envioDatoTiempo = 0;
bool ingresoOK = false;

//credenciales red
const char* ssid = "nombre_red";
const char* pass = "pass";

//Detalles correo
const char* host = "smtp.gmail.com";
const char* loginEmail = "maicolh474@gmail.com";
const char* loginPassword = "maROHer142020";
const int port = 465;
const char* accountEmail = "soymichaelfb2@gmail.com";

WiFiServer server(80);

//Variable http
String header;

//Auxiliares estados leds
String led1Output = "off";
String led2Output = "off";

//Asignacion pines
const int led1 = 4;
const int led2 = 5;
const int DhtPin = 2;
const int sensorSuelo = 23;

//Tiempo actual
unsigned long tiempoActual = millis();
unsigned long tiempoAnterior = 0;
const long tiempoFuera = 2000;

//Inicializo sensor
DHT dht(DhtPin, DHT11);

WiFiClient client;

void inicializar(){
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  //Estado inicial
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);

  //config firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if(Firebase.signUp(&config, &auth, "", "")){
    Serial.println("Conexion base de datos exitosa");
    ingresoOK = true;
  }else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  ThingSpeak.begin(client);

}

void conexionWifi(){
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  //Mostrando IP conexion servidor
  Serial.println("");
  Serial.println("Conectado a WIFI");
  Serial.println("IP Address:");
  Serial.println(WiFi.localIP());
  server.begin();
}

void lecturaSuelo(){
  float sensorValor = analogRead(sensorSuelo);
  Serial.print("Humedad de suelo -> ");
  Serial.println(sensorValor);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  //funciones
  inicializar();
  conexionWifi();
}

void loop() {
  
  //Cliente
  client = server.available();
  if(client){
    tiempoActual = millis();
    tiempoAnterior = tiempoActual;
    Serial.println("Nuevo cliente");
    String lineaIngreso = "";
    while(client.connected() && tiempoActual - tiempoAnterior <= tiempoFuera){
      tiempoActual = millis();
      if(client.available()){
        char c = client.read();
        Serial.write(c);
        header += c;
        if(c == '\n'){
          if(lineaIngreso.length() == 0){
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type: text-html");
            client.println("Connection: close");
            client.println();

            if(header.indexOf("GET /led1/on") >= 0){
              Serial.println("LED 1 Encendido");
              led1Output = "on";
              digitalWrite(led1, HIGH);
            }else if(header.indexOf("GET /led1/off") >= 0){
              Serial.println("LED 1 Apagado");
              led1Output = "off";
            }else if(header.indexOf("GET /led2/on") >= 0){
              Serial.println("LED 2 Encendido");
              led2Output = "on";
              digitalWrite(led2, HIGH);
            }else if(header.indexOf("GET /led2/off") >= 0){
              Serial.println("LED 2 Apagado");
              led2Output = "off";
              digitalWrite(led2, LOW);
            }

            //Valores tomados de sensores
            //Temperatura y humedad
            float t = dht.readTemperature();
            Serial.println("Temperatura-> " + int(t));
            ThingSpeak.setField(1, t);
            float h = dht.readHumidity();
            Serial.println("Humedad-> " +int(h));
            ThingSpeak.setField(2, h);

            //funcion suelo
            lecturaSuelo();

            //Html pagina
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css' rel='stylesheet' integrity='sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3' crossorigin='anonymous'>");
            client.println("<link href='https://fonts.googleapis.com/css2?family=Noto+Sans:wght@400;700&display=swap' rel='stylesheet'>");
            client.println("<style>body{ font-family: 'Noto Sans', sans-serif; background-color: #f5f5f5; }</style>");
            client.println("<script src='https://kit.fontawesome.com/8e596f10f3.js' crossorigin='anonymous'></script>");

            client.println("<body><h1>ESP32 web server</h1>");
            client.println("<div class=\"container\">");
              client.println("<h1 class=\"display-2 text-center\">Panel de control <i class=\"fas fa-home-user\"></i></h1>");
              client.println("<hr>");
              client.println("<div class=\"row\">");
                //Tarjeta de temperatura
                client.println("<div class=\"col-md-6\">");
                  client.println("<div class=\"card mx-auto m3 shadow-sm\">");
                    client.println("<div class=\"card-header\"><i class=\"fas fa-thermometer-full></i> Temperatura");
                    client.println("<div class=\"card-body\">");
                      client.println("<h2 class=\"text-center fw-bold\">");client.println(t);client.println("</h2>");
                      client.println("<div class=\"d-flex justify-content-center\">");
                        client.println("<a class=\"btn btn-primary btn-sm\" href=\"https://thingspeak.com/channels/1580456/private_show\" targer=\"__blank\">Ver grafico</a>");
                      client.println("</div>");
                    client.println("</div>");
                  client.println("</div>");
                client.println("</div>");
                //Tarjeta de humedad
                client.println("<div class=\"col-md-6\">");
                  client.println("<div class=\"card mx-auto m3 shadow-sm\">");
                    client.println("<div class=\"card-header\"><i class=\"fas fa-thermometer-full></i> Humedad");
                    client.println("<div class=\"card-body\">");
                      client.println("<h2 class=\"text-center fw-bold\">");client.println(h);client.println("</h2>");
                      client.println("<div class=\"d-flex justify-content-center\">");
                        client.println("<a class=\"btn btn-primary btn-sm\" href=\"https://thingspeak.com/channels/1580456/private_show\" targer=\"__blank\">Ver grafico</a>");
                      client.println("</div>");
                    client.println("</div>");
                  client.println("</div>");
                client.println("</div>");
              client.println("</div>");
              client.println("</body></html>");

              client.println();
              break; //cerrar loop
          }else{
            lineaIngreso = "";
          }
        }else if(c != '\r'){
          lineaIngreso += c;
        }
      }
    }
    header = "";
    //Cerrar conexion
    client.stop();
    Serial.println("Cliente desconectado");
    Serial.println("");
  }
}

void correo(String message){
  datosSMTP.setLogin(host,port,loginEmail,loginPassword);
  datosSMTP.setSender("CASA /ESP32 ", loginEmail);
  datosSMTP.setPriority("Normal");
  datosSMTP.setSubject("Estado AIRE ACONDICIONADO");
  datosSMTP.setMessage("Hola soy ESP32/ CASA, envio estado del aire acondicionado", false);
  datosSMTP.addRecipient(accountEmail);
  if(!MailClient.sendMail(datosSMTP)){
    Serial.println("Error enviando el correo, " + MailClient.smtpErrorReason());
    datosSMTP.empty();
    delay(10000);
  }
}



