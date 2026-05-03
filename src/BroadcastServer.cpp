#include "BroadcastServer.h"

BroadcastServer::BroadcastServer() : server(80), isRunning(false) {
    bcastMutex = xSemaphoreCreateMutex();
    currentWarnZone = {0,0,0,0};
    currentDeadZone = {0,0,0,0};
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

void BroadcastServer::updateZones(RadialZone warn, RadialZone dead) {
    if (xSemaphoreTake(bcastMutex, pdMS_TO_TICKS(10))) {
        currentWarnZone = warn;
        currentDeadZone = dead;
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
        body { font-family: monospace; background-color: #1a1c20; color: #e2e2e7; margin: 0; display: flex; flex-direction: column; align-items: center; min-height: 100vh; padding: 20px; box-sizing: border-box; }
        .header { display: flex; align-items: center; gap: 8px; margin-bottom: 24px; width: 100%; max-width: 400px; justify-content: center; }
        h1 { color: #00dbe9; text-align: center; margin: 0; font-size: 24px; text-transform: uppercase; letter-spacing: 2px; }
        .radar-wrapper { position: relative; width: 100%; max-width: 350px; aspect-ratio: 1; margin-bottom: 24px; }
        #radar-container {
            position: absolute; inset: 0;
            background-color: #1a1c20;
            border: 2px solid #00dbe9;
            overflow: hidden;
        }
        /* Grid lines */
        .grid-y { position: absolute; left: 50%; top: 0; bottom: 0; border-left: 1px dashed rgba(0, 219, 233, 0.2); }
        .grid-x { position: absolute; top: 50%; left: 0; right: 0; border-top: 1px dashed rgba(0, 219, 233, 0.2); }
        .grid-circle-1 { position: absolute; inset: 5%; border: 1px solid rgba(0, 219, 233, 0.1); border-radius: 50%; pointer-events: none; }
        .grid-circle-2 { position: absolute; inset: 25%; border: 1px solid rgba(0, 219, 233, 0.15); border-radius: 50%; pointer-events: none; }

        .dot {
            position: absolute;
            width: 12px; height: 12px;
            border-radius: 50%;
            transform: translate(-50%, -50%);
            transition: left 0.1s linear, top 0.1s linear;
            box-shadow: 0 0 8px currentColor;
        }
        #dot-0 { background-color: #ff3e3e; color: #ff3e3e; }
        #dot-1 { background-color: #00dbe9; color: #00dbe9; }
        #dot-2 { background-color: #facc15; color: #facc15; }

        .info-panel { width: 100%; max-width: 400px; display: flex; flex-direction: column; gap: 8px; }
        .target { background: #282a2e; padding: 12px 16px; font-size: 14px; position: relative; border: 1px solid #3f444d; }
        .target-header { display: flex; justify-content: space-between; margin-bottom: 8px; font-weight: bold; }
        .target-data { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
        .data-label { font-size: 10px; color: #849395; text-transform: uppercase; }
        .data-value { color: #e2e2e7; }
        .data-value-highlight { color: #00dbe9; }

        .active-0 { border-left: 4px solid #ff3e3e; }
        .active-1 { border-left: 4px solid #00dbe9; }
        .active-2 { border-left: 4px solid #facc15; }
        .inactive { border-left: 4px solid #3f444d; opacity: 0.5; }
        .inactive-text { display: flex; align-items: center; justify-content: center; padding: 16px 0; color: #849395; font-size: 12px; text-transform: uppercase; }

        .axis-label { position: absolute; font-size: 10px; color: #849395; }
        .axis-label.top { top: 4px; left: 50%; transform: translateX(-50%); }
        .axis-label.bottom { bottom: 4px; left: 50%; transform: translateX(-50%); }
        .axis-label.left { left: 4px; top: 50%; transform: translateY(-50%); }
        .axis-label.right { right: 4px; top: 50%; transform: translateY(-50%); }
    </style>
</head>
<body>
    <div class="header">
        <h1>RADAR_TRACKER</h1>
    </div>

    <div class="radar-wrapper">
        <div id="radar-container">
            <div class="grid-circle-1"></div>
            <div class="grid-circle-2"></div>
            <div class="grid-y"></div>
            <div class="grid-x"></div>
            <div class="axis-label top">6m</div>
            <div class="axis-label bottom">0m</div>
            <div class="axis-label left">-3m</div>
            <div class="axis-label right">3m</div>
            <div id="dot-0" class="dot" style="display:none;"></div>
            <div id="dot-1" class="dot" style="display:none;"></div>
            <div id="dot-2" class="dot" style="display:none;"></div>
        </div>
    </div>

    <div class="info-panel" id="data">Loading feed...</div>

    <script>
        const MAX_RANGE = 6000;
        const COLORS = ['#ff3e3e', '#00dbe9', '#facc15'];
        const IDS = ['TRK_01', 'TRK_02', 'TRK_03'];

        function drawZone(svg, zone, color, opacity) {
            if (zone.minDist === 0 && zone.maxDist === 0) return;
            const cx = 150;
            const cy = 300;
            const r1 = (zone.minDist / MAX_RANGE) * 300;
            const r2 = (zone.maxDist / MAX_RANGE) * 300;
            const a1 = (zone.minAngle - 90) * Math.PI / 180;
            const a2 = (zone.maxAngle - 90) * Math.PI / 180;

            const x11 = cx + r1 * Math.cos(a1);
            const y11 = cy + r1 * Math.sin(a1);
            const x12 = cx + r2 * Math.cos(a1);
            const y12 = cy + r2 * Math.sin(a1);

            const x21 = cx + r1 * Math.cos(a2);
            const y21 = cy + r1 * Math.sin(a2);
            const x22 = cx + r2 * Math.cos(a2);
            const y22 = cy + r2 * Math.sin(a2);

            const largeArcFlag = (zone.maxAngle - zone.minAngle) > 180 ? 1 : 0;

            const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
            let d = `M ${x11} ${y11} L ${x12} ${y12} A ${r2} ${r2} 0 ${largeArcFlag} 1 ${x22} ${y22} L ${x21} ${y21} A ${r1} ${r1} 0 ${largeArcFlag} 0 ${x11} ${y11} Z`;
            if (r1 === 0) {
                 d = `M ${cx} ${cy} L ${x12} ${y12} A ${r2} ${r2} 0 ${largeArcFlag} 1 ${x22} ${y22} Z`;
            }

            path.setAttribute("d", d);
            path.setAttribute("fill", color);
            path.setAttribute("fill-opacity", opacity);
            path.setAttribute("stroke", color);
            path.setAttribute("stroke-width", "1");
            svg.appendChild(path);
        }

        function updateData() {
            fetch('/api/data')
                .then(response => response.json())
                .then(data => {
                    let svg = document.getElementById('zone-svg');
                    if (!svg) {
                        svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
                        svg.setAttribute("id", "zone-svg");
                        svg.setAttribute("viewBox", "0 0 300 300");
                        svg.style.position = "absolute";
                        svg.style.inset = "0";
                        svg.style.width = "100%";
                        svg.style.height = "100%";
                        svg.style.pointerEvents = "none";
                        svg.style.zIndex = "1";
                        document.getElementById('radar-container').prepend(svg);
                    }
                    svg.innerHTML = '';

                    if (data.warnZone) drawZone(svg, data.warnZone, '#ff3e3e', '0.2');
                    if (data.deadZone) drawZone(svg, data.deadZone, '#3f444d', '0.5');

                    let html = '';
                    for (let i = 0; i < data.targets.length; i++) {
                        let t = data.targets[i];
                        let dot = document.getElementById('dot-' + i);
                        dot.style.zIndex = "10";

                        if (t.active) {
                            let xPct = 50 + ((t.x / (MAX_RANGE/2)) * 50);
                            let yPct = 100 - ((t.y / MAX_RANGE) * 100);

                            xPct = Math.max(0, Math.min(100, xPct));
                            yPct = Math.max(0, Math.min(100, yPct));

                            dot.style.left = xPct + '%';
                            dot.style.top = yPct + '%';
                            dot.style.display = 'block';

                            html += `<div class="target active-${i}">
                                <div class="target-header">
                                    <span style="color: ${COLORS[i]}">[ACT] ${IDS[i]}</span>
                                </div>
                                <div class="target-data">
                                    <div><div class="data-label">Position_X</div><div class="data-value">${t.x}mm</div></div>
                                    <div><div class="data-label">Position_Y</div><div class="data-value">${t.y}mm</div></div>
                                    <div><div class="data-label">Speed</div><div class="data-value-highlight">${t.speed}cm/s</div></div>
                                    <div><div class="data-label">Resolution</div><div class="data-value">${t.resolution}mm</div></div>
                                </div>
                            </div>`;
                        } else {
                            dot.style.display = 'none';
                            html += `<div class="target inactive">
                                <div class="target-header">
                                    <span style="color: #849395">[INA] ${IDS[i]}</span>
                                </div>
                                <div class="inactive-text">NO_SIGNAL</div>
                            </div>`;
                        }
                    }
                    document.getElementById('data').innerHTML = html;
                })
                .catch(err => console.error(err));
        }
        setInterval(updateData, 200);
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
            JsonObject wObj = doc["warnZone"].to<JsonObject>();
            wObj["minDist"] = currentWarnZone.minDist;
            wObj["maxDist"] = currentWarnZone.maxDist;
            wObj["minAngle"] = currentWarnZone.minAngle;
            wObj["maxAngle"] = currentWarnZone.maxAngle;

            JsonObject dObj = doc["deadZone"].to<JsonObject>();
            dObj["minDist"] = currentDeadZone.minDist;
            dObj["maxDist"] = currentDeadZone.maxDist;
            dObj["minAngle"] = currentDeadZone.minAngle;
            dObj["maxAngle"] = currentDeadZone.maxAngle;

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
