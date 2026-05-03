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
        body { font-family: monospace; background-color: #121315; color: #fff; margin: 20px; }
        .target { background: #333; padding: 10px; margin: 10px 0; border-radius: 5px; }
        .active { border-left: 5px solid #00dbe9; }
        .inactive { border-left: 5px solid #555; color: #888; }
        h1 { color: #00dbe9; }
    </style>
</head>
<body>
    <h1>ESP32 Radar Tracker Data</h1>
    <div id="data">Loading...</div>
    <script>
        function updateData() {
            fetch('/api/data')
                .then(response => response.json())
                .then(data => {
                    let html = '';
                    for (let i = 0; i < data.targets.length; i++) {
                        let t = data.targets[i];
                        if (t.active) {
                            html += `<div class="target active">`;
                            html += `<b>Target ${i+1}</b><br>`;
                            html += `X: ${t.x} mm, Y: ${t.y} mm<br>`;
                            html += `Speed: ${t.speed} cm/s<br>`;
                            html += `Res: ${t.resolution} mm`;
                            html += `</div>`;
                        } else {
                            html += `<div class="target inactive"><b>Target ${i+1}</b> - INACTIVE</div>`;
                        }
                    }
                    document.getElementById('data').innerHTML = html;
                })
                .catch(err => console.error(err));
        }
        setInterval(updateData, 500); // 2Hz updates
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
