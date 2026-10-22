#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Wi-Fi Access Point Credentials
const char* ssid = "Humanoid_Robot_AP";
const char* password = "robotpassword123";

WebServer server(80);
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver();

#define SERVOMIN  125 // ~0 degrees
#define SERVOMAX  575 // ~180 degrees
#define TOTAL_SERVOS 16 

// Joint/Servo Labels for 16 channels
String servoNames[TOTAL_SERVOS] = {
  "0: Right Hip Yaw", "1: Right Hip Roll", "2: Right Hip Pitch", 
  "3: Right Knee", "4: Right Ankle Pitch", "5: Left Hip Yaw", 
  "6: Left Hip Roll", "7: Left Hip Pitch", "8: Left Knee", 
  "9: Left Ankle Pitch", "10: Right Shoulder", "11: Right Elbow", 
  "12: Left Shoulder", "13: Left Elbow", "14: Head/Waist", "15: Extra / Gripper"
};

int servoAngles[TOTAL_SERVOS]; // Store current angles

// Preset Angle Arrays
int standAngles[TOTAL_SERVOS]       = {73, 73, 90, 96, 92, 80, 90, 90, 87, 78, 84, 74, 79, 77, 75, 79};
int handsFrontAngles[TOTAL_SERVOS]  = {73, 73, 90, 96, 92, 80, 90, 90, 87, 78, 84, 121, 79, 77, 21, 79};
int handsUpAngles[TOTAL_SERVOS]     = {73, 73, 90, 96, 92, 80, 90, 90, 87, 78, 0, 121, 79, 162, 21, 79};
int all90Angles[TOTAL_SERVOS]       = {90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90};

// Pushup Frame Arrays
int pushupFrameA[TOTAL_SERVOS];
int pushupFrameB[TOTAL_SERVOS];

// Loop Flags
bool pushupActive = false;
bool exerciseActive = false;

int angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVOMIN, SERVOMAX);
}

// Smooth transition function that is interruptible by active loops
bool smoothTransitionInterruptible(int targetArray[]) {
  int steps = 40;       // Number of incremental steps
  int stepDelay = 15;   // Delay per step
  
  float startAngles[TOTAL_SERVOS];
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    startAngles[i] = servoAngles[i];
  }
  
  for (int step = 1; step <= steps; step++) {
    server.handleClient(); // Keep web server active and responsive
    
    // Abort if loops were stopped
    if (!pushupActive && !exerciseActive && (targetArray == pushupFrameA || targetArray == pushupFrameB)) {
      return false; 
    }
    if (!exerciseActive && (targetArray == standAngles || targetArray == handsFrontAngles || targetArray == handsUpAngles)) {
      // If running exercise routine, check flag
      // (Handled via specific checks)
    }
    
    for (int i = 0; i < TOTAL_SERVOS; i++) {
      float interp = startAngles[i] + (targetArray[i] - startAngles[i]) * ((float)step / steps);
      servoAngles[i] = round(interp);
      pca.setPWM(i, 0, angleToPulse(servoAngles[i]));
    }
    delay(stepDelay);
  }
  return true;
}

