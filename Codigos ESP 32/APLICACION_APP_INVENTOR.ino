#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Característica BLE
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// Variable de modo (por defecto: MILI)
String modoActual = "MILI";

// Temporizador para enviar datos cada segundo
unsigned long previousMillis = 0;
const long intervalo = 1000; // En milisegundos
int contador = 0;  // Contador para los datos enviados

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
    String rxValue = pCharacteristic->getValue().c_str();  // Convertir std::string a String
    if (rxValue.length() > 0) {
      String comando = rxValue;

      // Cambiar modo de operación si el comando recibido coincide
      if (comando == "MILI") {
        modoActual = "MILI";
      } else if (comando == "MICRO") {
        modoActual = "MICRO";
      } else if (comando == "AUTO") {
        modoActual = "AUTO";
      }
    }
  }
};

void setup() {
  Serial.begin(115200);

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

void generarDatosAleatorios() {
  int valor = 0;

  // Generar 100 datos dependiendo del modo activo
  for (int i = 0; i < 100; i++) {
    if (modoActual == "MILI") {
      valor = random(100, 200);  // Genera datos aleatorios entre 100 y 200
    } else if (modoActual == "MICRO") {
      valor = random(1, 10);     // Genera datos aleatorios entre 1 y 10
    } else if (modoActual == "AUTO") {
      valor = analogRead(34);    // Lee un valor analógico (reemplazar con el pin real)
    }

    // Convertir el valor a cadena y enviarlo por BLE
    String dataToSend = String(valor);
    pCharacteristic->setValue(dataToSend.c_str());
    pCharacteristic->notify();

    // También se imprime en consola para depurar
    Serial.print("Modo: ");
    Serial.print(modoActual);
    Serial.print(" | Valor enviado: ");
    Serial.println(dataToSend);

    delay(1000);  // Espera un segundo entre los datos
  }
}

void loop() {
  if (deviceConnected) {
    unsigned long currentMillis = millis();

    // Esperar intervalo de tiempo antes de enviar otra lectura
    if (currentMillis - previousMillis >= intervalo && contador == 0) {
      previousMillis = currentMillis;
      contador = 1;  // Inicia el envío de los 100 datos
      generarDatosAleatorios();  // Llamar a la función para generar y enviar datos
    }
  }
}