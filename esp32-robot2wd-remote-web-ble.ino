// ================================================================
//  ROBOT REMOTE CONTROL VIA BROWSER (Dual Joystick + Turn 90)
//  ESP32 + WebSocket + Skala Speed 0-300 (FreeRTOS + Modern UI)
// ================================================================

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ================== WIFI ==================
const char* ssid     = "WiFi Gedung Terpadu";
const char* password = "";

// ================== MOTOR PINS ==================
#define ENA 14  // Sekarang: Motor Kanan
#define ENB 13  // Sekarang: Motor Kiri
#define IN1 27  // Kanan MAJU
#define IN2 26  // Kanan MUNDUR
#define IN3 25  // Kiri MAJU
#define IN4 33  // Kiri MUNDUR

// ================== CONFIG ==================
#define PWM_FREQ        5000  
#define PWM_RES         8
#define LED_PIN         2
#define CMD_TIMEOUT_MS  600
#define TURN_90_DURATION_MS 400 

// ================== GLOBALS & FREERTOS ==================
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

unsigned long lastCmdTime   = 0;
int           curLeftSpeed  = 0;
int           curRightSpeed = 0;
bool          isTurning90   = false;
unsigned long turnEndTime   = 0;

SemaphoreHandle_t robotMutex;

// ================================================================
//  NEW MODERN & FUTURISTIC HTML PAGE
// ================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>NEXUS COMMAND DECK v2.0</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@500;700;900&family=Rajdhani:wght@500;600;700&family=Share+Tech+Mono&display=swap" rel="stylesheet">
<style>
:root {
  --bg-primary: #030712;
  --bg-panel: rgba(11, 15, 25, 0.65);
  --border-color: rgba(6, 182, 212, 0.15);
  --text-primary: #f3f4f6;
  --text-secondary: #9ca3af;
  --color-cyan: #06b6d4;
  --color-purple: #a855f7;
  --color-red: #ef4444;
  --color-amber: #f59e0b;
  --color-green: #10b981;
}

* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
  -webkit-tap-highlight-color: transparent;
}

body {
  font-family: 'Rajdhani', sans-serif;
  background-color: var(--bg-primary);
  background-image: 
    radial-gradient(at 50% 50%, rgba(17, 24, 39, 0.6) 0, transparent 80%),
    linear-gradient(rgba(6, 182, 212, 0.02) 1px, transparent 1px),
    linear-gradient(90deg, rgba(6, 182, 212, 0.02) 1px, transparent 1px);
  background-size: 100% 100%, 25px 25px, 25px 25px;
  color: var(--text-primary);
  overflow: hidden;
  height: 100vh;
  width: 100vw;
  touch-action: none;
  user-select: none;
  -webkit-user-select: none;
  display: flex;
  flex-direction: column;
}

/* Header Styling */
header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 24px;
  background: rgba(8, 12, 20, 0.85);
  border-bottom: 1px solid var(--border-color);
  backdrop-filter: blur(24px);
  -webkit-backdrop-filter: blur(24px);
  z-index: 10;
  box-shadow: 0 4px 30px rgba(0, 0, 0, 0.6);
}

.brand {
  display: flex;
  flex-direction: column;
}

.brand h1 {
  font-family: 'Orbitron', sans-serif;
  font-size: 18px;
  font-weight: 900;
  letter-spacing: 3px;
  background: linear-gradient(135deg, var(--color-cyan), var(--color-purple));
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  display: flex;
  align-items: center;
  gap: 8px;
}

.brand .sub {
  font-size: 10px;
  letter-spacing: 1.5px;
  color: var(--text-secondary);
  text-transform: uppercase;
  font-weight: 600;
}

.hud-metrics {
  display: flex;
  align-items: center;
  gap: 20px;
}

.metric-item {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
}

.metric-label {
  font-size: 9px;
  color: var(--text-secondary);
  letter-spacing: 1px;
  text-transform: uppercase;
}

.metric-value {
  font-family: 'Share Tech Mono', monospace;
  font-size: 13px;
  font-weight: bold;
  color: var(--color-cyan);
}

.status-badge {
  display: flex;
  align-items: center;
  gap: 6px;
  background: rgba(239, 68, 68, 0.1);
  border: 1px solid rgba(239, 68, 68, 0.2);
  padding: 4px 10px;
  border-radius: 6px;
  font-family: 'Orbitron', sans-serif;
  font-size: 10px;
  font-weight: bold;
  color: var(--color-red);
  transition: all 0.3s ease;
}

.status-badge.online {
  background: rgba(16, 185, 129, 0.1);
  border: 1px solid rgba(16, 185, 129, 0.2);
  color: var(--color-green);
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background-color: var(--color-red);
  box-shadow: 0 0 8px var(--color-red);
  animation: pulse-red 1.5s infinite;
}

.status-badge.online .status-dot {
  background-color: var(--color-green);
  box-shadow: 0 0 8px var(--color-green);
  animation: pulse-green 1.5s infinite;
}

/* Layout container */
main {
  display: flex;
  flex: 1;
  min-height: 0;
  padding: 16px;
  gap: 16px;
  background: radial-gradient(circle at center, #0f172a 0%, #05070c 100%);
}

.panel {
  display: flex;
  flex-direction: column;
  gap: 16px;
  height: 100%;
}

/* Left panel: Joysticks */
.panel-joystick {
  flex: 1.3;
}

/* Middle panel: Telemetry SVG */
.panel-visualizer {
  flex: 1.2;
}

/* Right panel: System logs & settings */
.panel-controls {
  flex: 1;
}

.glass-card {
  background: var(--bg-panel);
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
  border: 1px solid var(--border-color);
  border-radius: 16px;
  padding: 16px;
  box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.5);
  display: flex;
  flex-direction: column;
  transition: border-color 0.3s;
}

.glass-card:hover {
  border-color: rgba(6, 182, 212, 0.3);
}

.card-header-hud {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 12px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.05);
  padding-bottom: 8px;
}

.card-title-hud {
  font-family: 'Orbitron', sans-serif;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 2px;
  color: var(--text-primary);
  text-transform: uppercase;
  display: flex;
  align-items: center;
  gap: 6px;
}

.card-title-hud svg {
  color: var(--color-cyan);
}

.card-header-extra {
  font-family: 'Share Tech Mono', monospace;
  font-size: 10px;
  color: var(--text-secondary);
}

