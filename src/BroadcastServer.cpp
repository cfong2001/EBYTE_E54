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

<html class="dark" lang="en"><script src="https://cdn.tailwindcss.com"></script>
<head>
<meta charset="utf-8"/>
<meta content="width=device-width, initial-scale=1.0" name="viewport"/>
<script src="https://cdn.tailwindcss.com?plugins=forms,container-queries"></script>
<link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500;700&amp;family=Geist:wght@400;600;700&amp;display=swap" rel="stylesheet"/>
<link href="https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:wght,FILL@100..700,0..1&amp;display=swap" rel="stylesheet"/>
<link href="https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:wght,FILL@100..700,0..1&amp;display=swap" rel="stylesheet"/>
<style>
    .material-symbols-outlined {
      font-variation-settings: 'FILL' 0, 'wght' 400, 'GRAD' 0, 'opsz' 24;
    }
    /* Custom scanline effect for technical feel */
    .scanline {
      width: 100%;
      height: 2px;
      background: rgba(0, 218, 243, 0.1);
      position: absolute;
      top: 0;
      z-index: 10;
      pointer-events: none;
    }
    .radar-sweep {
      transform-origin: center;
      animation: sweep 4s linear infinite;
    }
    @keyframes sweep {
      from { transform: rotate(0deg); }
      to { transform: rotate(360deg); }
    }
    .blip-pulse {
      animation: pulse 2s ease-out infinite;
    }
    @keyframes pulse {
      0% { r: 2; opacity: 1; }
      100% { r: 8; opacity: 0; }
    }
  </style>
<script id="tailwind-config">
    tailwind.config = {
      darkMode: "class",
      theme: {
        extend: {
          "colors": {
            "inverse-surface": "#e2e2e8",
            "on-primary-container": "#00626e",
            "inverse-on-surface": "#2f3035",
            "tertiary-container": "#ffc67a",
            "error": "#ffb4ab",
            "on-background": "#e2e2e8",
            "on-tertiary": "#452b00",
            "on-secondary": "#053900",
            "on-tertiary-container": "#7b5000",
            "outline": "#849396",
            "on-primary-fixed": "#001f24",
            "inverse-primary": "#006875",
            "on-primary": "#00363d",
            "surface-container-lowest": "#0c0e12",
            "surface-container-low": "#1a1c20",
            "secondary-fixed-dim": "#2ae500",
            "on-surface": "#e2e2e8",
            "primary-container": "#00e5ff",
            "on-tertiary-fixed-variant": "#633f00",
            "on-primary-fixed-variant": "#004f58",
            "surface-tint": "#00daf3",
            "on-error": "#690005",
            "on-surface-variant": "#bac9cc",
            "on-secondary-fixed-variant": "#095300",
            "background": "#111318",
            "secondary-container": "#2ff801",
            "error-container": "#93000a",
            "surface-container-high": "#282a2e",
            "secondary-fixed": "#79ff5b",
            "primary-fixed-dim": "#00daf3",
            "outline-variant": "#3b494c",
            "on-tertiary-fixed": "#291800",
            "tertiary-fixed": "#ffddb4",
            "on-secondary-fixed": "#022100",
            "secondary": "#d7ffc5",
            "on-secondary-container": "#0f6d00",
            "surface-container": "#1e2024",
            "primary": "#c3f5ff",
            "surface-bright": "#37393e",
            "surface": "#111318",
            "on-error-container": "#ffdad6",
            "primary-fixed": "#9cf0ff",
            "surface-dim": "#111318",
            "tertiary": "#ffe9d1",
            "tertiary-fixed-dim": "#ffb955",
            "surface-variant": "#333539",
            "surface-container-highest": "#333539"
          },
          "borderRadius": {
            "DEFAULT": "0.25rem",
            "lg": "0.5rem",
            "xl": "0.75rem",
            "full": "9999px"
          },
          "spacing": {
            "gutter": "16px",
            "container-max-width": "1920px",
            "unit": "4px",
            "panel-gap": "8px",
            "margin-edge": "24px"
          },
          "fontFamily": {
            "data-lg": ["JetBrains Mono"],
            "display-lg": ["Geist"],
            "telemetry-sm": ["JetBrains Mono"],
            "data-md": ["JetBrains Mono"],
            "label-caps": ["JetBrains Mono"],
            "headline-md": ["Geist"]
          },
          "fontSize": {
            "data-lg": ["18px", {"lineHeight": "1.4", "letterSpacing": "0em", "fontWeight": "500"}],
            "display-lg": ["48px", {"lineHeight": "1.1", "letterSpacing": "-0.02em", "fontWeight": "700"}],
            "telemetry-sm": ["10px", {"lineHeight": "12px", "letterSpacing": "0.05em", "fontWeight": "400"}],
            "data-md": ["14px", {"lineHeight": "1.4", "letterSpacing": "0em", "fontWeight": "400"}],
            "label-caps": ["11px", {"lineHeight": "12px", "letterSpacing": "0.1em", "fontWeight": "700"}],
            "headline-md": ["24px", {"lineHeight": "1.2", "letterSpacing": "0.01em", "fontWeight": "600"}]
          }
        }
      }
    }
  </script>
