#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ADS1114 configuration
Adafruit_ADS1115 ads; // Use I2C address 0x48 (default), adjust if needed
const adsGain_t gain = GAIN_FOUR; // Gain setting for 1.024V full-scale range

// BLE configuration
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// Variable de modo (por defecto: MILI)
String modoActual = "MILI";

// Temporizador para enviar datos cada segundo
unsigned long previousMillis = 0;
const long intervalo = 1000; // En milisegundos
int contador = 0;  // Contador para los datos enviados

// Pin definitions for S1 and a
const int pinS1 = 25; // Pin for S1 signal (adjust as needed)
const int pinA = 26;  // Pin for a signal (adjust as needed)

// UUIDs: deben ser iguales a los que usas en App Inventor
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-ab12-cd34-ef56-1234567890ab"

// Clase que detecta cuando el teléfono se conecta o desconecta
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    pServer->getAdvertising()->start(); // Reanuda publicidad para reconectar
  }
};

// Clase que detecta cuando la app envía un comando (MILI, MICRO, AUTO)
class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue().c_str();
    if (rxValue.length() > 0) {
      String comando = rxValue;

      // Cambiar modo de operación si el comando recibido coincide
      if (comando == "MILI") {
        modoActual = "MILI";
        digitalWrite(pinS1, LOW);  // S1 = 0 -> Manual
        digitalWrite(pinA, HIGH);  // a = 1 -> Mili
      } else if (comando == "MICRO") {
        modoActual = "MICRO";
        digitalWrite(pinS1, LOW);  // S1 = 0 -> Manual
        digitalWrite(pinA, LOW);   // a = 0 -> Micro
      } else if (comando == "AUTO") {
        modoActual = "AUTO";
        digitalWrite(pinS1, HIGH); // S1 = 1 -> Auto
        // No need to set 'a' in Auto mode as it's controlled automatically
      }
    }
  }
};

void setup() {
  Serial.begin(115200);

  // Configurar pines S1 y a como salidas
  pinMode(pinS1, OUTPUT);
  pinMode(pinA, OUTPUT);
  digitalWrite(pinS1, LOW);  // Default to Manual
  digitalWrite(pinA, HIGH);  // Default to Mili

  // Inicializar ADS1114
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1114!");
    while (1);
  }
  ads.setGain(gain); // Set gain to GAIN_FOUR (1.024V)

  // Inicializar BLE y nombrar el dispositivo
  BLEDevice::init("MedidorCorriente");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Crear servicio BLE
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Crear característica BLE con permisos de lectura, escritura y notificación
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_WRITE_NR
                    );

  // Agregar descriptor para permitir notificaciones
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());

  // Iniciar servicio y publicidad BLE
  pService->start();
  pServer->getAdvertising()->start();
}

float readVoltage() {
  int16_t adcValue = ads.readADC_SingleEnded(0); // Leer canal 0 (ajusta según tu conexión)
  float voltage = ads.computeVolts(adcValue);    // Convertir a voltios usando la ganancia configurada

  // Corregir offset de 9mV
  voltage -= 0.009; // Restar 9mV
  if (voltage < 0) voltage = 0; // Evitar valores negativos

  return voltage;
}

float voltageToCurrent(float voltage) {
  // 1V = 100% = 100xA (mili o micro según modo)
  float percentage = voltage / 1.0; // Normalizar a 1V como 100%
  float current;

  if (modoActual == "MILI") {
    current = percentage * 100.0; // 1V = 100 mA
  } else if (modoActual == "MICRO") {
    current = percentage * 100.0; // 1V = 100 µA
  } else { // AUTO
    current = percentage * 100.0; // Calcular como si fuera MILI primero
    if (current < 1.0) { // Si es menor a 1mA, cambiar a MICRO
      modoActual = "MICRO";
      current = percentage * 100.0; // Recalcular para MICRO (1V = 100 µA)
    } else {
      modoActual = "MILI";
    }
  }

  return current;
}

void enviarDatos() {
  // Generar 100 datos dependiendo del modo activo
  for (int i = 0; i < 100; i++) {
    // Leer el voltaje del ADS1114
    float voltage = readVoltage();

    // Convertir a corriente según el modo
    float current = voltageToCurrent(voltage);

    // Enviar el valor de corriente por BLE con 3 decimales
    String dataToSend = String(current, 3); // 3 decimales
    pCharacteristic->setValue(dataToSend.c_str());
    pCharacteristic->notify();

    // Imprimir en consola para depurar
    Serial.print("Modo: ");
    Serial.print(modoActual);
    Serial.print(" | Voltaje (V): ");
    Serial.print(voltage, 3);
    Serial.print(" | Corriente: ");
    Serial.print(current, 3);
    if (modoActual == "MILI") {
      Serial.println(" mA");
    } else {
      Serial.println(" µA");
    }

    // Controlar el tiempo de espera sin bloquear
    unsigned long startMillis = millis();
    while (millis() - startMillis < 1000) {
      delay(1);
    }
  }
  contador = 0; // Reiniciar el contador después de enviar los 100 datos
}

void loop() {
  if (deviceConnected) {
    unsigned long currentMillis = millis();

    // Esperar intervalo de tiempo antes de enviar otra lectura
    if (currentMillis - previousMillis >= intervalo && contador == 0) {
      previousMillis = currentMillis;
      contador = 1;  // Inicia el envío de los 100 datos
      enviarDatos();  // Llamar a la función para generar y enviar datos
    }
  }
}