/* Joystick Layout */
.joystick-container {
  display: flex;
  flex: 1;
  align-items: center;
  justify-content: space-around;
  gap: 16px;
  min-height: 0;
}

.joy-wrapper {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
  flex: 1;
  height: 100%;
  justify-content: center;
}

.joy-label {
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 1.5px;
  color: var(--text-secondary);
  text-transform: uppercase;
}

.joy-label span {
  font-family: 'Share Tech Mono', monospace;
  color: var(--color-cyan);
  background: rgba(6, 182, 212, 0.1);
  border: 1px solid rgba(6, 182, 212, 0.2);
  padding: 1px 6px;
  border-radius: 4px;
  margin-left: 6px;
}

.joy-canvas-wrap {
  position: relative;
  aspect-ratio: 1/1;
  width: 100%;
  max-width: 220px;
  background: radial-gradient(circle, rgba(6, 182, 212, 0.02) 0%, rgba(0, 0, 0, 0.2) 100%);
  border-radius: 50%;
  border: 1px dashed rgba(6, 182, 212, 0.1);
  display: flex;
  align-items: center;
  justify-content: center;
}

canvas {
  touch-action: none;
  display: block;
  filter: drop-shadow(0 0 15px rgba(6, 182, 212, 0.1));
}

/* Telemetry Blueprint Card */
.visualizer-container {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  min-height: 0;
}

#robot-blueprint {
  max-height: 280px;
  width: 100%;
}

.tread {
  stroke-dasharray: 6, 6;
  transition: stroke-dashoffset 0.05s linear;
}

/* Diagnostic values overlaid on visualizer */
.telemetry-overlay {
  position: absolute;
  font-family: 'Share Tech Mono', monospace;
  font-size: 11px;
  background: rgba(3, 7, 18, 0.8);
  border: 1px solid rgba(255, 255, 255, 0.05);
  padding: 4px 8px;
  border-radius: 4px;
  color: var(--text-secondary);
}

.telemetry-overlay span {
  font-weight: bold;
}

.overlay-left-speed {
  bottom: 12px;
  left: 12px;
  border-left: 2px solid var(--color-cyan);
}

.overlay-right-speed {
  bottom: 12px;
  right: 12px;
  border-right: 2px solid var(--color-purple);
}

.overlay-system-mode {
  top: 12px;
  left: 12px;
}

.overlay-battery {
  top: 12px;
  right: 12px;
}

/* Progress bar row styling */
.output-vector-container {
  display: flex;
  flex-direction: column;
  gap: 12px;
  margin-bottom: 16px;
}

.bar-row {
  display: flex;
  align-items: center;
  gap: 12px;
}

.bar-label {
  font-family: 'Orbitron', sans-serif;
  font-size: 11px;
  font-weight: 800;
  width: 20px;
  color: var(--text-secondary);
}

.bar-track-outer {
  flex: 1;
  height: 16px;
  background: rgba(15, 23, 42, 0.85);
  border-radius: 6px;
  border: 1px solid rgba(255, 255, 255, 0.05);
  padding: 2px;
  overflow: hidden;
}

.bar-track-inner {
  height: 100%;
  border-radius: 4px;
  width: 0%;
  transition: width 0.08s cubic-bezier(0.1, 0.8, 0.3, 1);
  position: relative;
  overflow: hidden;
}

.bar-track-inner::after {
  content: '';
  position: absolute;
  top: 0; left: 0; right: 0; bottom: 0;
  background: linear-gradient(
    90deg, 
    rgba(255, 255, 255, 0.15) 25%, 
    transparent 25%, 
    transparent 50%, 
    rgba(255, 255, 255, 0.15) 50%, 
    rgba(255, 255, 255, 0.15) 75%, 
    transparent 75%, 
    transparent
  );
  background-size: 16px 16px;
}

.left-fill {
  background: linear-gradient(90deg, #0891b2, var(--color-cyan));
  box-shadow: 0 0 12px rgba(6, 182, 212, 0.4);
}

.right-fill {
  background: linear-gradient(90deg, #7c3aed, var(--color-purple));
  box-shadow: 0 0 12px rgba(168, 85, 247, 0.4);
}

.bar-val {
  font-family: 'Share Tech Mono', monospace;
  font-size: 14px;
  font-weight: bold;
  width: 44px;
  text-align: right;
  color: var(--text-primary);
}

.bar-val.left-val {
  color: var(--color-cyan);
}

.bar-val.right-val {
  color: var(--color-purple);
}

/* Custom Slider */
.slider-card {
  margin-bottom: 16px;
}

.slider-container {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.slider-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.slider-label {
  font-family: 'Orbitron', sans-serif;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 1px;
  color: var(--text-secondary);
}

.slider-val-glowing {
  font-family: 'Share Tech Mono', monospace;
  font-size: 16px;
  font-weight: bold;
  color: var(--color-cyan);
  text-shadow: 0 0 8px rgba(6, 182, 212, 0.5);
}

.input-slider-wrap {
  position: relative;
  display: flex;
  align-items: center;
}

input[type=range] {
  width: 100%;
  -webkit-appearance: none;
  height: 8px;
  border-radius: 99px;
  background: rgba(15, 23, 42, 0.9);
  outline: none;
  border: 1px solid rgba(255, 255, 255, 0.05);
}

input[type=range]::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 22px;
  height: 22px;
  border-radius: 50%;
  background: #f8fafc;
  cursor: pointer;
  border: 4px solid var(--color-cyan);
  box-shadow: 0 0 15px var(--color-cyan);
  transition: transform 0.1s, background-color 0.2s;
}

input[type=range]::-webkit-slider-thumb:hover {
  transform: scale(1.1);
  background: var(--color-cyan);
}

input[type=range]::-webkit-slider-thumb:active {
  transform: scale(1.25);
}

/* Action button area */
.action-buttons {
  display: flex;
  flex-direction: column;
  gap: 10px;
  margin-top: auto;
}

.btn-cyber {
  width: 100%;
  padding: 14px;
  border: none;
  border-radius: 12px;
  cursor: pointer;
  font-family: 'Orbitron', sans-serif;
  font-size: 12px;
  font-weight: 900;
  letter-spacing: 2px;
  color: white;
  transition: all 0.2s ease;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
  text-transform: uppercase;
  position: relative;
  overflow: hidden;
}

.btn-cyber::before {
  content: '';
  position: absolute;
  top: 0; left: -100%; width: 100%; height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.15), transparent);
  transition: 0.5s;
}