<style>
    body {
      min-height: max(884px, 100dvh);
    }
  </style>

<script>
    tailwind.config = {
      theme: {
        extend: {
          colors: {
            'surface-container-low': '#1a1c20',
            'outline-variant': '#3b494c',
            'on-surface-variant': '#bac9cc',
            'primary-fixed-dim': '#00daf3',
            'secondary-fixed-dim': '#2ae500',
            'primary': '#c3f5ff',
            'primary-container': '#00e5ff',
            'on-surface': '#e2e2e8',
            'background': '#111318',
            'surface-container-lowest': '#0c0e12'
          },
          fontFamily: {
            'label-caps': ['JetBrains Mono', 'monospace'],
            'data-lg': ['JetBrains Mono', 'monospace'],
            'data-md': ['JetBrains Mono', 'monospace'],
            'telemetry-sm': ['JetBrains Mono', 'monospace']
          },
          spacing: {
            'gutter': '16px',
            'panel-gap': '8px'
          }
        }
      }
    }
  </script>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500;700&display=swap');

    body {
      background-color: #111318;
      color: #e2e2e8;
    }

    .font-label-caps { font-size: 11px; font-weight: 700; letter-spacing: 0.1em; line-height: 12px; }
    .font-data-lg { font-size: 18px; font-weight: 500; letter-spacing: 0em; line-height: 1.4; }
    .font-data-md { font-size: 14px; font-weight: 400; letter-spacing: 0em; line-height: 1.4; }
    .font-telemetry-sm { font-size: 10px; font-weight: 400; letter-spacing: 0.05em; line-height: 12px; }

    @keyframes pulse-ring {
      0% { transform: scale(0.8); opacity: 0.8; }
      100% { transform: scale(2.5); opacity: 0; }
    }

    .blip-pulse {
      transform-origin: center;
      animation: pulse-ring 2s cubic-bezier(0.215, 0.61, 0.355, 1) infinite;
    }

    @keyframes sweep {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }

    .radar-sweep {
      transform-origin: 100px 100px;
      animation: sweep 4s linear infinite;
    }
  </style>
</head>

