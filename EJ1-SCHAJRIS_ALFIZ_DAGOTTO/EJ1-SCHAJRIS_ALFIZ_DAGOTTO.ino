// GRUPO 8 - SCHAJRIS DAGOTTO ALFIZ - EJERCICIO 1 (único)

//Librerías de wifi
#include <WiFi.h>                   
#include <WiFiClientSecure.h>    

//Librerías del bot de telegram   
#include <UniversalTelegramBot.h>   

//Librerías de arduino
#include <ArduinoJson.h>            
#include <Arduino.h>   

//Librería del display             
#include <U8g2lib.h>   

//LIbrerías del sensor de temperatura             
#include <DHT.h>                    
#include <DHT_U.h>                  


// Defino estados de nuestra máquina de estados
#define PANTALLA1 1
#define ESTADO_CODIGO1 2
#define ESTADO_CODIGO2 3
#define ESTADO_CODIGO3 4
#define ESTADO_CODIGO4 5
#define ESTADO_CODIGO5 6
#define PANTALLA2 7
#define ESTADO_CONFIRMACION2 8
#define SUBIR_UMBRAL 9
#define BAJAR_UMBRAL 10

// Telegram BOT Token (Get from Botfather). El token sirve para que solo los que conozcan el token puedan controlar el bot
#define BOT_TOKEN "7156419115:AAEFRfW9Ofrzhk0dFDDwa64K4I34iHg9xWo" 
// El ID del grupo. Sirve para que solo mensajes viniendo de un chat específico (el id) sean tomados en cuenta.
#define CHAT_ID "7568706599"


#define DHTPIN 23      // pin del dht11
#define DHTTYPE DHT11  // tipo de dht (hay otros)

DHT dht(DHTPIN, DHTTYPE); // defino el DHT (el sensor necesita ser iniciado con esta función) (Más abajo la pantallita es iniciada con una función similar)

//Botones y en que pin están
#define PIN_BOTON1 35
#define PIN_BOTON2 34

//LEDs
#define PIN_LED 25

//Estados de los botones
#define PULSADO LOW
#define N_PULSADO !PULSADO

int sentTemp = 0;
bool sentUmbral = false;

//Valores usados para el delay sin bloqueo
unsigned long TiempoUltimoCambio = 0; //Variable que almacenará millis.
const long INTERVALO = 1000; //Cuanto tiempo dura el delay sin bloqueo
unsigned long TiempoAhora = millis(); //Guarda los milisegundos desde que se ejecutó el programa
unsigned long TiempoConteo = 0;


bool cambioHecho = LOW; //cambioHecho se usa para que al apretar cualquier boton, el valor aumente 1 sola vez
bool umbralSuperado = false; //Determina si se supera el umbral y hay que mandar un mensaje desde el bot
float umbral = 21.00; //El valor del umbral
float CAMBIO_UMB = 1;   //Cuanto cambia el umbral


int estadoActual = PANTALLA1;  // La máquina de estados inicia en PANTALLA1. Más abajo se usa esta variable para cambiar de estado.

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);  //defino la pantallita

WiFiClientSecure secured_client; //Garantiza conexión segura
UniversalTelegramBot bot(BOT_TOKEN, secured_client); //Función que define que bot vamos a usar. A nosotros nos interesa el TOKEN, ya que del secured_client se encarga el código

//Otorgamos acceso a Time.h a wifi
const char* SSID = "Tete3";
const char* PASSWORD = "potines2430";


//Defino las 2 tasks que van a correr en simultáneo (y sus handlers. a cada task hay que asignarle un handler).
TaskHandle_t Telegram; //La task 1 que maneja el uso del telegram
TaskHandle_t Maquina; //La task 2 que maneja la máquina de estados

TaskHandle_t Task1;
TaskHandle_t Task2;

//Cada 1 segundo, revisa si hay nuevos mensajes
int botRequestDelay = 3000;
unsigned long lastTimeBotRan;

