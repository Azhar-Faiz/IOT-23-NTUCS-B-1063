#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <math.h> 

// --- CONFIGURATION ---
const char* ssid = "Smart_Doorbell_Hub";
const char* password = "password123";

// HARDWARE PINS (Mic + Speaker Only)
#define MIC_PIN 34     
#define SPEAKER_PIN 25 

WebServer server(80);
WebSocketsServer wsControl = WebSocketsServer(81); 
WebSocketsServer wsMicStream = WebSocketsServer(82); 

// --- STATE VARIABLES ---
bool isRinging = false;
unsigned long ringTime = 0;
const unsigned long RING_TIMEOUT = 120000; // 2 Minutes memory

// --- CHIME ---
void playTone(int freq, int durationMs) {
  for (long i = 0; i < durationMs * 4; i++) { 
     dacWrite(SPEAKER_PIN, 128 + 100); 
     delayMicroseconds(1000000 / freq / 2);
     dacWrite(SPEAKER_PIN, 128 - 100);
     delayMicroseconds(1000000 / freq / 2);
  }
  dacWrite(SPEAKER_PIN, 0); 
}

void playDingDong() {
  playTone(800, 150); delay(50);
  playTone(600, 300);
}

// --- MODERN UI WEBPAGE (FIXED ENCODING) ---
const char HTML_CONTENT[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Doorbell Hub</title>
    <style>
        :root {
            --bg-color: #121212;
            --card-bg: #1e1e1e;
            --primary: #3071d1;
            --danger: #d13030;
            --success: #28a745;
            --text: #ffffff;
            --text-muted: #aaaaaa;
        }
        body {
            font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text);
            margin: 0;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
        }
        h2 { margin-top: 0; letter-spacing: 1px; font-weight: 300; }
        
        .card {
            background: var(--card-bg);
            border-radius: 20px;
            padding: 25px;
            width: 100%;
            max-width: 400px;
            box-shadow: 0 10px 20px rgba(0,0,0,0.5);
            display: flex;
            flex-direction: column;
            gap: 15px;
        }

        .video-container {
            width: 100%;
            height: 240px;
            background: #000;
            border-radius: 12px;
            overflow: hidden;
            display: flex;
            align-items: center;
            justify-content: center;
            border: 2px solid #333;
            position: relative;
        }
        
        /* The image is hidden by default */
        .vid { 
            width: 100%; 
            height: 100%; 
            object-fit: contain; 
            display: none; 
            z-index: 2; /* Ensure it sits on top */
        }
        
        .placeholder { color: #555; font-size: 14px; z-index: 1; }

        #status-badge {
            background: #333;
            color: #ccc;
            padding: 5px 12px;
            border-radius: 15px;
            font-size: 12px;
            align-self: center;
            text-transform: uppercase;
            font-weight: bold;
            letter-spacing: 0.5px;
            transition: 0.3s;
        }

        .controls { display: flex; flex-direction: column; gap: 12px; }

        button {
            border: none;
            padding: 18px;
            border-radius: 12px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.1s, opacity 0.2s;
            color: white;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            width: 100%;
        }
        button:active { transform: scale(0.98); }
        
        .btn-cam { background: linear-gradient(135deg, #444, #222); box-shadow: 0 4px 6px rgba(0,0,0,0.2); }
        .btn-talk { background: linear-gradient(135deg, #555, #333); }
        .btn-talk:active { background: var(--danger); }
        .btn-listen { background: linear-gradient(135deg, #3071d1, #2050a0); }
        .btn-listen.active { background: linear-gradient(135deg, #d13030, #a02020); }

    </style>
</head>
<body>

    <div class="card">
        <h2>DOORBELL HUB</h2>
        <div id="status-badge">System Ready</div>

        <div class="video-container">
            <div class="placeholder" id="vidPlaceholder">No Signal</div>
            <img id="cam" class="vid" src="" alt="Stream">
        </div>

        <div class="controls">
            <button class="btn-cam" id="btnCam" onclick="toggleCamera()">&#128247; View Cam</button>
            <button class="btn-listen" id="btnListen">&#127911; Start Listening</button>
            <button class="btn-talk" id="btnTalk">&#127897; Hold to Talk</button>
        </div>
    </div>

    <script>
        let audioCtx = new (window.AudioContext || window.webkitAudioContext)({sampleRate: 8000});
        let wsControl = new WebSocket('ws://' + window.location.hostname + ':81');

        // --- 1. WEBSOCKET HANDLER ---
        wsControl.onmessage = (e) => {
            if (e.data === "RING") {
                triggerRingState();
            }
        };

        function updateStatus(msg, color) {
            const badge = document.getElementById('status-badge');
            badge.innerText = msg;
            badge.style.background = color;
            badge.style.color = "white";
        }

        function triggerRingState() {
            updateStatus("Doorbell Ringing!", "#d13030");
            // Force start camera when ring occurs
            if(!camActive) {
                startCamera();
            }
        }

        // --- 2. CAMERA LOGIC ---
        let camActive = false;
        
        function toggleCamera() {
            if(!camActive) startCamera();
            else stopCamera();
        }

        function startCamera() {
            let img = document.getElementById('cam');
            let ph = document.getElementById('vidPlaceholder');
            let btn = document.getElementById('btnCam');

            // Force display updates
            img.style.display = "block";
            ph.style.display = "none";
            
            // Add Timestamp to bust cache
            img.src = "http://192.168.4.2:81/stream?t=" + new Date().getTime(); 
            
            // Update Button
            btn.innerHTML = "&#128683; Stop Cam"; 
            camActive = true;
        }

        function stopCamera() {
            let img = document.getElementById('cam');
            let ph = document.getElementById('vidPlaceholder');
            let btn = document.getElementById('btnCam');

            img.style.display = "none";
            ph.style.display = "block";
            img.src = ""; // Stop network request
            
            btn.innerHTML = "&#128247; View Cam";
            camActive = false;
        }

        // --- 3. LISTEN LOGIC ---
        let listenSocket = null;
        const btnListen = document.getElementById('btnListen');

        btnListen.onclick = () => {
            if (listenSocket) {
                listenSocket.close();
                listenSocket = null;
                btnListen.innerHTML = "&#127911; Start Listening";
                btnListen.classList.remove("active");
                updateStatus("System Ready", "#333");
            } else {
                updateStatus("Listening...", "#3071d1");
                listenSocket = new WebSocket('ws://' + window.location.hostname + ':82');
                listenSocket.binaryType = 'arraybuffer';
                
                listenSocket.onmessage = (e) => {
                    let raw = new Uint8Array(e.data);
                    let buffer = audioCtx.createBuffer(1, raw.length, 8000);
                    let data = buffer.getChannelData(0);
                    for(let i=0; i<raw.length; i++) data[i] = (raw[i] - 128) / 128;
                    
                    let src = audioCtx.createBufferSource();
                    src.buffer = buffer; 
                    src.connect(audioCtx.destination); 
                    src.start();
                };
                
                btnListen.innerHTML = "&#128721; Stop Listening";
                btnListen.classList.add("active");
            }
        };

        // --- 4. TALK LOGIC ---
        let talkSocket;
        const btnTalk = document.getElementById('btnTalk');
        btnTalk.onpointerdown = async () => {
            updateStatus("Broadcasting...", "#28a745");
            const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
            const talkCtx = new AudioContext({ sampleRate: 16000 });
            const source = talkCtx.createMediaStreamSource(stream);
            const processor = talkCtx.createScriptProcessor(512, 1, 1);
            
            talkSocket = new WebSocket('ws://' + window.location.hostname + ':81'); 
            talkSocket.binaryType = 'arraybuffer';
            talkSocket.onopen = () => {
                source.connect(processor);
                processor.connect(talkCtx.destination);
                processor.onaudioprocess = (e) => {
                    let input = e.inputBuffer.getChannelData(0);
                    let output = new Uint8Array(input.length);
                    for (let i = 0; i < input.length; i++) output[i] = Math.floor((input[i] + 1) * 127);
                    if (talkSocket.readyState === 1) talkSocket.send(output.buffer);
                };
            };
        };
        btnTalk.onpointerup = () => { 
            if(talkSocket) talkSocket.close(); 
            updateStatus("System Ready", "#333");
        };
    </script>
</body>
</html>
)=====";

// --- CONTROL EVENT HANDLER ---
void onControlEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if(type == WStype_CONNECTED) {
        if(isRinging && (millis() - ringTime < RING_TIMEOUT)) {
             wsControl.sendTXT(num, "RING");
        }
    }
    else if(type == WStype_BIN) {
        for(int i = 0; i < length; i++) {
            dacWrite(SPEAKER_PIN, payload[i]);
            delayMicroseconds(50); 
        }
    }
}

void setup() {
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
  
  WiFi.softAP(ssid, password);
  
  // FIXED: ADDED CHARSET TO HEADER
  server.on("/", [](){ server.send(200, "text/html; charset=utf-8", HTML_CONTENT); });
  
  server.on("/ring", [](){
    server.send(200, "text/plain", "OK");
    isRinging = true;
    ringTime = millis();
    wsControl.broadcastTXT("RING");
    playDingDong();
  });
  
  server.begin();
  wsControl.begin();
  wsControl.onEvent(onControlEvent);
  wsMicStream.begin();
}

void loop() {
  server.handleClient();
  wsControl.loop();
  wsMicStream.loop();

  if (isRinging && millis() - ringTime > RING_TIMEOUT) {
      isRinging = false;
  }

  // MIC LOGIC
  if(wsMicStream.connectedClients() > 0) {
      const int bufferSize = 256; 
      uint8_t audioBuf[bufferSize];
      for (int i = 0; i < bufferSize; i++) {
          int val = analogRead(MIC_PIN);
          audioBuf[i] = val >> 4; 
          delayMicroseconds(120); 
      }
      wsMicStream.broadcastBIN(audioBuf, bufferSize);
  }
}