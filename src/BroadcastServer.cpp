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

    String wifiPass = getWiFiPassword();
    if (wifiPass == "") {
        wifiPass = generateWiFiPassword();
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
<title>E54 RADAR TRACKER - WEB DASHBOARD</title>
<style>
        body { margin: 0; background: #0a0c10; color: #00dbe9; font-family: 'Courier New', monospace; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; overflow: hidden; }
        .hud-title { font-size: 16px; font-weight: bold; letter-spacing: 2px; margin-bottom: 12px; text-shadow: 0 0 10px #00dbe988; }
        .radar-box { position: relative; width: 320px; height: 320px; border: 2px solid #00dbe944; border-radius: 50%; padding: 4px; box-shadow: 0 0 30px #00dbe922, inset 0 0 30px #00dbe911; background: radial-gradient(circle, #0b1c24 0%, #05080c 100%); }
        svg { width: 100%; height: 100%; }
        .grid { stroke: #00dbe9; stroke-width: 0.5; stroke-opacity: 0.35; fill: none; }
        .heavy-grid { stroke: #00dbe9; stroke-width: 1.0; stroke-opacity: 0.6; fill: none; }
        .readouts { margin-top: 20px; display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px; width: 320px; }
        .target { border: 1px solid #1e3a45; padding: 10px; font-size: 11px; letter-spacing: 1px; text-align: center; background: #0e1620; border-radius: 4px; transition: border-color 0.3s; }
        .target span { display: block; font-weight: bold; color: #ffffff; margin-top: 4px; font-size: 12px; }
        @keyframes rotate { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }
        .blip { fill: #00ff88; filter: drop-shadow(0 0 6px #00ff88); }
        .scale-info { margin-top: 10px; font-size: 11px; color: #00dbe9aa; }
        .status-badge { font-size: 12px; margin-left: 10px; padding: 2px 6px; border-radius: 4px; background: #1e3a45; color: #00dbe9; }
        .status-live { background: #00ff8822; color: #00ff88; border: 1px solid #00ff88; }
        .status-offline { background: #ff004422; color: #ff0044; border: 1px solid #ff0044; }
    </style>
</head>
<body>
<div class="hud-title">E54 RADAR TRACKER HUD<span id="conn-status" class="status-badge status-offline">● OFFLINE</span></div>
<div class="radar-box">
<svg viewBox="0 0 100 100" role="img" aria-label="Radar Sweep Display">
<!-- Polar Distance Arcs -->
<circle class="heavy-grid" cx="50" cy="100" r="90"></circle>
<circle class="grid" cx="50" cy="100" r="67.5"></circle>
<circle class="heavy-grid" cx="50" cy="100" r="45"></circle>
<circle class="grid" cx="50" cy="100" r="22.5"></circle>
<!-- Radial Spoke Rays -->
<line class="heavy-grid" x1="50" x2="50" y1="100" y2="10"></line>
<line class="grid" x1="50" x2="5" y1="100" y2="55"></line>
<line class="grid" x1="50" x2="95" y1="100" y2="55"></line>
<!-- Rotating Radar Sweep Beam -->
<g style="transform-origin: 50px 100px; animation: rotate 4s linear infinite;">
<path d="M 50 100 L 50 10 A 90 90 0 0 1 95 55 Z" fill="rgba(0, 219, 233, 0.15)"></path>
<line stroke="#00dbe9" stroke-width="1" x1="50" x2="50" y1="100" y2="10"></line>
</g>
<!-- Target Blips -->
<circle class="blip" id="blip-0" cx="50" cy="100" r="2.5" style="display:none;"></circle>
<circle class="blip" id="blip-1" cx="50" cy="100" r="2.5" style="display:none;"></circle>
<circle class="blip" id="blip-2" cx="50" cy="100" r="2.5" style="display:none;"></circle>
</svg>
</div>
<div class="scale-info" id="scale-label">RANGE SCALE: 10m (2m/div)</div>
<div class="readouts">
<div class="target" id="t0-card">T-01<span id="t0-val">OFFLINE</span></div>
<div class="target" id="t1-card">T-02<span id="t1-val">OFFLINE</span></div>
<div class="target" id="t2-card">T-03<span id="t2-val">OFFLINE</span></div>
</div>
<script>
        function updateData() {
            fetch('/api/data')
                .then(r => r.json())
                .then(data => {
                    let targets = data.targets || [];
                    let maxDist = 10000; // Minimum 10m scale

                    for (let i = 0; i < 3; i++) {
                        if (targets[i] && targets[i].active) {
                            let d = Math.sqrt(targets[i].x * targets[i].x + targets[i].y * targets[i].y);
                            if (d > maxDist) maxDist = Math.ceil(d / 2000) * 2000;
                        }
                    }

                    document.getElementById('conn-status').innerText = '● LIVE';
                    document.getElementById('conn-status').className = 'status-badge status-live';
                    document.getElementById('scale-label').innerText = `RANGE SCALE: ${maxDist/1000}m (${maxDist/5000}m/div)`;

                    for (let i = 0; i < 3; i++) {
                        let t = targets[i] || {active: false};
                        let blip = document.getElementById(`blip-${i}`);
                        let val = document.getElementById(`t${i}-val`);
                        let card = document.getElementById(`t${i}-card`);

                        if (t.active) {
                            let svgX = 50 + (t.x / maxDist) * 90;
                            let svgY = 100 - (t.y / maxDist) * 90;
                            svgX = Math.max(5, Math.min(95, svgX));
                            svgY = Math.max(5, Math.min(95, svgY));

                            blip.setAttribute('cx', svgX);
                            blip.setAttribute('cy', svgY);
                            blip.style.display = 'block';

                            val.innerText = `X:${t.x} Y:${t.y}\n${t.speed}cm/s`;
                            card.style.borderColor = '#00ff88';
                        } else {
                            blip.style.display = 'none';
                            val.innerText = 'OFFLINE';
                            card.style.borderColor = '#1e3a45';
                        }
                    }
                })
                .catch(err => {
                    console.error(err);
                    document.getElementById('conn-status').innerText = '● OFFLINE';
                    document.getElementById('conn-status').className = 'status-badge status-offline';
                });
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
            AsyncWebServerResponse *response503 = request->beginResponse(503, "application/json", "{\"error\":\"Server busy\"}");
            response503->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response503);
            return;
        }

        String response;
        serializeJson(doc, response);
        AsyncWebServerResponse *wsResponse = request->beginResponse(200, "application/json", response);
        wsResponse->addHeader("Access-Control-Allow-Origin", "*");
        request->send(wsResponse);
    });
}

String BroadcastServer::getWiFiPassword() {
    Preferences prefs;
    prefs.begin("radar_sys", true);
    String wifiPass = prefs.getString("wifi_pass", "Not Set");
    if (wifiPass == "Not Set") wifiPass = "";
    prefs.end();
    return wifiPass;
}

String BroadcastServer::generateWiFiPassword() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    String newPass = "";
    for (int i = 0; i < 12; i++) {
        newPass += charset[random(0, sizeof(charset) - 1)];
    }
    Preferences prefs;
    prefs.begin("radar_sys", false);
    prefs.putString("wifi_pass", newPass);
    prefs.end();
    return newPass;
}
