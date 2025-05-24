int TIME_TO_SLEEP = 10;                        // Time ESP32 will go to sleep (in seconds)

unsigned long long uS_TO_S_FACTOR = 1000000;  // Conversion factor for microseconds to seconds

RTC_DATA_ATTR int bootCount = 0;              // Number of reboots

void setup() {

  /****  Do your stuff here! ****/

  Serial.begin(115200);                                 // Start serial communication at 115200 baud rate

  ++bootCount;                                          // Add 1 to the current value of bootCount

  Serial.println("Boot number: " + String(bootCount));  // print the value of bootCount on the serial monitor

  Serial.println("Going to sleep now");                 // Print when the ESP is about to go into deep sleep mode

  /* Now we wrap up for Deep Sleep - I hope you did everything you needed to... */

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);  // Set up timer as the wake up source and set sleep duration to 5 seconds

  Serial.flush();                                                 // Waits for the transmission of outgoing serial data to complete.

  esp_deep_sleep_start();                                         // Start the deep sleep mode

}

void loop() {

  // This is not going to be called

}