<body class="bg-background text-on-surface font-data-md selection:bg-primary-container/30 overflow-hidden">
<!-- TOP APP BAR -->
<header class="bg-background text-primary-fixed-dim font-headline-md text-headline-md docked full-width top-0 border-b border-outline-variant flex justify-between items-center w-full px-gutter h-14 z-50">
<div class="flex items-center gap-2">
<span class="material-symbols-outlined text-primary-fixed-dim" data-icon="sensors">sensors</span>
<span class="font-label-caps text-label-caps tracking-widest text-primary-fixed-dim">SYS_RADAR_01</span>
</div>
<div class="flex items-center gap-4">
<div class="flex items-center gap-1.5 px-2 py-1 rounded bg-surface-container-high border border-outline-variant">
<div class="w-1.5 h-1.5 bg-secondary-fixed-dim rounded-full shadow-[0_0_8px_#2ae500]"></div>
<span class="font-label-caps text-[9px] uppercase tracking-tighter text-on-surface-variant">Live</span>
</div>
<span class="material-symbols-outlined text-on-surface-variant cursor-pointer hover:bg-surface-container-high transition-colors p-1 rounded" data-icon="settings">settings</span>
</div>
</header>
<!-- MAIN CANVAS -->
<main class="relative h-[calc(100vh-120px)] w-full flex flex-col pt-4 overflow-hidden">
<!-- SYSTEM STATUS STRIP -->
<section class="px-gutter mb-4 grid grid-cols-2 gap-panel-gap">
<div class="p-3 bg-surface-container-low border border-outline-variant flex flex-col gap-1">
<span class="font-label-caps text-label-caps text-on-surface-variant">UPTIME</span>
<span class="font-data-lg text-data-lg text-primary-fixed-dim tracking-tight">142:08:44:12</span>
</div>
<div class="p-3 bg-surface-container-low border border-outline-variant flex flex-col gap-1">
<span class="font-label-caps text-label-caps text-on-surface-variant">SIGNAL_STR</span>
<div class="flex items-end gap-1 h-6">
<div class="w-1.5 h-1 bg-secondary-fixed-dim"></div>
<div class="w-1.5 h-2 bg-secondary-fixed-dim"></div>
<div class="w-1.5 h-3 bg-secondary-fixed-dim"></div>
<div class="w-1.5 h-4 bg-secondary-fixed-dim"></div>
<div class="w-1.5 h-5 bg-outline-variant"></div>
<span class="font-data-md text-data-md text-primary ml-2">-42dBm</span>
</div>
</div>
</section>
<!-- RADAR SCANNER COMPONENT -->
<section class="flex-1 flex items-center justify-center px-gutter relative">
<div class="relative w-full max-w-[320px] aspect-square flex items-center justify-center">
<!-- Background Glow -->
<div class="absolute inset-0 bg-primary-fixed-dim/5 rounded-full blur-3xl"></div>
<!-- SVG RADAR -->
<svg class="w-full h-full drop-shadow-[0_0_15px_rgba(0,218,243,0.1)]" viewbox="0 0 200 200">
<!-- Concentric Circles -->
<circle class="text-outline-variant" cx="100" cy="100" fill="none" r="95" stroke="currentColor" stroke-width="0.5"></circle>
<circle class="text-outline-variant" cx="100" cy="100" fill="none" r="75" stroke="currentColor" stroke-width="0.5"></circle>
<circle class="text-outline-variant" cx="100" cy="100" fill="none" r="55" stroke="currentColor" stroke-width="0.5"></circle>
<circle class="text-outline-variant" cx="100" cy="100" fill="none" r="35" stroke="currentColor" stroke-width="0.5"></circle>
<circle class="text-outline-variant" cx="100" cy="100" fill="none" r="15" stroke="currentColor" stroke-width="0.5"></circle>
<!-- Axes -->
<line class="text-outline-variant" stroke="currentColor" stroke-width="0.5" x1="100" x2="100" y1="5" y2="195"></line>
<line class="text-outline-variant" stroke="currentColor" stroke-width="0.5" x1="5" x2="195" y1="100" y2="100"></line>
<!-- Degree Labels -->
<text class="font-label-caps text-[4px] fill-on-surface-variant" text-anchor="middle" x="100" y="12">000</text>
<text class="font-label-caps text-[4px] fill-on-surface-variant" text-anchor="start" x="188" y="101">090</text>
<text class="font-label-caps text-[4px] fill-on-surface-variant" text-anchor="middle" x="100" y="192">180</text>
<text class="font-label-caps text-[4px] fill-on-surface-variant" text-anchor="end" x="12" y="101">270</text>
<!-- TARGET BLIPS -->
<!-- T-01 -->
<g transform="translate(130, 70)">
<circle class="fill-primary-fixed-dim" r="1.5"></circle>
<circle class="blip-pulse stroke-primary-fixed-dim fill-none" r="2" stroke-width="0.5"></circle>
<text class="font-label-caps text-[5px] fill-primary-fixed-dim font-bold" x="3" y="-3">T-01</text>
</g>
<!-- T-02 -->
<g transform="translate(70, 140)">
<circle class="fill-primary-fixed-dim" r="1.5"></circle>
<circle class="blip-pulse stroke-primary-fixed-dim fill-none" r="2" stroke-width="0.5"></circle>
<text class="font-label-caps text-[5px] fill-primary-fixed-dim font-bold" x="3" y="-3">T-02</text>
</g>
<!-- T-03 -->
<g transform="translate(150, 160)">
<circle class="fill-primary-fixed-dim" r="1.5"></circle>
<circle class="blip-pulse stroke-primary-fixed-dim fill-none" r="2" stroke-width="0.5"></circle>
<text class="font-label-caps text-[5px] fill-primary-fixed-dim font-bold" x="3" y="-3">T-03</text>
</g>
<!-- Sweep Gradient Animation -->
<g class="radar-sweep">
<path d="M 100 100 L 100 5 A 95 95 0 0 1 185 145 Z" fill="url(#sweepGradient)"></path>
</g>
<defs>
<radialgradient cx="100" cy="100" gradientunits="userSpaceOnUse" id="sweepGradient" r="95">
<stop offset="0%" stop-color="#00daf3" stop-opacity="0"></stop>
<stop offset="100%" stop-color="#00daf3" stop-opacity="0.15"></stop>
</radialgradient>
</defs>
</svg>
<!-- Center Point -->
<div class="absolute w-2 h-2 bg-primary shadow-[0_0_10px_#00daf3] rotate-45 border border-background"></div>
<!-- Peripheral UI markers -->
<div class="absolute top-0 right-0 p-2 border-t border-r border-primary-fixed-dim/30 w-8 h-8"></div>
<div class="absolute top-0 left-0 p-2 border-t border-l border-primary-fixed-dim/30 w-8 h-8"></div>
<div class="absolute bottom-0 right-0 p-2 border-b border-r border-primary-fixed-dim/30 w-8 h-8"></div>
<div class="absolute bottom-0 left-0 p-2 border-b border-l border-primary-fixed-dim/30 w-8 h-8"></div>
</div>
</section>
<!-- DATA PANEL / TARGET LIST -->
<section class="px-gutter pb-20 overflow-y-auto">
<div class="flex items-center justify-between mb-3">
<h2 class="font-label-caps text-label-caps text-primary-fixed-dim flex items-center gap-2">
<span class="w-2 h-2 bg-primary-fixed-dim"></span>
          TRACKED_ENTITIES [03]
        </h2>
