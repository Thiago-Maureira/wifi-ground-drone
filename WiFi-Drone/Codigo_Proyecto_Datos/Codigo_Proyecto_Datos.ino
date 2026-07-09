#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <TinyGPS++.h>

// WiFi Configuration
#define ssid "YOUR_WIFI_SSID"
#define password "YOUR_WIFI_PASSWORD"

// GPS Configuration
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

// Motor pins
const int motor1Pin1 = 33;
const int motor1Pin2 = 3;
const int motor2Pin1 = 25;
const int motor2Pin2 = 33;
#define transistorPin 13  // LED control

// Create servers
WebSocketsServer webSocket = WebSocketsServer(81);
WebServer server(80);

// GPS objects
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// GPS variables
String latitude = "No data";
String longitude = "No data";
String speedKmph = "No data";
String altitudeMeters = "No data";
String hdop = "No data";
String satellites = "No data";
String fecha = "No data";
String hora = "No data";
bool ledState = false;

// Combined HTML content with both robot control and GPS display
const char* htmlContent = R"(
<p hidden> const char* htmlContent = R"( </p>
<!DOCTYPE HTML>
<html>
<head>
    <meta charset="UTF-8">
    <title>PROYECTO 6°3</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #040918;
            color: #ffffff;
            margin: 0;
            padding: 20px;
            min-height: 100vh;
        }

        .container {
            display: grid;
            grid-template-columns: 300px 1fr 300px;
            gap: 20px;
            height: calc(100vh - 40px);
        }

        .control-panel {
            padding: 20px;
            border-radius: 10px;
            height: fit-content;
        }

        .data {
            padding: 10px;
            height: fit-content;
            text-align: left;
        }

        .data p {
            margin: 10px 0;
        }

        #stream-container {
            display: flex;
            justify-content: center;
            align-items: center;
            overflow: hidden;
            height: 100%;
        }

        iframe {
            width: 100%;
            height: 100%;
            max-width: 100%;
            max-height: 100%;
            aspect-ratio: 16/9;
            border: none;
        }

        #ledStatus {
            margin: 20px 0;
            font-weight: bold;
        }

        h2 {
            margin-top: 0;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Left Column - Controls -->
        <div class="control-panel">
            <h2>CONTROLES DEL DRON</h2>
            <p>Usar W,A,S,D para controlar el robot</p>
            <p>Presionar 'Q' para la iluminación</p>
            <p id="ledStatus">LEDs: Apagados</p>
        </div>

        <!-- Center Column - Stream -->
        <div id="stream-container">
            <iframe src="http://192.168.0.26" allowfullscreen></iframe>
        </div>

        <!-- Right Column - GPS Data -->
        <div class="data">
            <h2>Datos GPS</h2>
            <p><strong>Latitud:</strong> <span id="lat">--</span></p>
            <p><strong>Longitud:</strong> <span id="lon">--</span></p>
            <p><strong>Velocidad (km/h):</strong> <span id="speed">--</span></p>
            <p><strong>Altitud (m):</strong> <span id="alt">--</span></p>
            <p><strong>HDOP:</strong> <span id="hdop">--</span></p>
            <p><strong>Satélites:</strong> <span id="sats">--</span></p>
            <p><strong>Fecha:</strong> <span id="date">--</span></p>        
            <p><strong>Hora:</strong> <span id="time">--</span></p>
        </div>
    </div>

    <script>
        // WebSocket connection for robot control
        var connection = new WebSocket('ws://' + window.location.hostname + ':81/');
        
        // Robot control with WASD
        document.addEventListener('keydown', function(event) {
            if(['W','A','S','D'].includes(event.key.toUpperCase())) {
                connection.send(event.key.toUpperCase());
            }
        });
        
        document.addEventListener('keyup', function(event) {
            if(['W','A','S','D'].includes(event.key.toUpperCase())) {
                connection.send('X');
            }
        });

        // LED control with Q
        document.addEventListener('keypress', function(event) {
            if(event.key.toLowerCase() === 'q') {
                fetch('/toggle')
                    .then(response => response.text())
                    .then(state => {
                        document.getElementById('ledStatus').innerHTML = 
                            'LEDs: ' + (state === 'ON' ? 'Encendidos' : 'Apagados');
                    });
            }
        });

        // GPS data update
        function updateGPSData() {
            fetch('/gpsData')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('lat').textContent = data.latitude;
                    document.getElementById('lon').textContent = data.longitude;
                    document.getElementById('speed').textContent = data.speed;
                    document.getElementById('alt').textContent = data.altitude;
                    document.getElementById('hdop').textContent = data.hdop;
                    document.getElementById('sats').textContent = data.satellites;
                    document.getElementById('time').textContent = data.time;
                });
        }

        // Update GPS data every 2 seconds
        setInterval(updateGPSData, 2000);
        updateGPSData();
    </script>
</body>
</html>
)";

// Motor control functions
void setupMotors() {
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
}

void stopMotors() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);
}

void moveForward() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
}

void moveBackward() {
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
}

void turnLeft() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
}

void turnRight() {
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
}

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_TEXT:
      char command = (char)payload[0];
      switch (command) {
        case 'W': moveForward();  break;
        case 'S': moveBackward(); break;
        case 'A': turnLeft(); break;
        case 'D': turnRight(); break;
        case 'X': stopMotors(); break;
      }
      break;
  }
}

// HTTP handlers
void handleGPSData() {
  String jsonResponse = "{";
  jsonResponse += "\"latitude\":\"" + latitude + "\",";
  jsonResponse += "\"longitude\":\"" + longitude + "\",";
  jsonResponse += "\"speed\":\"" + speedKmph + "\",";
  jsonResponse += "\"altitude\":\"" + altitudeMeters + "\",";
  jsonResponse += "\"hdop\":\"" + hdop + "\",";
  jsonResponse += "\"satellites\":\"" + satellites + "\",";
  jsonResponse += "\"fecha\":\"" + fecha + "\"";
  jsonResponse += "\"hora\":\"" + hora + "\"";
  jsonResponse += "}";

  server.send(200, "application/json", jsonResponse);
}

void handleLEDToggle() {
  ledState = !ledState;
  digitalWrite(transistorPin, ledState ? HIGH : LOW);
  server.send(200, "text/plain", ledState ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);
  setupMotors();
  pinMode(transistorPin, OUTPUT);
  
  // Initialize GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("/");
  }
  Serial.println("IP: " + WiFi.localIP().toString());

  // Setup WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Setup HTTP server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlContent);
  });
  server.on("/toggle", HTTP_GET, handleLEDToggle);
  server.on("/gpsData", HTTP_GET, handleGPSData);

  // Start HTTP server
  server.begin();
}

void loop() {
  webSocket.loop();
  server.handleClient();

  // Update GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Update GPS variables if new data is available
  if (gps.location.isUpdated()) {
    latitude = String(gps.location.lat(), 6);
    longitude = String(gps.location.lng(), 6);
    speedKmph = String(gps.speed.kmph());
    altitudeMeters = String(gps.altitude.meters());
    hdop = String(gps.hdop.value() / 100.0);
    satellites = String(gps.satellites.value());
    fecha = String(gps.date.year()) + " / " + String(gps.date.month()) + " / " + String(gps.date.day());
    hora = String(gps.time.hour() - 3) + ":" + String(gps.time.minute());
  }
}