void setup() {
  Serial.begin(115200); //Baudios (monitor serie)

  // Pines configurados como entradas normales (sin pull-up porque 34 y 35 no lo soportan)
  pinMode(PIN_BOTON1, INPUT);
  pinMode(PIN_BOTON2, INPUT);
  pinMode(PIN_LED, OUTPUT);

  dht.begin();   // inicializo el dht          (No confundir estas dos funciones de inicialización con las definiciones arriba)
  u8g2.begin();  //inicializo la pantallita



  //Otorgamos acceso a nuestro ESP32 a wifi con datos constantes definidos anteriormente
  WiFi.begin(SSID, PASSWORD);
  Serial.println("Conectando a wifi"); //Confirmación de inicio de intento de conexión
  TiempoUltimoCambio = millis(); //Cuantos milisegundos pasaron desde el último cambio
  while (WiFi.status() != WL_CONNECTED) { //Mientras el wifi esté conectándose, que cada un intervalo imprima que intenta conectarse
    TiempoAhora = millis();
    if (TiempoAhora - TiempoUltimoCambio >= INTERVALO) {
      TiempoUltimoCambio = millis();
      Serial.println("Intentando conectar..."); //Cada tanto, enviar un "Intentando conectar..." para avisar que el código no está trabado.
    }

  }  
  Serial.println("Conexión exitosa"); //Cuando sale del while (Se conecta), avisa del éxito de conexión

  secured_client.setInsecure();

  bot.sendMessage(CHAT_ID, "¡Bot encendido!", "");// Cuando se conecte, el bot debería enviar este mensaje a el chat de telegram
  

  xTaskCreatePinnedToCore( //Esto es lo más importante del ejercicio, los 2 loops. Acá lo que hacemos es crear 2 tareas (Líneas 104 a 112 y 113 a 120).
    Task1code,   /* Función que implementa la tarea. */
    "Telegram",  /* nombre del loop/tarea. */
    10000,       /* Tamaño que puede tener. */
    NULL,        /* Parámetro que va a recibir.*/
    1,           /* La prioridad que va a tener la tarea. */
    &Task1,      /* Task handle, hace un seguimiento de la tarea. */
    0);          /* Asignar tarea al núcleo 0. */                

  xTaskCreatePinnedToCore( //Lo más importante de estas definiciones, es que a cada una le asignamos un núcleo (o core). La cantidad de cores que tiene nuestra placa es lo que determina cuantos loops podemos tener funcionando simultáneamente. El Arduino UNO tiene UNO, y el ESP32 tiene DOS.
    Task2code,   /* Función que implementa la tarea. */
    "Maquina",   /* nombre del loop/tarea. */
    10000,       /* Tamaño que puede tener. */
    NULL,        /* Parámetro que va a recibir.*/
    1,           /* La prioridad que va a tener la tarea. */
    &Task2,      /* Task handle, hace un seguimiento de la tarea. */
    1);          /* Asignar tarea al núcleo 1. */
}

void handleNewMessages(int numNewMessages) { //Función cuyo propósito es recibir los mensajes enviados al bot y procesarlos
  Serial.print("Handle New Messages: ");

  float temperatura = dht.readTemperature();

  Serial.println(temperatura); //Muestra cuantos mensajes esá "handleando".

  for (int i = 0; i < numNewMessages; i++) { //Básicamente es como decir "Mientras haya mensajes que revisar:"
    String chat_id = String(bot.messages[i].chat_id); //guarda el id del chat desde el que se mandó un mensaje para el bot.
    if (chat_id != CHAT_ID){   //Si el mensaje viene de un usuario con un chat id diferente al que le especificamos previamente:
      bot.sendMessage(chat_id, "Unauthorized user", ""); //Le avisamos que su uso del bot no fue autorizado.
      continue; //Sigue revisando mensajes
    }
    
    // Si el mensaje sobrevive al chequeo de id, se imprime en el monitor serial.
    String text = bot.messages[i].text;
    Serial.println(text);
    
    String from_name = bot.messages[i].from_name; //Comando Start, que se usa normalmente para iniciar la conversación con el bot. Devuelve el string "welcome", que da un glosario de los comandos del bot.
    if (text == "/start") {
      String welcome = "Bienvenido, " + from_name + "\n";
      welcome += "¡Los siguientes comandos se pueden usar para interactuar con tu ESP 32! \n";
      welcome += "/temp : devuelve la temperatura actual\n";
      bot.sendMessage(CHAT_ID, welcome, "");
    }
    if (text == "/temp") { //Si el mensaje es "/temp", envío la temperatura leída.
      String sendTemp = "la temperatura registrada fue de";
      sendTemp += temperatura;

      bot.sendMessage(CHAT_ID, sendTemp, "");
    }

  }
}

