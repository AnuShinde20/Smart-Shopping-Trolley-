#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "quirc.h"
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>  // Include the ESP32Servo library

/* ======================================== */

// creating a task handle
TaskHandle_t QRCodeReader_Task;

/* Select camera model */
#define CAMERA_MODEL_AI_THINKER
/* ======================================== */

/* ======================================== GPIO of camera models */
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

/* Define LED GPIO pin */
#define LED_GPIO_NUM 4  // You can choose any available GPIO pin
#define WIFI_STATUS_LED_PIN 13 


/* Define servo pin and angles */
#define SERVO_PIN 14  // You can choose any available GPIO pin for the servo
#define SERVO_PIN2 15 
#define BUZZER_PIN 12       // GPIO for Buzzer
#define SERVO_OPEN_ANGLE 130
#define SERVO_CLOSED_ANGLE 0
#define SERVO2_OPEN_ANGLE 130
#define SERVO2_CLOSED_ANGLE 10
Servo myServo;  // Create a servo object
Servo myServo2;  
/* ======================================== */

/* ======================================== Firebase configuration */
#define FIREBASE_HOST "trial-eb597-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyATqXMDqF_81Fn1wp4JzRW2c9qcLR7DQrA"
#define WIFI_SSID "Deven123"
#define WIFI_PASSWORD "deven3112"
/* ======================================== */

/* ======================================== Variables declaration */
struct QRCodeData {
  bool valid;
  int dataType;
  uint8_t payload[1024];
  int payloadLen;
};

struct quirc *q = NULL;
uint8_t *image = NULL;
camera_fb_t *fb = NULL;
struct quirc_code code;
struct quirc_data data;
quirc_decode_error_t err;
struct QRCodeData qrCodeData;
String QRCodeResult = "";

FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;
int productCount = 1;  // Counter to dynamically assign product IDs
/* ======================================== */

const String ownerQRCode = "devil";  // Replace with the actual predefined QR code data for the owner


/* __________ VOID SETUP() */
void setup() {
  // Disable brownout detector.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Init serial communication speed (baud rate).
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Camera configuration.
  Serial.println("Start configuring and initializing the camera...");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 15;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);

  Serial.println("Configure and initialize the camera successfully.");
  Serial.println();

  // Initialize the LED pin
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);

   // Initialize the Wi-Fi status LED
  pinMode(WIFI_STATUS_LED_PIN, OUTPUT);
  digitalWrite(WIFI_STATUS_LED_PIN, LOW);  

  // Initialize the servo motor
  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_CLOSED_ANGLE);  // Start with the servo in the closed position

  myServo2.attach(SERVO_PIN2);
  myServo2.write(SERVO2_CLOSED_ANGLE);  // Start with the second servo in the closed position
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Start with buzzer off

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFI_STATUS_LED_PIN, LOW); 
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Turn on the Wi-Fi status LED to indicate connection
  digitalWrite(WIFI_STATUS_LED_PIN, HIGH);

  // Initialize Firebase
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);


  // Create "QRCodeReader_Task" using the xTaskCreatePinnedToCore() function
  xTaskCreatePinnedToCore(
    QRCodeReader,         // Task function.
    "QRCodeReader_Task",  // name of task.
    10000,                // Stack size of task
    NULL,                 // parameter of the task
    1,                    // priority of the task
    &QRCodeReader_Task,   // Task handle to keep track of created task
    0);                   // pin task to core 0
}

void loop() {
   // Check Wi-Fi connection status and update LED
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(WIFI_STATUS_LED_PIN, HIGH);  // Keep LED ON when connected
  } else {
    digitalWrite(WIFI_STATUS_LED_PIN, LOW);  // Turn LED OFF when disconnected
  }
  delay(1);
}

/* Add a new variable to track owner mode state */
/* Add a new variable to track owner mode state */
bool ownerModeActive = false;  // Initially, owner mode is off

void QRCodeReader(void *pvParameters) {
  Serial.println("QRCodeReader is ready.");
  Serial.print("QRCodeReader running on core ");
  Serial.println(xPortGetCoreID());
  Serial.println();

  while (1) {
    q = quirc_new();
    if (q == NULL) {
      Serial.print("Can't create quirc object\r\n");
      continue;
    }

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      continue;
    }

    quirc_resize(q, fb->width, fb->height);
    image = quirc_begin(q, NULL, NULL);
    memcpy(image, fb->buf, fb->len);
    quirc_end(q);

    int count = quirc_count(q);
    if (count > 0) {
      quirc_extract(q, 0, &code);
      err = quirc_decode(&code, &data);

      if (err) {
        Serial.println("Decoding FAILED");
        QRCodeResult = "Decoding FAILED";
      } else {
        Serial.printf("Decoding successful:\n");
        dumpData(&data);  // Store the QR data in QRCodeResult

        // Flash the LED for 1 second upon successful decoding
        digitalWrite(LED_GPIO_NUM, HIGH);
        delay(1000);
        digitalWrite(LED_GPIO_NUM, LOW);

        if (QRCodeResult == ownerQRCode) {
          if (ownerModeActive) {
            Serial.println("Owner QR code detected again. Deactivating owner mode...");
            ownerModeActive = false;
            myServo.write(SERVO_CLOSED_ANGLE);  // Close the servo
            myServo2.write(SERVO2_CLOSED_ANGLE);  // Close second servo
            Serial.println("Servo OFF: Deactivated with owner mode.");
          } else {
            Serial.println("Owner QR code detected. Activating owner mode...");
            ownerModeActive = true;
            enterOwnerMode();
            
            Serial.println("Servo ON: Opening at 90 degrees.");
            myServo.write(SERVO_OPEN_ANGLE);  // Open servo
            myServo2.write(SERVO2_OPEN_ANGLE);  // Open second servo
              delay(7000);  // Wait for 7 seconds
          }
        } else if (ownerModeActive) {  // Handle only in owner mode
          if (isProductInFirebase(QRCodeResult)) {
            Serial.println("Duplicate product detected. Removing from Firebase...");
            removeProductFromFirebase(QRCodeResult);  // Remove product from Firebase
          } else {
            Serial.println("Product not found in Firebase. No action taken.");
          }
        } else {  // Handle new products when not in owner mode
          if (!isProductInFirebase(QRCodeResult)) {
            Serial.println("New product detected. Adding to Firebase...");
            sendDataToFirebase(QRCodeResult);

            // Serial.println("Servo ON: Opening at 90 degrees.");
            // myServo.write(SERVO_OPEN_ANGLE);  // Open servo
            // myServo2.write(SERVO2_OPEN_ANGLE);  // Open second servo
            //   delay(7000);  // Wait for 7 seconds

// Start buzzer for 3 seconds
Serial.println("Buzzer ON: Beeping for 3 seconds.");
digitalWrite(BUZZER_PIN, HIGH);  
delay(3000);  // Buzzer beeps for 3 seconds
digitalWrite(BUZZER_PIN, LOW);  // Turn off buzzer

// Close servo after 10 seconds in total
Serial.println("Servo OFF: Closing at 0 degrees.");
myServo.write(SERVO_CLOSED_ANGLE);  
myServo2.write(SERVO2_CLOSED_ANGLE);  // Close second servo

          } else {
            Serial.println("Duplicate QR code detected. No action taken.");
          }
        }
      }
    }

    esp_camera_fb_return(fb);
    fb = NULL;
    image = NULL;
    quirc_destroy(q);

    if (ownerModeActive) {
      myServo.write(SERVO_OPEN_ANGLE);  // Keep the servo open in owner mode
    }
  }
}