.btn-cyber:hover::before {
  left: 100%;
}

.btn-cyber:active {
  transform: scale(0.97);
}

.btn-turn {
  background: linear-gradient(135deg, #d97706, #b45309);
  box-shadow: 0 4px 15px rgba(217, 119, 6, 0.25);
  border: 1px solid rgba(245, 158, 11, 0.3);
}

.btn-turn:hover {
  box-shadow: 0 6px 20px rgba(217, 119, 6, 0.45);
  border-color: rgba(245, 158, 11, 0.6);
}

.btn-stop {
  background: linear-gradient(135deg, #b91c1c, #991b1b);
  box-shadow: 0 4px 20px rgba(185, 28, 28, 0.35);
  border: 1px solid rgba(239, 68, 68, 0.3);
}

.btn-stop:hover {
  box-shadow: 0 6px 25px rgba(185, 28, 28, 0.55);
  border-color: rgba(239, 68, 68, 0.6);
}

/* Warnings/Aesthetic elements */
.caution-stripes {
  height: 4px;
  background: repeating-linear-gradient(
    -45deg,
    var(--color-amber),
    var(--color-amber) 8px,
    #000 8px,
    #000 16px
  );
  border-radius: 99px;
  margin: 6px 0;
}

/* Animations */
@keyframes pulse-red {
  0% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0.7); }
  70% { box-shadow: 0 0 0 10px rgba(239, 68, 68, 0); }
  100% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0); }
}

@keyframes pulse-green {
  0% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.7); }
  70% { box-shadow: 0 0 0 10px rgba(16, 185, 129, 0); }
  100% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
}

.pulse-red-glow {
  animation: svg-red-glow 2s infinite alternate;
}

.pulse-amber-glow {
  animation: svg-amber-glow 2s infinite alternate;
}

.pulse-cyan-glow {
  animation: svg-cyan-glow 2s infinite alternate;
}