// Standard smooth transition for presets
void smoothTransition(int targetArray[]) {
  int steps = 40;
  int stepDelay = 15;
  float startAngles[TOTAL_SERVOS];
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    startAngles[i] = servoAngles[i];
  }
  for (int step = 1; step <= steps; step++) {
    for (int i = 0; i < TOTAL_SERVOS; i++) {
      float interp = startAngles[i] + (targetArray[i] - startAngles[i]) * ((float)step / steps);
      servoAngles[i] = round(interp);
      pca.setPWM(i, 0, angleToPulse(servoAngles[i]));
    }
    delay(stepDelay);
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background: #121212; color: #e0e0e0; text-align: center; margin: 0; padding: 10px; }";
  html += "h1 { font-size: 22px; color: #00bcd4; margin-bottom: 5px; }";
  html += "h3 { font-size: 14px; color: #888; margin-top: 0; }";
  html += ".preset-container { max-width: 400px; margin: 0 auto 15px auto; display: flex; flex-direction: column; gap: 8px; }";
  html += ".master-btn { color: white; border: none; padding: 12px 20px; font-size: 15px; font-weight: bold; border-radius: 8px; cursor: pointer; width: 100%; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }";
  html += ".master-btn:active { opacity: 0.8; }";
  html += ".card { background: #1e1e1e; padding: 12px; margin: 8px auto; border-radius: 10px; max-width: 400px; box-shadow: 0 2px 5px rgba(0,0,0,0.5); text-align: left; }";
  html += ".label { font-size: 14px; font-weight: bold; margin-bottom: 6px; color: #fff; display: flex; justify-content: space-between; }";
  html += ".controls { display: flex; align-items: center; gap: 6px; flex-wrap: wrap; }";
  html += "input[type=range] { flex-grow: 1; accent-color: #00bcd4; height: 8px; }";
  html += "input[type=number] { width: 45px; background: #2c2c2c; color: #fff; border: 1px solid #444; padding: 5px; text-align: center; border-radius: 4px; font-size: 14px; }";
  html += "button.btn-step, button.btn-set { background: #333; color: white; border: 1px solid #555; padding: 5px 8px; border-radius: 4px; cursor: pointer; font-weight: bold; }";
  html += "button.btn-set { background: #00bcd4; border: none; color: #000; }";
  html += "button.btn-step:active { background: #555; }";
  html += "</style></head><body>";
  
  html += "<h1>16-DOF Humanoid Control</h1>";
  html += "<h3>ESP32 Local Access Point Dashboard</h3>";
  
  // Master Preset & Routine Buttons
  html += "<div class='preset-container'>";
  html += "<button class='master-btn' style='background: #ff9800;' onclick='sendPreset(\"/setall\")'>SET ALL TO 90&deg;</button>";
  html += "<button class='master-btn' style='background: #4caf50;' onclick='sendPreset(\"/stand\")'>STAND ROBOT</button>";
  html += "<button class='master-btn' style='background: #2196f3;' onclick='sendPreset(\"/handsfront\")'>HANDS FRONT</button>";
  html += "<button class='master-btn' style='background: #9c27b0;' onclick='sendPreset(\"/handsup\")'>HANDS UP</button>";
  html += "<button class='master-btn' style='background: #e91e63;' onclick='sendPreset(\"/startpushups\")'>START PUSHUPS</button>";
  html += "<button class='master-btn' style='background: #f44336;' onclick='sendPreset(\"/stoppushups\")'>STOP PUSHUPS</button>";
  html += "<button class='master-btn' style='background: #009688;' onclick='sendPreset(\"/startexercise\")'>START EXERCISE</button>";
  html += "<button class='master-btn' style='background: #795548;' onclick='sendPreset(\"/stopexercise\")'>STOP EXERCISE</button>";
  html += "</div>";

  // Generate individual controls for all 16 servos
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    html += "<div class='card'>";
    html += "<div class='label'><span>" + servoNames[i] + "</span><span id='val_" + String(i) + "'>" + String(servoAngles[i]) + "&deg;</span></div>";
    html += "<div class='controls'>";
    
    html += "<button class='btn-step' onclick='stepServo(" + String(i) + ", -1)'>-</button>";
    html += "<input type='range' id='slider_" + String(i) + "' min='0' max='180' value='" + String(servoAngles[i]) + "' oninput='updateVal(" + String(i) + ", this.value)' onchange='sendAngle(" + String(i) + ", this.value)'>";
    html += "<button class='btn-step' onclick='stepServo(" + String(i) + ", 1)'>+</button>";
    html += "<input type='number' id='num_" + String(i) + "' min='0' max='180' value='" + String(servoAngles[i]) + "'>";
    html += "<button class='btn-set' onclick='setDirect(" + String(i) + ")'>Set</button>";
    
    html += "</div></div>";
  }

  // JavaScript
  html += "<script>";
  html += "function updateVal(id, val) { document.getElementById('val_' + id).innerText = val + '°'; document.getElementById('num_' + id).value = val; document.getElementById('slider_' + id).value = val; }";
  html += "function sendAngle(id, val) { fetch('/set?servo=' + id + '&angle=' + val); }";
  html += "function setDirect(id) { var val = document.getElementById('num_' + id).value; if(val>=0 && val<=180){ updateVal(id, val); sendAngle(id, val); } }";
  html += "function stepServo(id, delta) { var slider = document.getElementById('slider_' + id); var newVal = parseInt(slider.value) + delta; if(newVal>=0 && newVal<=180){ updateVal(id, newVal); sendAngle(id, newVal); } }";
  html += "function sendPreset(route) { fetch(route); }";
  html += "</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void stopAllLoops() {
  pushupActive = false;
  exerciseActive = false;
}