void Task1code (void * parameter) { //Este es uno de los loops. Éste se usa para el bot de telegram.
  for(;;) {
    Serial.println(estadoActual);
    Serial.print (umbral);
    float temperatura = dht.readTemperature(); //Guarda la lectura del sensor de temperatura en una variable

    if (millis() > lastTimeBotRan + botRequestDelay)  {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      lastTimeBotRan = millis();
    }
    
    if (umbralSuperado){ //se explica solo
      digitalWrite(PIN_LED, HIGH);
      String superado = "Temperatura por arriba de los niveles de umbral. Temperatura actual:" + int(temperatura);
      bot.sendMessage(CHAT_ID, superado, "");
      Serial.print (superado);
      sentUmbral = true;
      umbralSuperado = false;
    }

  }


}

void Task2code (void * parameter) { // El Siguiente loop se encarga de lo que es la máquina de estado. Esto lo hablamos juntos en persona

  bool boton1PresionadoAntes = false;
  bool boton2PresionadoAntes = false;

  for(;;) {
    
    TiempoAhora = millis(); //Milisegundos desde que se ejecutó el código
    float temperatura = dht.readTemperature();

    if (temperatura > umbral){
      if (sentUmbral == false){
        umbralSuperado = true;
      }
    }
    else{
      sentUmbral = false;
      digitalWrite(PIN_LED, LOW);
      umbralSuperado = false;
    }

    //Lectura de botones
    bool lecturaBoton1 = digitalRead(PIN_BOTON1);
    bool lecturaBoton2 = digitalRead(PIN_BOTON2);

    bool boton1Liberado = (boton1PresionadoAntes == true && lecturaBoton1 == N_PULSADO);
    bool boton2Liberado = (boton2PresionadoAntes == true && lecturaBoton2 == N_PULSADO);
    
    switch (estadoActual) { //Empieza en el estado que estadoActual tiene como valor

      case PANTALLA1: //Pantalla 1, muestra hora establecida y temperatura
        if (TiempoAhora - TiempoUltimoCambio >= INTERVALO) { //Delay sin bloqueo
          TiempoUltimoCambio = millis(); //actualizo el tiempo


          char bufferTemperatura[10]; //Reserva espacios en el búfer para la variable de temperatura
       
          sprintf(bufferTemperatura, "%.2f", temperatura); //Guarda la variable temperatura en los espacios reservados para bufferTemperatura


          u8g2.clearBuffer(); //Borra lo almacenado en el búfer anteriormente
          u8g2.setFont(u8g2_font_helvB10_tf); //La fuente del texto del oled
          u8g2.drawStr(10, 40, "Temperatura");
          u8g2.drawStr(10, 55, bufferTemperatura);
          u8g2.sendBuffer(); //Envía los datos guardados en el búfer a la pantalla
        }

        if (lecturaBoton1 == PULSADO) { 
          Serial.print("número correcto");
          estadoActual = ESTADO_CODIGO1;
        }
      break;

      case ESTADO_CODIGO1:
        if (TiempoConteo == 0){
          TiempoConteo = TiempoAhora;
        }

        if (TiempoAhora >= TiempoConteo + 5000){
          estadoActual = PANTALLA1;
          TiempoConteo = 0;
        }
        
        if (lecturaBoton2 == PULSADO){

         estadoActual = PANTALLA1;
        }
        if (lecturaBoton1 == N_PULSADO) {
          Serial.print("número correcto");
          estadoActual = ESTADO_CODIGO2;
        }

      break;

      case ESTADO_CODIGO2:
        if (TiempoConteo == 0){
          TiempoConteo = TiempoAhora;
        }

        if (TiempoAhora >= TiempoConteo + 5000){
          estadoActual = PANTALLA1;
          TiempoConteo = 0;
        }
        
        if (lecturaBoton1 == PULSADO){

         estadoActual = PANTALLA1;
        }
        if (lecturaBoton2 == PULSADO) { 
          Serial.print("número correcto");
          estadoActual = ESTADO_CODIGO3;
        }

      break;

      case ESTADO_CODIGO3:
        if (TiempoConteo == 0){
          TiempoConteo = TiempoAhora;
        }

        if (TiempoAhora >= TiempoConteo + 5000){
          estadoActual = PANTALLA1;
          TiempoConteo = 0;
        }
        
        if (lecturaBoton1 == PULSADO){
          
         estadoActual = PANTALLA1;
        }
        if (boton2Liberado) {
          Serial.print("número correcto");
          estadoActual = ESTADO_CODIGO4;
        }

      break;

      case ESTADO_CODIGO4:
        if (TiempoConteo == 0){
          TiempoConteo = TiempoAhora;
        }

        if (TiempoAhora >= TiempoConteo + 5000){
          estadoActual = PANTALLA1;
          TiempoConteo = 0;
        }
        
        if (lecturaBoton2 == PULSADO){
          
         estadoActual = PANTALLA1;
        }
        if (lecturaBoton1 == PULSADO) { 
          Serial.print("número correcto");
          estadoActual = ESTADO_CODIGO5;
        }

      break;

      case ESTADO_CODIGO5:
        if (TiempoConteo == 0){
          TiempoConteo = TiempoAhora;
        }

        if (TiempoAhora >= TiempoConteo + 5000){
          estadoActual = PANTALLA1;
          TiempoConteo = 0;
        }
        
        if (lecturaBoton2 == PULSADO){
          
         estadoActual = PANTALLA1;
        }
        if (boton1Liberado) {
          Serial.print("número correcto");
          estadoActual = PANTALLA2;
        }

      break;


      case PANTALLA2:
        if (TiempoAhora - TiempoUltimoCambio >= INTERVALO){  ///delay sin bloqueo
          TiempoUltimoCambio = millis();  /// importante actualizar el tiempo
          char bufferUmbral[5]; //Reserva espacios en el búfer para la variable del umbral


          sprintf(bufferUmbral, "%.2f", umbral); //Guarda la variable GMTActual en los espacios reservados para bufferUmbral


          u8g2.clearBuffer(); //Véase línea 106 a 114
          u8g2.setFont(u8g2_font_helvB10_tf);
          u8g2.drawStr(5, 25, "Temperatura umbral:");
          u8g2.drawStr(5, 45, bufferUmbral);
          u8g2.sendBuffer();
        }


        if (lecturaBoton1 == PULSADO && lecturaBoton2 == PULSADO) { //Si los dos botones son pulsados, se sale de pantalla 2 y se entra a un estado de confirmación (Línea 162)
          estadoActual = ESTADO_CONFIRMACION2;
        }


        if (lecturaBoton2 == PULSADO) { //Si solo el boton 2 es pulsado, entra al estado de bajar hora (Línea 192) y cambioHecho devuelve True
          cambioHecho = HIGH;
          estadoActual = BAJAR_UMBRAL;
          //Serial.println(cambioHecho);
        }

        if (lecturaBoton1 == PULSADO) { //Si solo el boton 1 es pulsado, entra al estado de subir hora (Línea 169) y cambioHecho devuelve True
          cambioHecho = HIGH;
          estadoActual = SUBIR_UMBRAL;
          //Serial.println(cambioHecho);
        }
      break;




      case ESTADO_CONFIRMACION2:
        if (lecturaBoton1 == N_PULSADO && lecturaBoton2 == N_PULSADO) { //Si los dos botones se dejan de pulsar, se entra a la pantalla 1 (Línea 96)
          estadoActual = PANTALLA1;
        }
      break;




      case SUBIR_UMBRAL:
        if (lecturaBoton2 == PULSADO) { //Si el boton 2 es pulsado, se entra al estado de confirmación 2 (para llegar a SUBIR_HORA se necesitaba pulsar el boton 2)
          estadoActual = ESTADO_CONFIRMACION2;
        }


        if (lecturaBoton1 == N_PULSADO) { //Si se deja de presionar el boton 1 sin que se presione el boton 2

          if (cambioHecho == HIGH) { //Si cambio hecho sigue siendo HIGH, el umbral aumenta 1, y se imprime en el monitor serial
            umbral = umbral + CAMBIO_UMB;
            cambioHecho = LOW;
            //Serial.println(GMTActual);
          }
          estadoActual = PANTALLA2;
        }
      break;

      case BAJAR_UMBRAL:
        if (lecturaBoton1 == PULSADO) { //Si el boton 1 es pulsado, se entra al estado de confirmación 2 (para llegar a BAJAR_HORA se necesitaba pulsar el boton 1)
          estadoActual = ESTADO_CONFIRMACION2;
        }


        if (lecturaBoton2 == N_PULSADO) { //Si se deja de presionar el boton 2 sin que se presione el boton 1

          if (cambioHecho == HIGH) { //Si cambio hecho sigue siendo HIGH, la hora aumenta en 1, y se imprime en el monitor serial
            umbral = umbral - CAMBIO_UMB;
            cambioHecho = LOW;
          }
          estadoActual = PANTALLA2;
        }
      break;
  }
  boton1PresionadoAntes = (lecturaBoton1 == PULSADO);
  boton2PresionadoAntes = (lecturaBoton2 == PULSADO);
  }

}
void loop ()
{}