#include <Wire.h>
#include <Adafruit_ADS1X15.h>  // Biblioteca para ADS1114 y ADS1015

Adafruit_ADS1115 ads;  // Crear el objeto para el ADS1114

void setup() {
  Serial.begin(115200);  // Iniciar la comunicación serial
  
  // Inicialización del bus I2C
  Wire.begin(21, 22);    // Configuración de los pines SDA (21) y SCL (22)
  
  // Inicialización del ADC ADS1114
  if (!ads.begin()) {
    Serial.println("No se pudo encontrar un dispositivo ADS1114.");
    while (1);  // Detener la ejecución si no se encuentra el ADS1114
  }

  // Configurar el ADC para usar un rango de ±2.048V
  ads.setGain(GAIN_TWO);  // Establecer la ganancia para ±2.048V
}

void loop() {
  // Leer el valor del canal 0 (entrada simple de un solo extremo)
  int16_t adc0 = ads.readADC_SingleEnded(0);
  
  // Convertir el valor digital a voltaje (usando el rango de ±2.048V)
  float voltaje = (adc0 / 32768.0) * 2.048;
  
  // Mostrar el voltaje calculado en el monitor serial
  Serial.print("Voltaje del canal 0: ");
  Serial.print(voltaje, 4);  // Mostrar el voltaje con 4 decimales
  Serial.println(" V");
  
  delay(1000);  // Esperar 1 segundo antes de la siguiente lectura
}