<span class="font-telemetry-sm text-telemetry-sm text-on-surface-variant">FREQ: 24.15GHz</span>
</div>
<div class="space-y-panel-gap">
<!-- Target Card 01 -->
<div class="bg-surface-container-low border border-outline-variant p-3 flex justify-between items-center relative overflow-hidden group hover:border-primary-fixed-dim/50 transition-colors">
<div class="absolute left-0 top-0 bottom-0 w-1 bg-primary-fixed-dim"></div>
<div class="flex flex-col gap-1">
<span class="font-data-lg text-data-lg text-primary tracking-widest">T-01</span>
<div class="flex gap-4">
<div class="flex flex-col">
<span class="font-telemetry-sm text-telemetry-sm text-on-surface-variant uppercase">Coord_XY</span>
<span class="font-data-md text-data-md text-on-surface">14.2m / -8.5m</span>
</div>
</div>
</div>
<div class="flex flex-col items-end gap-1">
<div class="flex flex-col items-end">
<span class="font-telemetry-sm text-telemetry-sm text-on-surface-variant uppercase">Velocity</span>
<span class="font-data-md text-data-md text-secondary-fixed-dim">2.4 m/s</span>
</div>
<div class="flex flex-col items-end">
<span class="font-telemetry-sm text-telemetry-sm text-on-surface-variant uppercase">Res</span>
<span class="font-data-md text-data-md text-on-surface">0.15m</span>
</div>
</div>
</div>
<!-- Target Card 02 -->
<div class="bg-surface-container-low border border-outline-variant p-3 flex justify-between items-center relative overflow-hidden group hover:border-primary-fixed-dim/50 transition-colors">
<div class="absolute left-0 top-0 bottom-0 w-1 bg-primary-fixed-dim"></div>
<div class="flex flex-col gap-1">
<span class="font-data-lg text-data-lg text-primary tracking-widest">T-02</span>
<div class="flex gap-4">
<div class="flex flex-col">
<span class="font-telemetry-sm text-telemetry-sm text-on-surface-variant uppercase">Coord_XY</span>
<span class="font-data-md text-data-md text-on-surface">-11.4m / 22.1m</span>
</div>
</div>
</div>
<div class="flex flex-col items-end gap-1">
<div class="flex flex-col items-end">
<span class="font-telemetry-sm text-telemetry-sm text-on-surface-variant uppercase">Velocity</span>
<span class="font-data-md text-data-md text-secondary-fixed-dim">0.8 m/s</span>
</div>
<div class="flex flex-col items-end">
<span class="font-telemetry-sm text-telemetry-sm text-on-surface-variant uppercase">Res</span>
<span class="font-data-md text-data-md text-on-surface">0.15m</span>
</div>
</div>
</div>
<!-- Target Card 03 -->
<div class="bg-surface-container-low border border-outline-variant p-3 flex justify-between items-center relative overflow-hidden group hover:border-primary-fixed-dim/50 transition-colors">
<div class="absolute left-0 top-0 bottom-0 w-1 bg-primary-fixed-dim"></div>
<div class="flex flex-col gap-1">
<span class="font-data-lg text-data-lg text-primary tracking-widest">T-03</span>
<div class="flex gap-4">
<div class="flex flex-col">
<span class="font-telemetry-sm text-telemetry-sm text-on-surface-variant uppercase">Coord_XY</span>
<span class="font-data-md text-data-md text-on-surface">32.8m / 4.1m</span>
</div>
</div>
</div>
<div class="flex flex-col items-end gap-1">
<div class="flex flex-col items-end">
<span class="font-telemetry-sm text-telemetry-sm text-on-surface-variant uppercase">Velocity</span>
<span class="font-data-md text-data-md text-secondary-fixed-dim">1.2 m/s</span>
</div>
<div class="flex flex-col items-end">
<span class="font-telemetry-sm text-telemetry-sm text-on-surface-variant uppercase">Res</span>
<span class="font-data-md text-data-md text-on-surface">0.15m</span>
</div>
</div>
</div>
</div>
</section>
</main>
<!-- BOTTOM NAV BAR -->
<nav class="bg-surface-container-lowest text-primary-fixed-dim font-label-caps text-label-caps docked full-width bottom-0 border-t border-outline-variant flat no shadows fixed bottom-0 left-0 w-full z-50 flex justify-around items-center h-16 px-4">
<button class="active:scale-95 transition-transform flex flex-col items-center gap-1 text-primary-fixed-dim bg-primary-container/10 rounded-lg p-2">
<span class="material-symbols-outlined" data-icon="power_settings_new">power_settings_new</span>
</button>
<button class="active:scale-95 transition-transform flex flex-col items-center gap-1 text-on-surface-variant hover:text-primary transition-colors p-2">
<span class="material-symbols-outlined" data-icon="zoom_in">zoom_in</span>
</button>
<button class="active:scale-95 transition-transform flex flex-col items-center gap-1 text-on-surface-variant hover:text-primary transition-colors p-2">
<span class="material-symbols-outlined" data-icon="zoom_out">zoom_out</span>
</button>
<button class="active:scale-95 transition-transform flex flex-col items-center gap-1 text-on-surface-variant hover:text-primary transition-colors p-2">
<span class="material-symbols-outlined" data-icon="tune">tune</span>
</button>
<button class="active:scale-95 transition-transform flex flex-col items-center gap-1 text-on-surface-variant hover:text-primary transition-colors p-2">
<span class="material-symbols-outlined" data-icon="history">history</span>
</button>
</nav>
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
