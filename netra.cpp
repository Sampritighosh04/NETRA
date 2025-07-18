#include <SoftwareSerial.h>

// A9G GPS/GSM module pins
const int RXPin = 7, TXPin = 6;
const uint32_t GPSBaud = 9600;

// Ultrasonic sensors + motors
const int trigPin1 = 2;
const int echoPin1 = 3;
const int motorPin1 = 9;

const int trigPin2 = 4;
const int echoPin2 = 5;
const int motorPin2 = 10;

const int trigPin3 = 8;
const int echoPin3 = A0;  // Fixed (no conflict with motor)
const int motorPin3 = 11;

// Buzzer
const int buzzerPin = 12;

// Motor speeds
const int speedUnder40 = 240;
const int speedUnder50 = 220;
const int speedUnder60 = 200;
const int speedUnder70 = 180;
const int speedUnder80 = 160;
const int speedUnder90 = 140;
const int speedUnder100 = 120;
const int speedDefault = 0;

// Distance thresholds
const int threshold40 = 40;
const int threshold50 = 50;
const int threshold60 = 60;
const int threshold70 = 70;
const int threshold80 = 80;
const int threshold90 = 90;
const int threshold100 = 100;

// Google Maps URL base
String s = "www.google.com/maps/dir/";

// Timing
unsigned long interval = 10000;
unsigned long previousMillis = 0;
int data_counter = 0;

SoftwareSerial ss(RXPin, TXPin);

void setup() {
  Serial.begin(9600);
  ss.begin(GPSBaud);
  Serial.println("System Starting...");

  // A9G initialization
  sendATCommand("AT");
  sendATCommand("AT+GPS=1");
  sendATCommand("AT+CREG=2");
  sendATCommand("AT+CGATT=1");
  sendATCommand("AT+CGDCONT=1,\"IP\",\"WWW\"");
  sendATCommand("AT+CGACT=1,1");

  sendATCommand("AT+GPS=1");

  sendATCommand("AT+CMGF=1");

  // Pin modes
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(motorPin1, OUTPUT);

  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(motorPin2, OUTPUT);

  pinMode(trigPin3, OUTPUT);
  pinMode(echoPin3, INPUT);
  pinMode(motorPin3, OUTPUT);

  pinMode(buzzerPin, OUTPUT);

  analogWrite(motorPin1, speedDefault);
  analogWrite(motorPin2, speedDefault);
  analogWrite(motorPin3, speedDefault);
  digitalWrite(buzzerPin, LOW);
}

void loop() {
  unsigned long currentMillis = millis();

  long distance1 = readUltrasonicDistance(trigPin1, echoPin1);
  long distance2 = readUltrasonicDistance(trigPin2, echoPin2);
  long distance3 = readUltrasonicDistance(trigPin3, echoPin3);

  Serial.print("D1: "); Serial.print(distance1); Serial.print(" cm\t");
  Serial.print("D2: "); Serial.print(distance2); Serial.print(" cm\t");
  Serial.print("D3: "); Serial.println(distance3); Serial.print(" cm");

  int speed1 = getMotorSpeed(distance1);
  int speed2 = getMotorSpeed(distance2);
  int speed3 = getMotorSpeed(distance3);

  analogWrite(motorPin1, speed1);
  analogWrite(motorPin2, speed2);
  analogWrite(motorPin3, speed3);

  // Buzzer control
  if (distance1 > 0 && distance1 <= threshold40 ||
      distance2 > 0 && distance2 <= threshold40 ||
      distance3 > 0 && distance3 <= threshold40) {
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(buzzerPin, LOW);
  }

  // GPS send
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    send_gps_data();
  }

  delay(100);
}

void sendATCommand(String command) {
  ss.println(command);
  delay(1000);
  while (ss.available()) {
    Serial.write(ss.read());
  }
}

void send_gps_data() {
  ss.println("AT+LOCATION=2");
  delay(2000);

  String gpsData = "";
  while (ss.available()) {
    char c = ss.read();
    gpsData += c;
  }

  Serial.println("GPS Response:");
  Serial.println(gpsData);

  float latitude = 0.0;
  float longitude = 0.0;

  int latIndex = gpsData.indexOf(",", gpsData.indexOf(",") + 1) + 1;
  int lngIndex = gpsData.indexOf(",", latIndex) + 1;

  if (latIndex > 0 && lngIndex > 0) {
    latitude = gpsData.substring(latIndex, gpsData.indexOf(",", latIndex)).toFloat();
    longitude = gpsData.substring(lngIndex, gpsData.indexOf(",", lngIndex)).toFloat();
  }

  if (latitude == 0 || longitude == 0) {
    Serial.println("Invalid GPS data.");
    return;
  }

  data_counter++;
  Serial.print("Latitude: "); Serial.println(latitude, 6);
  Serial.print("Longitude: "); Serial.println(longitude, 6);
  Serial.println(data_counter);

  s += String(latitude, 6);
  s += ",";
  s += String(longitude, 6);
  s += "/";

  Serial.println("Google Maps URL:");
  Serial.println(s);

  if (data_counter >= 10) {
    data_counter = 0;
    Serial.println("Sending SMS...");
    sendATCommand("AT+CMGF=1");
    sendATCommand("AT+CNMI=2,2,0,0,0");
    ss.print("AT+CMGS=\"+919339858145\"\r");  // Your number
    delay(1000);
    ss.print(s);
    ss.write(0x1A);
    delay(3000);
    s = "www.google.com/maps/dir/";
  }
}

long readUltrasonicDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  long distance = duration * 0.034 / 2;

  if (duration == 0) {
    return -1;
  } else {
    return distance;
  }
}

int getMotorSpeed(long distance) {
  if (distance <= threshold40) return speedUnder40;
  else if (distance <= threshold50) return speedUnder50;
  else if (distance <= threshold60) return speedUnder60;
  else if (distance <= threshold70) return speedUnder70;
  else if (distance <= threshold80) return speedUnder80;
  else if (distance <= threshold90) return speedUnder90;
  else if (distance <= threshold100) return speedUnder100;
  else return speedDefault;
}
