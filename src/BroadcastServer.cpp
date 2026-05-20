#include "BroadcastServer.h"

BroadcastServer::BroadcastServer() : server(80), isRunning(false) {
    bcastMutex = xSemaphoreCreateMutex();
    for (int i = 0; i < 3; i++) {
        currentTargets[i].active = false;
        currentTargets[i].x = 0;
        currentTargets[i].y = 0;
        currentTargets[i].speed = 0;
        currentTargets[i].resolution = 0;
    }
}

void BroadcastServer::begin() {
    if (isRunning) return;


    Preferences prefs;
    prefs.begin("radar_sys", true);
    String wifiPass = prefs.getString("wifi_pass", "");
    prefs.end();

    if (wifiPass == "") {
        const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        wifiPass = "";
        for (int i = 0; i < 12; i++) {
            wifiPass += charset[random(0, sizeof(charset) - 1)];
        }
        prefs.begin("radar_sys", false);
        prefs.putString("wifi_pass", wifiPass);
        prefs.end();
    }

    WiFi.softAP("ESP32-Radar-Tracker", wifiPass.c_str()); // Secured AP

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    setupRoutes();
    server.begin();
    isRunning = true;
}

void BroadcastServer::stop() {
    if (!isRunning) return;
    server.end();
    WiFi.softAPdisconnect(true);
    isRunning = false;
}

void BroadcastServer::updateData(const RadarTarget targets[3]) {
    if (xSemaphoreTake(bcastMutex, pdMS_TO_TICKS(10))) {
        for (int i = 0; i < 3; i++) {
            currentTargets[i] = targets[i];
        }
        xSemaphoreGive(bcastMutex);
    }
}

void BroadcastServer::setupRoutes() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>

<html class="dark" lang="en"><head>
<meta charset="utf-8"/>
<meta content="width=device-width, initial-scale=1.0" name="viewport"/>
<title>SYS_RADAR_01 - ESP32 INTERFACE</title>
<style>
        body { margin: 0; background: #121315; color: #00dbe9; font-family: monospace; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; overflow: hidden; }
        .radar-box { position: relative; width: 300px; height: 300px; border: 1px solid #3b494c; border-radius: 50%; padding: 10px; box-shadow: inset 0 0 20px #00dbe922; }
        svg { width: 100%; height: 100%; transform: rotate(-90deg); }
        .grid { stroke: #00dbe9; stroke-width: 0.5; stroke-opacity: 0.3; fill: none; }
        .sweep { fill: conic-gradient(from 0deg, #00dbe944, transparent); transform-origin: center; animation: rotate 4s linear infinite; }
        .readouts { margin-top: 24px; display: grid; grid-template-columns: repeat(3, 1fr); gap: 16px; width: 300px; }
        .target { border: 1px solid #3b494c; padding: 8px; font-size: 10px; letter-spacing: 1px; text-align: center; background: #1a1c20; }
        .target span { display: block; font-weight: bold; color: #e2e2e8; margin-top: 4px; }
        @keyframes rotate { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }
        .blip { fill: #00dbe9; filter: drop-shadow(0 0 3px #00dbe9); }
    </style>


  </head>
<body>
<div class="radar-box">
<svg viewbox="0 0 100 100">
<!-- Grid Rings -->
<circle class="grid" cx="50" cy="50" r="48"></circle>
<circle class="grid" cx="50" cy="50" r="32"></circle>
<circle class="grid" cx="50" cy="50" r="16"></circle>
<!-- Axis -->
<line class="grid" x1="0" x2="100" y1="50" y2="50"></line>
<line class="grid" x1="50" x2="50" y1="0" y2="100"></line>
<!-- Rotating Sweep -->
<g style="transform-origin: 50px 50px; animation: rotate 4s linear infinite;">
<path d="M 50 50 L 100 50 A 50 50 0 0 0 85.35 14.64 Z" fill="rgba(0, 219, 233, 0.2)"></path>
<line stroke="#00dbe9" stroke-width="1" x1="50" x2="100" y1="50" y2="50"></line>
</g>
<!-- Static Targets (Simulated) -->
<circle class="blip" cx="70" cy="30" r="1.5"></circle>
<circle class="blip" cx="35" cy="45" r="1.5"></circle>
<circle class="blip" cx="60" cy="80" r="1.5"></circle>
</svg>
</div>
<div class="readouts">
<div class="target">
            ID: T-01
            <span id="t1-val">RNG: 42m</span>
</div>
<div class="target">
            ID: T-02
            <span id="t2-val">RNG: 18m</span>
</div>
<div class="target">
            ID: T-03
            <span id="t3-val">RNG: 65m</span>
</div>
</div>
<script>
        const MAX_RANGE = 6000;
        function updateData() {
            fetch('/api/data')
                .then(response => response.json())
                .then(data => {
                    let blips = document.querySelectorAll('svg > circle.blip');
                    let readouts = [
                        document.getElementById('t1-val'),
                        document.getElementById('t2-val'),
                        document.getElementById('t3-val')
                    ];
                    let targetsDivs = document.querySelectorAll('.target');

                    for (let i = 0; i < 3; i++) {
                        let t = data.targets[i] || {active: false};
                        let blip = blips[i];
                        let readout = readouts[i];
                        let div = targetsDivs[i];

                        if (t.active) {
                            let xPct = 50 + ((t.x / (MAX_RANGE/2)) * 50);
                            let yPct = 100 - ((t.y / MAX_RANGE) * 100);
                            xPct = Math.max(0, Math.min(100, xPct));
                            yPct = Math.max(0, Math.min(100, yPct));

                            blip.setAttribute('cx', yPct); // rotated -90deg, swap axes for SVG view
                            blip.setAttribute('cy', xPct);
                            blip.style.display = 'block';

                            readout.innerText = `X:${t.x} Y:${t.y}`;
                            div.style.borderColor = '#00dbe9';
                        } else {
                            blip.style.display = 'none';
                            readout.innerText = 'OFFLINE';
                            div.style.borderColor = '#3b494c';
                        }
                    }
                })
                .catch(err => console.error(err));
        }
        setInterval(updateData, 200);
        updateData();
    </script>
</body></html>
)rawliteral";
        request->send(200, "text/html", html);
    });

    server.on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument doc;
        JsonArray jsonTargets = doc["targets"].to<JsonArray>();

        if (xSemaphoreTake(bcastMutex, pdMS_TO_TICKS(50))) {
            for (int i = 0; i < 3; i++) {
                JsonObject tObj = jsonTargets.add<JsonObject>();
                tObj["active"] = currentTargets[i].active;
                tObj["x"] = currentTargets[i].x;
                tObj["y"] = currentTargets[i].y;
                tObj["speed"] = currentTargets[i].speed;
                tObj["resolution"] = currentTargets[i].resolution;
            }
            xSemaphoreGive(bcastMutex);
        } else {
            request->send(503, "application/json", "{\"error\":\"Server busy\"}");
            return;
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
}