void handleSet() {
  stopAllLoops();
  if (server.hasArg("servo") && server.hasArg("angle")) {
    int servoID = server.arg("servo").toInt();
    int angle = server.arg("angle").toInt();
    
    if (servoID >= 0 && servoID < TOTAL_SERVOS && angle >= 0 && angle <= 180) {
      servoAngles[servoID] = angle;
      pca.setPWM(servoID, 0, angleToPulse(angle));
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleSetAll() {
  stopAllLoops();
  smoothTransition(all90Angles);
  server.send(200, "text/plain", "OK");
}

void handleStand() {
  stopAllLoops();
  smoothTransition(standAngles);
  server.send(200, "text/plain", "OK");
}

void handleHandsFront() {
  stopAllLoops();
  smoothTransition(handsFrontAngles);
  server.send(200, "text/plain", "OK");
}

void handleHandsUp() {
  stopAllLoops();
  smoothTransition(handsUpAngles);
  server.send(200, "text/plain", "OK");
}

void handleStartPushups() {
  stopAllLoops();
  pushupActive = true;
  Serial.println("Pushups started.");
  server.send(200, "text/plain", "OK");
}

void handleStopPushups() {
  pushupActive = false;
  Serial.println("Pushups stopped.");
  server.send(200, "text/plain", "OK");
}

void handleStartExercise() {
  stopAllLoops();
  exerciseActive = true;
  Serial.println("Exercise routine started.");
  server.send(200, "text/plain", "OK");
}

void handleStopExercise() {
  exerciseActive = false;
  Serial.println("Exercise routine stopped.");
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // ESP32 I2C pins
  
  pca.begin();
  pca.setPWMFreq(50); // 50Hz for analog servos

  // Initialize pushup frames
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    pushupFrameA[i] = handsFrontAngles[i];
    pushupFrameB[i] = handsFrontAngles[i];
  }
  pushupFrameA[12] = 145;
  pushupFrameA[15] = 17;
  pushupFrameB[12] = 56;
  pushupFrameB[15] = 103;

  // Initialize with Stand preset on startup
  smoothTransition(standAngles);

  // Set up Wi-Fi Access Point
  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.print("Access Point Started! IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Define Web Server Routes
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/setall", handleSetAll);
  server.on("/stand", handleStand);
  server.on("/handsfront", handleHandsFront);
  server.on("/handsup", handleHandsUp);
  server.on("/startpushups", handleStartPushups);
  server.on("/stoppushups", handleStopPushups);
  server.on("/startexercise", handleStartExercise);
  server.on("/stopexercise", handleStopExercise);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient(); // Handle incoming web client requests

  // Continuous pushup loop
  if (pushupActive) {
    if (smoothTransitionInterruptible(pushupFrameA)) {
      delay(400);
      if (pushupActive && smoothTransitionInterruptible(pushupFrameB)) {
        delay(400);
      }
    }
  }

  // Continuous exercise routine loop (Stand -> Hands Front -> Hands Up)
  if (exerciseActive) {
    // 1. Stand Position
    int steps = 40;
    int stepDelay = 15;
    
    // Smooth transition to standAngles with abort check
    bool completed = true;
    float startAngles[TOTAL_SERVOS];
    for(int i=0; i<TOTAL_SERVOS; i++) startAngles[i] = servoAngles[i];
    for(int step=1; step<=steps; step++) {
      server.handleClient();
      if(!exerciseActive) { completed = false; break; }
      for(int i=0; i<TOTAL_SERVOS; i++) {
        servoAngles[i] = round(startAngles[i] + (standAngles[i] - startAngles[i]) * ((float)step/steps));
        pca.setPWM(i, 0, angleToPulse(servoAngles[i]));
      }
      delay(stepDelay);
    }
    
    if (completed) {
      delay(500); // Pause at Stand
      
      // 2. Hands Front Position
      for(int i=0; i<TOTAL_SERVOS; i++) startAngles[i] = servoAngles[i];
      for(int step=1; step<=steps; step++) {
        server.handleClient();
        if(!exerciseActive) { completed = false; break; }
        for(int i=0; i<TOTAL_SERVOS; i++) {
          servoAngles[i] = round(startAngles[i] + (handsFrontAngles[i] - startAngles[i]) * ((float)step/steps));
          pca.setPWM(i, 0, angleToPulse(servoAngles[i]));
        }
        delay(stepDelay);
      }
    }
    
    if (completed) {
      delay(500); // Pause at Hands Front
      
      // 3. Hands Up Position
      for(int i=0; i<TOTAL_SERVOS; i++) startAngles[i] = servoAngles[i];
      for(int step=1; step<=steps; step++) {
        server.handleClient();
        if(!exerciseActive) { completed = false; break; }
        for(int i=0; i<TOTAL_SERVOS; i++) {
          servoAngles[i] = round(startAngles[i] + (handsUpAngles[i] - startAngles[i]) * ((float)step/steps));
          pca.setPWM(i, 0, angleToPulse(servoAngles[i]));
        }
        delay(stepDelay);
      }
    }
    
    if (completed) {
      delay(500); // Pause at Hands Up before repeating loop
    }
  }
}