/* Function to remove a product from Firebase */
void removeProductFromFirebase(String data) {
  String model, name, price;

  // Split the data into components
  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);

  if (firstComma != -1 && secondComma != -1) {
    model = data.substring(0, firstComma);
    name = data.substring(firstComma + 1, secondComma);
    price = data.substring(secondComma + 1);
  }

  // Find the product in Firebase
  for (int i = 1; i <= productCount; i++) {
    String productPath = "/Cart/P" + String(i) + "/Model_no";

    if (Firebase.getString(firebaseData, productPath)) {
      if (firebaseData.stringData() == model) {
        String productRoot = "/Cart/P" + String(i);
        Firebase.deleteNode(firebaseData, productRoot);  // Remove the product
        Serial.println("Product removed from Firebase: " + model);
        return;  // Exit after removing the product
      }
    } else {
      Serial.println("Error fetching product from Firebase.");
    }
  }

  Serial.println("Product not found in Firebase.");
}

/* Keep the rest of your code as it is... */

/* Function to handle owner-specific operations */
void enterOwnerMode() {
  Serial.println("Owner mode activated.");
  myServo.write(SERVO_OPEN_ANGLE);  // Open servo as soon as owner mode is activated
  myServo2.write(SERVO2_OPEN_ANGLE); 
  Serial.println("Servo On in owner mode.");
  // Add more logic here if needed for owner mode operations
}


void dumpData(const struct quirc_data *data) {
  Serial.printf("Version: %d\n", data->version);
  Serial.printf("ECC level: %c\n", "MLHQ"[data->ecc_level]);
  Serial.printf("Mask: %d\n", data->mask);
  Serial.printf("Length: %d\n", data->payload_len);
  Serial.printf("Payload: %s\n", data->payload);

  QRCodeResult = (const char *)data->payload;
}

bool isProductInFirebase(String data) {
  String model, name, price;

  // Split the QR code data into components
  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);

  if (firstComma != -1 && secondComma != -1) {
    model = data.substring(0, firstComma);
    name = data.substring(firstComma + 1, secondComma);
    price = data.substring(secondComma + 1);
  }

  // Iterate through existing products in Firebase
  for (int i = 1; i <= productCount; i++) {
    String productPath = "/Cart/P" + String(i) + "/Model_no";

    // Retry logic: Attempt to fetch the product 3 times if it fails
    for (int attempt = 1; attempt <= 3; attempt++) {
      if (Firebase.getString(firebaseData, productPath)) {
        String storedModel = firebaseData.stringData();
        if (storedModel == model) {
          // If the model matches, the product is already registered
          return true;
        }
      } else {
        Serial.println("Error fetching product from Firebase. Retrying...");
        delay(500);  // Small delay before retrying
      }
    }
    Serial.println("Failed to fetch product after 3 attempts.");
  }

  // Product not found in Firebase
  return false;
}

void sendDataToFirebase(String data) {
  String name, model, price;

  // Split the data from the QR code using commas
  int firstComma = data.indexOf(',');
  int secondComma = data.indexOf(',', firstComma + 1);

  if (firstComma != -1 && secondComma != -1) {
    model = data.substring(0, firstComma);
    name = data.substring(firstComma + 1, secondComma);
    price = data.substring(secondComma + 1);
  }

  String productPath = "/Cart/P" + String(productCount);  // Dynamic product path

  // Ensure all Firebase operations succeed
  bool success = true;
  success &= Firebase.setString(firebaseData, productPath + "/Model_no", model);
  success &= Firebase.setString(firebaseData, productPath + "/Name", name);
  success &= Firebase.setString(firebaseData, productPath + "/Price", price);

  if (success) {
    Serial.println("Data sent to Firebase successfully:");
    Serial.println("Model: " + model);
    Serial.println("Name: " + name);
    Serial.println("Price: " + price);
    productCount++;  // Increment counter for next product
  } else {
    Serial.println("Error sending data to Firebase.");
  }
}
