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

    WiFi.softAP("ESP32-Radar-Tracker", ""); // Open AP
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
<html>
<head>
    <title>ESP32 Radar Tracker</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: monospace; background-color: #121315; color: #fff; margin: 20px; display: flex; flex-direction: column; align-items: center; }
        h1 { color: #00dbe9; text-align: center; }
        #radar-container {
            position: relative;
            width: 300px;
            height: 300px;
            background-color: #1a1c20;
            border: 2px solid #00dbe9;
            border-radius: 5px;
            margin-bottom: 20px;
            overflow: hidden;
        }
        /* Grid lines */
        .grid-y { position: absolute; left: 50%; top: 0; bottom: 0; border-left: 1px dashed #333; }
        .grid-x { position: absolute; top: 50%; left: 0; right: 0; border-top: 1px dashed #333; }

        .dot {
            position: absolute;
            font-size: 20px;
            transform: translate(-50%, -50%);
            transition: left 0.3s linear, top 0.3s linear;
        }

        .info-panel { width: 300px; display: flex; flex-direction: column; gap: 10px; }
        .target { background: #333; padding: 10px; border-radius: 5px; font-size: 12px; }
        .active { border-left: 5px solid #00dbe9; }
        .inactive { border-left: 5px solid #555; color: #888; }
    </style>
</head>
<body>
    <h1>Radar Tracker</h1>

    <div id="radar-container">
        <div class="grid-y"></div>
        <div class="grid-x"></div>
        <div id="dot-0" class="dot" style="display:none;">🔴</div>
        <div id="dot-1" class="dot" style="display:none;">🟢</div>
        <div id="dot-2" class="dot" style="display:none;">🟡</div>
    </div>

    <div class="info-panel" id="data">Loading...</div>

    <script>
        // Sensor max range in mm (roughly 6000mm)
        const MAX_RANGE = 6000;

        function updateData() {
            fetch('/api/data')
                .then(response => response.json())
                .then(data => {
                    let html = '';
                    for (let i = 0; i < data.targets.length; i++) {
                        let t = data.targets[i];
                        let dot = document.getElementById('dot-' + i);

                        if (t.active) {
                            // Map coordinate to percentage (0-100%)
                            // X is horizontal (-3000 to 3000mm mapped to 0-100%, center is 50%)
                            // Y is vertical (0 to 6000mm mapped to 100-0%, sensor is at bottom 100%)
                            let xPct = 50 + ((t.x / (MAX_RANGE/2)) * 50);
                            let yPct = 100 - ((t.y / MAX_RANGE) * 100);

                            // Clamp values to keep inside box
                            xPct = Math.max(0, Math.min(100, xPct));
                            yPct = Math.max(0, Math.min(100, yPct));

                            dot.style.left = xPct + '%';
                            dot.style.top = yPct + '%';
                            dot.style.display = 'block';

                            html += `<div class="target active">`;
                            html += `<b>Target ${i+1}</b><br>`;
                            html += `X: ${t.x} mm, Y: ${t.y} mm<br>`;
                            html += `Speed: ${t.speed} cm/s | Res: ${t.resolution} mm`;
                            html += `</div>`;
                        } else {
                            dot.style.display = 'none';
                            html += `<div class="target inactive"><b>Target ${i+1}</b> - INACTIVE</div>`;
                        }
                    }
                    document.getElementById('data').innerHTML = html;
                })
                .catch(err => console.error(err));
        }
        setInterval(updateData, 200); // 5Hz updates
        updateData();
    </script>
</body>
</html>
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