@keyframes svg-red-glow {
  from { filter: drop-shadow(0 0 2px var(--color-red)); fill: #dc2626; }
  to { filter: drop-shadow(0 0 8px var(--color-red)); fill: #f87171; }
}

@keyframes svg-amber-glow {
  from { filter: drop-shadow(0 0 2px var(--color-amber)); fill: #d97706; }
  to { filter: drop-shadow(0 0 8px var(--color-amber)); fill: #fbbf24; }
}

@keyframes svg-cyan-glow {
  from { filter: drop-shadow(0 0 2px var(--color-cyan)); fill: #0891b2; }
  to { filter: drop-shadow(0 0 8px var(--color-cyan)); fill: #67e8f9; }
}

/* Responsive Overrides */
@media (max-width: 1024px) {
  main {
    flex-direction: column;
    overflow-y: auto;
    padding: 10px;
    gap: 10px;
  }

  .panel {
    height: auto;
    flex: none !important;
  }

  .panel-visualizer {
    order: 1;
  }
  
  #robot-blueprint {
    max-height: 200px;
  }

  .panel-joystick {
    order: 3;
  }

  .joystick-container {
    padding: 10px 0;
  }

  .joy-canvas-wrap {
    max-width: 150px;
  }

  .panel-controls {
    order: 2;
    flex-direction: row !important;
    flex-wrap: wrap;
    gap: 10px;
  }

  .panel-controls .glass-card {
    flex: 1;
    min-width: 200px;
    margin-bottom: 0;
  }

  .action-buttons {
    flex: 1.2;
    min-width: 200px;
    margin-top: 0;
  }
}

@media (max-width: 480px) {
  header {
    padding: 10px 14px;
  }
  .brand h1 {
    font-size: 15px;
  }
  .hud-metrics {
    gap: 10px;
  }
  .metric-item:not(.status-badge-wrap) {
    display: none;
  }
  .joystick-container {
    gap: 8px;
  }
  .joy-canvas-wrap {
    max-width: 140px;
  }
}
</style>
</head>
<body>
<header>
  <div class="brand">
    <h1><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/></svg> SYSTEM NEXUS</h1>
    <div class="sub">Tactile Telemetry Control</div>
  </div>
  <div class="hud-metrics">
    <div class="metric-item">
      <span class="metric-label">RTT Latency</span>
      <span class="metric-value" id="pingVal">-- ms</span>
    </div>
    <div class="metric-item">
      <span class="metric-label">Packets Tx</span>
      <span class="metric-value" id="txVal">0</span>
    </div>
    <div class="metric-item status-badge-wrap">
      <div class="status-badge" id="statusBadge">
        <div class="status-dot"></div>
        <span id="statusText">OFFLINE</span>
      </div>
    </div>
  </div>
</header>

<main>
  <!-- Left panel: Tactical Joysticks -->
  <div class="panel panel-joystick glass-card">
    <div class="card-header-hud">
      <div class="card-title-hud">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><circle cx="12" cy="12" r="10"/><path d="M12 2v20M2 12h20"/></svg>
        Tactical Interface
      </div>
      <div class="card-header-extra">DUAL-THUMB JOYSTICK</div>
    </div>
    <div class="joystick-container">
      <div class="joy-wrapper">
        <div class="joy-label">DRIVE <span>L-THRUST</span></div>
        <div class="joy-canvas-wrap">
          <canvas id="joyLeft"></canvas>
        </div>
      </div>
      <div class="joy-wrapper">
        <div class="joy-label">STEER <span>R-STEER</span></div>
        <div class="joy-canvas-wrap">
          <canvas id="joyRight"></canvas>
        </div>
      </div>
    </div>
  </div>

  <!-- Middle panel: Robot Blueprint Visualizer -->
  <div class="panel panel-visualizer glass-card">
    <div class="card-header-hud">
      <div class="card-title-hud">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"/><path d="M9 3v18M15 3v18M3 9h18M3 15h18"/></svg>
        Telemetry Visualizer
      </div>
      <div class="card-header-extra">REAL-TIME BLUEPRINT</div>
    </div>
    <div class="visualizer-container">
      <!-- SVG Blueprint -->
      <svg id="robot-blueprint" viewBox="0 0 200 200">
        <defs>
          <pattern id="grid-pattern" width="16" height="16" patternUnits="userSpaceOnUse">
            <path d="M 16 0 L 0 0 0 16" fill="none" stroke="rgba(6, 182, 212, 0.05)" stroke-width="0.8"/>
          </pattern>
        </defs>
        <rect width="100%" height="100%" fill="url(#grid-pattern)" rx="8" />
        
        <!-- Axis indicators -->
        <line x1="100" y1="0" x2="100" y2="200" stroke="rgba(255,255,255,0.03)" stroke-width="1" />
        <line x1="0" y1="100" x2="200" y2="100" stroke="rgba(255,255,255,0.03)" stroke-width="1" />
        
        <!-- Turn indicators (arc path) -->
        <path id="vector-left" d="M 75,50 A 25,25 0 0,0 45,80" fill="none" stroke="rgba(255, 255, 255, 0.05)" stroke-width="3" stroke-linecap="round" />
        <path id="vector-right" d="M 125,50 A 25,25 0 0,1 155,80" fill="none" stroke="rgba(255, 255, 255, 0.05)" stroke-width="3" stroke-linecap="round" />
        
        <!-- Robot Chassis -->
        <rect x="52" y="62" width="96" height="86" rx="14" fill="rgba(15, 23, 42, 0.75)" stroke="rgba(6, 182, 212, 0.3)" stroke-width="1.5" />
        <rect x="50" y="60" width="100" height="90" rx="16" fill="none" stroke="var(--color-cyan)" stroke-width="2" style="filter: drop-shadow(0 0 4px rgba(6, 182, 212, 0.3));" />
        
        <!-- Internal Details / Schematic lines -->
        <line x1="70" y1="105" x2="130" y2="105" stroke="rgba(6, 182, 212, 0.15)" stroke-width="1.5" />
        <line x1="100" y1="75" x2="100" y2="135" stroke="rgba(6, 182, 212, 0.15)" stroke-width="1.5" />
        <circle cx="100" cy="105" r="22" fill="none" stroke="rgba(168, 85, 247, 0.2)" stroke-width="1.5" />
        
        <!-- Center Core LED -->
        <circle id="core-led" cx="100" cy="105" r="7" fill="var(--color-red)" class="pulse-red-glow" />
        
        <!-- Front Wheel (Caster) -->
        <rect x="91" y="32" width="18" height="18" rx="4" fill="rgba(30, 41, 59, 0.9)" stroke="#64748b" stroke-width="1.5" />
        <circle cx="100" cy="41" r="3" fill="#64748b" />
        
        <!-- Left Wheel -->
        <g>
          <rect x="25" y="75" width="20" height="60" rx="6" fill="#090d16" stroke="var(--color-cyan)" stroke-width="2" />
          <line id="left-tread" x1="35" y1="77" x2="35" y2="133" stroke="var(--color-cyan)" stroke-width="16" stroke-dasharray="6,6" stroke-linecap="butt" class="tread" stroke-dashoffset="0" />
        </g>
        
        <!-- Right Wheel -->
        <g>
          <rect x="155" y="75" width="20" height="60" rx="6" fill="#090d16" stroke="var(--color-purple)" stroke-width="2" />
          <line id="right-tread" x1="165" y1="77" x2="165" y2="133" stroke="var(--color-purple)" stroke-width="16" stroke-dasharray="6,6" stroke-linecap="butt" class="tread" stroke-dashoffset="0" />
        </g>
        
        <!-- Vectors arrows -->
        <path id="vector-fwd" d="M 100,20 L 100,5 M 100,5 L 94,11 M 100,5 L 106,11" fill="none" stroke="rgba(255,255,255,0.06)" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" />
        <path id="vector-bwd" d="M 100,180 L 100,195 M 100,195 L 94,189 M 100,195 L 106,189" fill="none" stroke="rgba(255,255,255,0.06)" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" />
      </svg>
      
      <!-- Overlay details -->
      <div class="telemetry-overlay overlay-system-mode">SYSTEM: <span id="telemetryMode" style="color: var(--color-red);">OFFLINE</span></div>
      <div class="telemetry-overlay overlay-battery">VOLTAGE: <span style="color: var(--color-green);">5.0V (USB)</span></div>
      <div class="telemetry-overlay overlay-left-speed">MOTOR-L: <span id="telemetryLeftSpeed">0</span></div>
      <div class="telemetry-overlay overlay-right-speed">MOTOR-R: <span id="telemetryRightSpeed">0</span></div>
    </div>
  </div>

  <!-- Right panel: Vector Power meters & Core settings -->
  <div class="panel panel-controls">
    <div class="glass-card">
      <div class="card-header-hud">
        <div class="card-title-hud">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M12 2v20M17 5H9.5a3.5 3.5 0 0 0 0 7h5a3.5 3.5 0 0 1 0 7H6"/></svg>
          Vector Output
        </div>
        <div class="card-header-extra">TELEMETRY DIAG</div>
      </div>
      <div class="output-vector-container">
        <div class="bar-row">
          <span class="bar-label">L</span>
          <div class="bar-track-outer">
            <div class="bar-track-inner left-fill" id="lBar"></div>
          </div>
          <span class="bar-val left-val" id="lVal">0</span>
        </div>
        <div class="bar-row">
          <span class="bar-label">R</span>
          <div class="bar-track-outer">
            <div class="bar-track-inner right-fill" id="rBar"></div>
          </div>
          <span class="bar-val right-val" id="rVal">0</span>
        </div>
      </div>
    </div>

    <!-- Core settings card -->
    <div class="glass-card slider-card">
      <div class="card-header-hud">
        <div class="card-title-hud">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><line x1="4" y1="21" x2="4" y2="14"/><line x1="4" y1="10" x2="4" y2="3"/><line x1="12" y1="21" x2="12" y2="12"/><line x1="12" y1="8" x2="12" y2="3"/><line x1="20" y1="21" x2="20" y2="16"/><line x1="20" y1="12" x2="20" y2="3"/><line x1="1" y1="14" x2="7" y2="14"/><line x1="9" y1="8" x2="15" y2="8"/><line x1="17" y1="16" x2="23" y2="16"/></svg>
          Core Calibration
        </div>
        <div class="card-header-extra">PWM LIMIT</div>
      </div>
      <div class="slider-container">
        <div class="slider-row">
          <span class="slider-label">CORE THROTTLE</span>
          <span class="slider-val-glowing" id="spdVal">100%</span>
        </div>
        <div class="input-slider-wrap">
          <input type="range" min="20" max="100" value="100" id="spdSlider">
        </div>
      </div>
    </div>

    <!-- Warning / Stop Card -->
    <div class="action-buttons">
      <div class="caution-stripes"></div>
      <button class="btn-cyber btn-turn" id="turnBtn">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M21.5 2v6h-6M21.34 15.57a10 10 0 1 1-.57-8.38l5.67-5.67"/></svg>
        TURN 90&deg; VECTOR
      </button>
      <button class="btn-cyber btn-stop" id="stopBtn">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"/></svg>
        EMERGENCY SHUTDOWN
      </button>
      <div class="caution-stripes"></div>
    </div>
  </div>
</main>

<script>
let ws=null,connected=false;
const statusBadge=document.getElementById('statusBadge');
const statusText=document.getElementById('statusText');
const pingVal=document.getElementById('pingVal');
const txVal=document.getElementById('txVal');
const coreLed=document.getElementById('core-led');
const telemetryMode=document.getElementById('telemetryMode');

let packetTxCount = 0;
let lastPingTime = 0;
let pingInterval = null;

function wsConnect(){
  statusText.textContent='CONNECTING';
  statusBadge.style.borderColor='rgba(245, 158, 11, 0.4)';
  statusBadge.style.backgroundColor='rgba(245, 158, 11, 0.1)';
  statusBadge.style.color='var(--color-amber)';
  statusBadge.classList.remove('online');
  coreLed.setAttribute('fill', 'var(--color-amber)');
  coreLed.setAttribute('class', 'pulse-amber-glow');
  telemetryMode.textContent = 'CONNECTING';
  telemetryMode.style.color = 'var(--color-amber)';
  
  ws=new WebSocket('ws://'+location.hostname+'/ws');
  
  ws.onopen=()=>{
    connected=true;
    statusBadge.classList.add('online');
    statusText.textContent='ONLINE';
    statusBadge.style.borderColor='rgba(16, 185, 129, 0.4)';
    statusBadge.style.backgroundColor='rgba(16, 185, 129, 0.1)';
    statusBadge.style.color='var(--color-green)';
    coreLed.setAttribute('fill', 'var(--color-cyan)');
    coreLed.setAttribute('class', 'pulse-cyan-glow');
    telemetryMode.textContent = 'STANDBY';
    telemetryMode.style.color = 'var(--color-cyan)';
    pingVal.textContent = '-- ms';
    
    // Start Ping-Pong RTT telemetry
    pingInterval = setInterval(() => {
      if (connected && ws && ws.readyState === 1) {
        lastPingTime = performance.now();
        ws.send('P');
      }
    }, 2000);
  };
  
  ws.onmessage=(event)=>{
    if(event.data === 'P' && lastPingTime > 0){
      const rtt = Math.round(performance.now() - lastPingTime);
      pingVal.textContent = rtt + ' ms';
    }
  };
  
  ws.onclose=()=>{
    connected=false;
    statusBadge.classList.remove('online');
    statusText.textContent='OFFLINE';
    statusBadge.style.borderColor='rgba(239, 68, 68, 0.4)';
    statusBadge.style.backgroundColor='rgba(239, 68, 68, 0.1)';
    statusBadge.style.color='var(--color-red)';
    coreLed.setAttribute('fill', 'var(--color-red)');
    coreLed.setAttribute('class', 'pulse-red-glow');
    telemetryMode.textContent = 'OFFLINE';
    telemetryMode.style.color = 'var(--color-red)';
    pingVal.textContent = '-- ms';
    
    if (pingInterval) {
      clearInterval(pingInterval);
      pingInterval = null;
    }
    
    // Reset outputs on disconnect
    lBar.style.width='0%';
    rBar.style.width='0%';
    lVal.textContent='0';
    rVal.textContent='0';
    telemetryLeftSpeed.textContent = '0';
    telemetryRightSpeed.textContent = '0';
    jLeft.reset();
    jRight.reset();
    
    setTimeout(wsConnect,1500);
  };
  
  ws.onerror=()=>{ws.close()};
}
wsConnect();

class Joystick {
  constructor(canvasId, axisX, axisY) {
    this.canvas=document.getElementById(canvasId);
    this.ctx=this.canvas.getContext('2d');
    this.axisX=axisX; 
    this.axisY=axisY;
    this.cx=0;
    this.cy=0;
    this.rad=0;
    this.knobR=0;
    this.knobX=0;
    this.knobY=0;
    this.active=false;
    this.touchId=null;
    this.vx=0;
    this.vy=0;
    this.resize();
    this.bindEvents();
    this.draw();
  }
  
  resize(){
    const wrap=this.canvas.parentElement;
    const avail=Math.min(wrap.clientWidth,wrap.clientHeight);
    const s=Math.max(100,avail*0.9);
    const dpr=devicePixelRatio || 1;
    this.canvas.width=this.canvas.height=s*dpr;
    this.canvas.style.width=this.canvas.style.height=s+'px';
    this.ctx.scale(dpr,dpr);
    this.cx=this.cy=s/2;
    this.rad=s/2-14;
    this.knobR=this.rad*0.28;
    this.knobX=this.cx;
    this.knobY=this.cy;
  }
  
  moveKnob(px,py){
    let dx=this.axisX?px-this.cx:0;
    let dy=this.axisY?py-this.cy:0;
    const dist=Math.hypot(dx,dy);
    if(dist>this.rad){
      dx=dx/dist*this.rad;
      dy=dy/dist*this.rad;
    }
    this.knobX=this.cx+dx;
    this.knobY=this.cy+dy;
    this.vx=this.axisX?Math.round(dx/this.rad*300):0;
    this.vy=this.axisY?Math.round(-dy/this.rad*300):0;
  }
  
  reset(){
    this.active=false;
    this.touchId=null;
    this.knobX=this.cx;
    this.knobY=this.cy;
    this.vx=0;
    this.vy=0;
  }
  
  getPos(e,touch){
    const r=this.canvas.getBoundingClientRect();
    return{x:touch.clientX-r.left,y:touch.clientY-r.top};
  }
  
  bindEvents(){
    const c=this.canvas;
    c.addEventListener('touchstart',e=>{
      e.preventDefault();
      if(this.touchId!==null)return;
      const t=e.changedTouches[0];
      this.touchId=t.identifier;
      this.active=true;
      const p=this.getPos(e,t);
      this.moveKnob(p.x,p.y);
      if(navigator.vibrate)navigator.vibrate(12);
    },{passive:false});
    
    c.addEventListener('touchmove',e=>{
      e.preventDefault();
      for(const t of e.changedTouches){
        if(t.identifier===this.touchId){
          const p=this.getPos(e,t);
          this.moveKnob(p.x,p.y);
        }
      }
    },{passive:false});
    
    c.addEventListener('touchend',e=>{
      for(const t of e.changedTouches){
        if(t.identifier===this.touchId)this.reset();
      }
    });
    
    c.addEventListener('touchcancel',()=>this.reset());
    
    c.addEventListener('mousedown',e=>{
      e.preventDefault();
      this.active=true;
      const r=c.getBoundingClientRect();
      this.moveKnob(e.clientX-r.left,e.clientY-r.top);
    });
    
    c.addEventListener('mousemove',e=>{
      if(!this.active)return;
      const r=c.getBoundingClientRect();
      this.moveKnob(e.clientX-r.left,e.clientY-r.top);
    });
    
    c.addEventListener('mouseup',()=>this.reset());
    c.addEventListener('mouseleave',()=>this.reset());
  }
  
  draw(){
    const ctx=this.ctx;
    const s=this.canvas.width/(devicePixelRatio || 1);
    ctx.clearRect(0,0,s,s);
    
    // Outer Track Ring (Glowing / HUD styling)
    ctx.beginPath();
    ctx.arc(this.cx,this.cy,this.rad,0,Math.PI*2);
    ctx.strokeStyle=this.active? 'rgba(6, 182, 212, 0.45)' : 'rgba(6, 182, 212, 0.15)';
    ctx.lineWidth=2.5;
    ctx.stroke();
    
    // Secondary inner ticks/ring
    ctx.beginPath();
    ctx.arc(this.cx,this.cy,this.rad*0.6,0,Math.PI*2);
    ctx.strokeStyle='rgba(6, 182, 212, 0.05)';
    ctx.setLineDash([4, 4]);
    ctx.lineWidth=1;
    ctx.stroke();
    ctx.setLineDash([]); // Reset
    
    // Grid Crosshair lines
    ctx.strokeStyle='rgba(6, 182, 212, 0.08)';
    ctx.lineWidth=1.5;
    if(this.axisX){
      ctx.beginPath();
      ctx.moveTo(this.cx-this.rad,this.cy);
      ctx.lineTo(this.cx+this.rad,this.cy);
      ctx.stroke();
      
      // Arrow ticks on X axis
      ctx.fillStyle='rgba(6, 182, 212, 0.3)';
      ctx.beginPath();
      ctx.moveTo(this.cx + this.rad - 6, this.cy - 4);
      ctx.lineTo(this.cx + this.rad, this.cy);
      ctx.lineTo(this.cx + this.rad - 6, this.cy + 4);
      ctx.fill();
      
      ctx.beginPath();
      ctx.moveTo(this.cx - this.rad + 6, this.cy - 4);
      ctx.lineTo(this.cx - this.rad, this.cy);
      ctx.lineTo(this.cx - this.rad + 6, this.cy + 4);
      ctx.fill();
    }
    if(this.axisY){
      ctx.beginPath();
      ctx.moveTo(this.cx,this.cy-this.rad);
      ctx.lineTo(this.cx,this.cy+this.rad);
      ctx.stroke();
      
      // Arrow ticks on Y axis
      ctx.fillStyle='rgba(6, 182, 212, 0.3)';
      ctx.beginPath();
      ctx.moveTo(this.cx - 4, this.cy - this.rad + 6);
      ctx.lineTo(this.cx, this.cy - this.rad);
      ctx.lineTo(this.cx + 4, this.cy - this.rad + 6);
      ctx.fill();
      
      ctx.beginPath();
      ctx.moveTo(this.cx - 4, this.cy + this.rad - 6);
      ctx.lineTo(this.cx, this.cy + this.rad);
      ctx.lineTo(this.cx + 4, this.cy + this.rad - 6);
      ctx.fill();
    }
    
    // Center Core Anchor
    ctx.beginPath();
    ctx.arc(this.cx,this.cy,6,0,Math.PI*2);
    ctx.fillStyle='rgba(6, 182, 212, 0.2)';
    ctx.fill();
    ctx.strokeStyle='rgba(6, 182, 212, 0.5)';
    ctx.stroke();
    
    // Glowing active trace
    if (this.active) {
      ctx.beginPath();
      ctx.moveTo(this.cx, this.cy);
      ctx.lineTo(this.knobX, this.knobY);
      ctx.strokeStyle = this.axisX ? 'rgba(168, 85, 247, 0.3)' : 'rgba(6, 182, 212, 0.3)';
      ctx.lineWidth = 2;
      ctx.stroke();
    }
    
    // Joystick Knob Shadow Ring
    if(this.active){
      ctx.beginPath();
      ctx.arc(this.knobX,this.knobY,this.knobR+6,0,Math.PI*2);
      ctx.fillStyle=this.axisX?'rgba(168, 85, 247, 0.12)':'rgba(6, 182, 212, 0.12)';
      ctx.fill();
    }
    
    // Joystick Knob
    const g=ctx.createRadialGradient(this.knobX-this.knobR*0.15,this.knobY-this.knobR*0.15,0,this.knobX,this.knobY,this.knobR);
    if(this.active){
      if (this.axisX) {
        g.addColorStop(0,'#c084fc');g.addColorStop(1,'#7e22ce');
      } else {
        g.addColorStop(0,'#22d3ee');g.addColorStop(1,'#0891b2');
      }
    } else {
      g.addColorStop(0,'#4b5563');g.addColorStop(1,'#1f2937');
    }
    
    ctx.beginPath();
    ctx.arc(this.knobX,this.knobY,this.knobR,0,Math.PI*2);
    ctx.fillStyle=g;
    ctx.fill();
    ctx.strokeStyle=this.active ? 'rgba(255,255,255,0.45)' : 'rgba(255,255,255,0.1)';
    ctx.lineWidth=1.5;
    ctx.stroke();
    
    // Digital readout inside canvas
    ctx.font = '9px "Share Tech Mono"';
    ctx.fillStyle = 'rgba(255, 255, 255, 0.3)';
    if (this.axisY) {
      ctx.fillText('THRUST: ' + (this.vy > 0 ? '+' : '') + this.vy, 8, 16);
    }
    if (this.axisX) {
      ctx.fillText('STEER: ' + (this.vx > 0 ? '+' : '') + this.vx, 8, 16);
    }
    
    requestAnimationFrame(()=>this.draw());
  }
}

const jLeft =new Joystick('joyLeft', false,true);
const jRight=new Joystick('joyRight',true, false);

window.addEventListener('resize',()=>{
  jLeft.resize();
  jRight.resize();
});

function applyTurnCurve(val){
  const norm=val/300;
  const curved=Math.sign(norm)*Math.pow(Math.abs(norm),1.8);
  return Math.round(curved*300*0.275);
}

// References
const lBar=document.getElementById('lBar');
const rBar=document.getElementById('rBar');
const lVal=document.getElementById('lVal');
const rVal=document.getElementById('rVal');
const spdSlider=document.getElementById('spdSlider');
const spdVal=document.getElementById('spdVal');

const leftTread = document.getElementById('left-tread');
const rightTread = document.getElementById('right-tread');
const vectorFwd = document.getElementById('vector-fwd');
const vectorBwd = document.getElementById('vector-bwd');
const vectorLeft = document.getElementById('vector-left');
const vectorRight = document.getElementById('vector-right');

const telemetryLeftSpeed = document.getElementById('telemetryLeftSpeed');
const telemetryRightSpeed = document.getElementById('telemetryRightSpeed');

let maxSpd=100;
let leftTreadOffset = 0;
let rightTreadOffset = 0;
let lastSentSx = 0;
let lastSentSy = 0;
let lastSentTime = 0;

spdSlider.addEventListener('input',()=>{
  maxSpd=parseInt(spdSlider.value);
  spdVal.textContent=maxSpd+'%';
});

// Telemetry & WebSocket Send loop
setInterval(()=>{
  if(!connected||!ws||ws.readyState!==1)return;
  
  const scale=maxSpd/100;
  const sy=Math.round(jLeft.vy*scale);
  const sx=Math.round(applyTurnCurve(jRight.vx)*scale); // Scale turn proportionally with core speed
  
  const changed = (sx !== lastSentSx || sy !== lastSentSy);
  const timeoutReached = (Date.now() - lastSentTime > 250);
  
  if (changed || timeoutReached) {
    ws.send('J:'+sx+','+sy);
    lastSentSx = sx;
    lastSentSy = sy;
    lastSentTime = Date.now();
    packetTxCount++;
    txVal.textContent = packetTxCount;
  }
  
  // Update speeds
  const ml=Math.min(300,Math.max(0,Math.abs(sy-sx)));
  const mr=Math.min(300,Math.max(0,Math.abs(sy+sx)));
  
  const finalLeftVal = (sy - sx);
  const finalRightVal = (sy + sx);
  
  // UI Progress bars
  lBar.style.width=(ml/300*100)+'%';
  rBar.style.width=(mr/300*100)+'%';
  lVal.textContent=(finalLeftVal >= 0 ? '+' : '') + finalLeftVal;
  rVal.textContent=(finalRightVal >= 0 ? '+' : '') + finalRightVal;
  
  // Visualizer updates
  telemetryLeftSpeed.textContent = finalLeftVal;
  telemetryRightSpeed.textContent = finalRightVal;
  
  if (finalLeftVal !== 0 || finalRightVal !== 0) {
    telemetryMode.textContent = 'ACTIVE COMMAND';
    telemetryMode.style.color = 'var(--color-amber)';
  } else {
    telemetryMode.textContent = 'STANDBY';
    telemetryMode.style.color = 'var(--color-cyan)';
  }
  
  // Tread animation (SVG offset shift)
  if (finalLeftVal !== 0) {
    leftTreadOffset += (finalLeftVal / 300) * 8;
    leftTread.style.strokeDashoffset = leftTreadOffset;
  }
  if (finalRightVal !== 0) {
    rightTreadOffset += (finalRightVal / 300) * 8;
    rightTread.style.strokeDashoffset = rightTreadOffset;
  }
  
  // Arrow highlight
  if (sy > 30) {
    vectorFwd.setAttribute('stroke', 'var(--color-cyan)');
    vectorFwd.style.filter = 'drop-shadow(0 0 6px var(--color-cyan))';
    vectorBwd.setAttribute('stroke', 'rgba(255,255,255,0.06)');
    vectorBwd.style.filter = 'none';
  } else if (sy < -30) {
    vectorBwd.setAttribute('stroke', 'var(--color-cyan)');
    vectorBwd.style.filter = 'drop-shadow(0 0 6px var(--color-cyan))';
    vectorFwd.setAttribute('stroke', 'rgba(255,255,255,0.06)');
    vectorFwd.style.filter = 'none';
  } else {
    vectorFwd.setAttribute('stroke', 'rgba(255,255,255,0.06)');
    vectorFwd.style.filter = 'none';
    vectorBwd.setAttribute('stroke', 'rgba(255,255,255,0.06)');
    vectorBwd.style.filter = 'none';
  }
  
  // Turn highlight
  if (sx > 20) {
    vectorRight.setAttribute('stroke', 'var(--color-purple)');
    vectorRight.style.filter = 'drop-shadow(0 0 6px var(--color-purple))';
    vectorLeft.setAttribute('stroke', 'rgba(255,255,255,0.05)');
    vectorLeft.style.filter = 'none';
  } else if (sx < -20) {
    vectorLeft.setAttribute('stroke', 'var(--color-purple)');
    vectorLeft.style.filter = 'drop-shadow(0 0 6px var(--color-purple))';
    vectorRight.setAttribute('stroke', 'rgba(255,255,255,0.05)');
    vectorRight.style.filter = 'none';
  } else {
    vectorLeft.setAttribute('stroke', 'rgba(255,255,255,0.05)');
    vectorLeft.style.filter = 'none';
    vectorRight.setAttribute('stroke', 'rgba(255,255,255,0.05)');
    vectorRight.style.filter = 'none';
  }
},50);

// Turn 90 Degree Vector Button
document.getElementById('turnBtn').addEventListener('click',()=>{
  if(ws&&ws.readyState===1) {
    ws.send('T90');
    packetTxCount++;
    txVal.textContent = packetTxCount;
    
    // Visual flash in HUD
    telemetryMode.textContent = 'EXECUTING TURN 90';
    telemetryMode.style.color = 'var(--color-amber)';
    if(navigator.vibrate)navigator.vibrate(30);
  }
});

// Emergency Stop Button
document.getElementById('stopBtn').addEventListener('click',()=>{
  if(ws&&ws.readyState===1) {
    ws.send('S');
    packetTxCount++;
    txVal.textContent = packetTxCount;
  }
  jLeft.reset();
  jRight.reset();
  
  lBar.style.width='0%';
  rBar.style.width='0%';
  lVal.textContent='0';
  rVal.textContent='0';
  telemetryLeftSpeed.textContent = '0';
  telemetryRightSpeed.textContent = '0';
  
  telemetryMode.textContent = 'E-STOP CRITICAL';
  telemetryMode.style.color = 'var(--color-red)';
  
  if(navigator.vibrate)navigator.vibrate([60, 40, 60]);
  
  // Visual shutdown indication: Flash red chassis border
  const bodyBorder = document.querySelector('#robot-blueprint rect:nth-of-type(3)');
  if(bodyBorder) {
    bodyBorder.setAttribute('stroke', 'var(--color-red)');
    bodyBorder.style.filter = 'drop-shadow(0 0 12px var(--color-red))';
    setTimeout(() => {
      bodyBorder.setAttribute('stroke', 'var(--color-cyan)');
      bodyBorder.style.filter = 'none';
    }, 1000);
  }
});
</script>
</body>
</html>
)rawliteral";

// ================================================================
//  MOTOR FUNCTIONS
// ================================================================
void setupMotor(){
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  ledcAttach(ENA, PWM_FREQ, PWM_RES);
  ledcAttach(ENB, PWM_FREQ, PWM_RES);
}

void setMotor(int kiri, int kanan){
  curLeftSpeed = kiri;
  curRightSpeed = kanan;

  digitalWrite(IN3, kiri > 0);
  digitalWrite(IN4, kiri < 0);
  // Note: The right motor's physical wiring on the chassis is reversed.
  // In order to drive forward when kanan > 0, we set IN2 (Mundur pin) to HIGH
  // and IN1 (Maju pin) to LOW.
  digitalWrite(IN1, kanan < 0);
  digitalWrite(IN2, kanan > 0);

  int pwmKiri = map(abs(kiri), 0, 300, 0, 255);
  int pwmKanan = map(abs(kanan), 0, 300, 0, 255);

  ledcWrite(ENB, constrain(pwmKiri, 0, 255));
  ledcWrite(ENA, constrain(pwmKanan, 0, 255));
}

void stopMotors(){
  setMotor(0, 0);
}

// ================================================================
//  WEBSOCKET HANDLER
// ================================================================
void onWsEvent(AsyncWebSocket *srv, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("[WS] Client #%u connected\n", client->id());
  }
  else if(type == WS_EVT_DISCONNECT){
    Serial.printf("[WS] Client #%u disconnected\n", client->id());
    if (xSemaphoreTake(robotMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      stopMotors();
      xSemaphoreGive(robotMutex);
    }
  }
  else if(type == WS_EVT_DATA){
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT){
      // Safe conversion to avoid modifying the read-only or exact-sized incoming heap buffer (out-of-bounds write prevention)
      String msg;
      msg.reserve(len);
      for(size_t i = 0; i < len; i++){
        msg += (char)data[i];
      }

      if (xSemaphoreTake(robotMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        if(msg == "P"){
          // Telemetry ping request: respond immediately with a pong message
          client->text("P");
        }
        else if(msg == "T90"){
          isTurning90 = true;
          turnEndTime = millis() + TURN_90_DURATION_MS;
          setMotor(280, -280); 
          lastCmdTime = millis();
          Serial.println("[CMD] TURN 90 DEG");
        }
        else if(msg == "S"){
          isTurning90 = false;
          stopMotors();
          lastCmdTime = millis();
          Serial.println("[CMD] EMERGENCY STOP");
        }
        else if(msg.startsWith("J:") && !isTurning90){
          int comma = msg.indexOf(',', 2);
          if(comma > 0){
            int x = msg.substring(2, comma).toInt();
            int y = msg.substring(comma+1).toInt();

            if(abs(y) < 13) y = 0;
            if(abs(x) < 25) x = 0;

            int leftSpd  = constrain(y - x, -300, 300);
            int rightSpd = constrain(y + x, -300, 300);

            setMotor(leftSpd, rightSpd);
            lastCmdTime = millis();
          }
        }
        xSemaphoreGive(robotMutex);
      }
    }
  }
}

// ================================================================
//  FREERTOS TASKS
// ================================================================
void vRobotControlTask(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(20);

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    if (xSemaphoreTake(robotMutex, portMAX_DELAY) == pdTRUE) {
      if (isTurning90 && millis() >= turnEndTime) {
        isTurning90 = false;
        stopMotors();
        Serial.println("[STATUS] Turn 90 Finished");
      }

      if (!isTurning90 && (millis() - lastCmdTime > CMD_TIMEOUT_MS) 
          && (curLeftSpeed != 0 || curRightSpeed != 0)) {
        stopMotors();
        Serial.println("[SAFETY] Auto-stop");
      }
      xSemaphoreGive(robotMutex);
    }
  }
}

void vSystemMonitorTask(void *pvParameters) {
  uint32_t lastWifiCheck = 0;
  uint32_t lastClean = 0;

  for (;;) {
    uint32_t now = millis();

    if (now - lastClean > 1000) {
      ws.cleanupClients();
      lastClean = now;
    }

    if (now - lastWifiCheck > 3000) {
      lastWifiCheck = now;
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Reconnecting...");
        digitalWrite(LED_PIN, LOW);
        WiFi.disconnect();
        WiFi.begin(ssid, password);
      } else {
        digitalWrite(LED_PIN, HIGH);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ================================================================
//  SETUP
// ================================================================
void setup(){
  Serial.begin(115200);
  Serial.println("\n=== Robot Remote WiFi FreeRTOS (Modern UI) ===");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  setupMotor();
  stopMotors();

  robotMutex = xSemaphoreCreateMutex();

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to '%s'", ssid);

  while(WiFi.status() != WL_CONNECTED){
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }

  Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
  digitalWrite(LED_PIN, HIGH);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send_P(200, "text/html", index_html);
  });

  server.begin();
  Serial.println("Web server started!");

  xTaskCreatePinnedToCore(vRobotControlTask, "RobotControl", 3000, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(vSystemMonitorTask, "SysMonitor", 3000, NULL, 1, NULL, 0);
}

void loop(){
  vTaskDelete(NULL); 
}