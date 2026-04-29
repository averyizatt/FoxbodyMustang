// ---------------------------------------------------------------------------
// wifi_server.cpp
// WiFi (AP / Station) + synchronous HTTP server.
// The settings page HTML/CSS/JS is embedded as a raw-string literal so no
// filesystem (SPIFFS / LittleFS) is required.
// ---------------------------------------------------------------------------

#include "wifi_server.h"
#include "settings.h"
#include "states.h"

#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Preview state — set by POST /api/preview, consumed in main.cpp loop().
// Declared here and extern'd in main.cpp.
volatile LightState    g_preview_state    = LightState::OFF;
volatile unsigned long g_preview_until_ms = 0;

static WebServer _server(80);

// ---------------------------------------------------------------------------
// Embedded web page (stored in flash — no SRAM copy needed via send_P)
// ---------------------------------------------------------------------------
static const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>Foxbody Taillights</title>
<style>
:root{
  --bg:#0d0d0f;--surface:#1c1c1e;--surface2:#2c2c2e;--border:#3a3a3c;
  --accent:#e31c25;--amber:#ff8c00;--green:#34c759;--blue:#0a84ff;--purple:#af52de;
  --text:#ffffff;--text2:rgba(235,235,240,.8);--text3:#8e8e93;
  --radius:12px;--shadow:0 2px 20px rgba(0,0,0,.6);
}
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{background:var(--bg);color:var(--text);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;font-size:16px;min-height:100dvh;padding-bottom:90px}

/* ── Header ── */
header{position:sticky;top:0;z-index:100;background:rgba(28,28,30,.92);border-bottom:1px solid var(--border);padding:0 16px;display:flex;align-items:center;justify-content:space-between;height:56px;backdrop-filter:blur(12px);-webkit-backdrop-filter:blur(12px)}
.hd-left{display:flex;align-items:center;gap:10px}
.hd-icon{width:30px;height:30px;background:var(--accent);border-radius:7px;display:flex;align-items:center;justify-content:center;font-size:15px;flex-shrink:0}
.hd-title{font-size:17px;font-weight:700;letter-spacing:-.3px}
.hd-sub{font-size:11px;color:var(--text3);margin-top:1px}
.badge{display:flex;align-items:center;gap:5px;font-size:12px;font-weight:600;color:var(--text3);background:var(--surface2);padding:5px 10px;border-radius:20px;border:1px solid var(--border)}
.dot{width:7px;height:7px;border-radius:50%;background:var(--green);animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.3}}

/* ── Layout ── */
main{padding:16px;display:flex;flex-direction:column;gap:16px}
.section-label{font-size:11px;font-weight:700;text-transform:uppercase;letter-spacing:.9px;color:var(--text3);padding:0 4px;margin-bottom:-4px}

/* ── Card ── */
.card{background:var(--surface);border-radius:var(--radius);border:1px solid var(--border);overflow:hidden}
.card-hd{padding:14px 16px 0;display:flex;align-items:center;gap:10px;margin-bottom:2px}
.card-icon{width:32px;height:32px;border-radius:8px;display:flex;align-items:center;justify-content:center;font-size:15px;flex-shrink:0}
.ci-red{background:rgba(227,28,37,.18)} .ci-amber{background:rgba(255,140,0,.18)}
.ci-green{background:rgba(52,199,89,.18)} .ci-blue{background:rgba(10,132,255,.18)}
.ci-purple{background:rgba(175,82,222,.18)}
.card-title{font-size:15px;font-weight:600}
.card-sub{font-size:12px;color:var(--text3);margin-top:1px}
.card-body{padding:10px 16px 16px}

/* ── Rows ── */
.row{display:flex;align-items:center;justify-content:space-between;padding:11px 0;gap:12px}
.row:not(:last-child){border-bottom:1px solid var(--border)}
.row-lbl{flex:1}.row-lbl span{display:block;font-size:15px}
.row-lbl small{color:var(--text3);font-size:12px}

/* ── Slider ── */
.slider-wrap{display:flex;align-items:center;gap:10px;width:100%;padding:6px 0}
.slider-wrap input[type=range]{flex:1;-webkit-appearance:none;appearance:none;height:5px;border-radius:3px;background:var(--surface2);outline:none;cursor:pointer}
.slider-wrap input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;border-radius:50%;background:#fff;cursor:pointer;box-shadow:0 1px 8px rgba(0,0,0,.5)}
.slider-wrap input[type=range]::-moz-range-thumb{width:24px;height:24px;border-radius:50%;border:none;background:#fff;cursor:pointer;box-shadow:0 1px 8px rgba(0,0,0,.5)}
.slider-val{min-width:52px;text-align:right;font-size:15px;font-variant-numeric:tabular-nums;color:var(--text2);font-weight:500}

/* ── Color picker ── */
.color-row{display:flex;flex-direction:column;gap:8px;padding:10px 0}
.color-row:not(:last-child){border-bottom:1px solid var(--border)}
.color-row-hd{display:flex;align-items:center;justify-content:space-between}
.color-row-lbl{font-size:15px;font-weight:500}
.color-row-sub{font-size:12px;color:var(--text3);margin-top:2px}
.color-pick-wrap{display:flex;align-items:center;gap:8px}
input[type=color]{-webkit-appearance:none;width:44px;height:36px;border:none;border-radius:8px;cursor:pointer;padding:0;background:none}
input[type=color]::-webkit-color-swatch-wrapper{padding:0;border-radius:8px;overflow:hidden}
input[type=color]::-webkit-color-swatch{border:none;border-radius:8px}
.hex-in{font-family:"SF Mono","Menlo",monospace;font-size:13px;color:var(--text2);background:var(--surface2);border:1px solid var(--border);border-radius:6px;padding:6px 10px;width:84px;outline:none}
.hex-in:focus{border-color:var(--accent)}

/* ── Select ── */
select{background:var(--surface2);color:var(--text);border:1px solid var(--border);border-radius:8px;padding:9px 32px 9px 12px;font-size:15px;outline:none;-webkit-appearance:none;appearance:none;cursor:pointer;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='7' viewBox='0 0 12 7'%3E%3Cpath d='M1 1l5 5 5-5' stroke='%238e8e93' stroke-width='1.5' fill='none' stroke-linecap='round' stroke-linejoin='round'/%3E%3C/svg%3E");background-repeat:no-repeat;background-position:right 12px center;width:100%}

/* ── Text input ── */
.txt-in{background:var(--surface2);color:var(--text);border:1px solid var(--border);border-radius:8px;padding:11px 12px;font-size:15px;outline:none;width:100%;-webkit-appearance:none}
.txt-in:focus{border-color:var(--accent)}

/* ── Field ── */
.field{display:flex;flex-direction:column;gap:6px;padding:10px 0}
.field:not(:last-child){border-bottom:1px solid var(--border)}
.field label{font-size:11px;font-weight:700;text-transform:uppercase;letter-spacing:.6px;color:var(--text3)}

/* ── WiFi section visibility ── */
.wifi-sec{display:none}.wifi-sec.active{display:block}

/* ── Preview grid ── */
.prev-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}
.prev-btn{padding:13px 8px;background:var(--surface2);border:1px solid var(--border);border-radius:10px;color:var(--text);font-size:14px;font-weight:500;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:7px;transition:background .12s,transform .1s;-webkit-appearance:none}
.prev-btn:active{background:var(--border);transform:scale(.96)}
.prev-dot{width:9px;height:9px;border-radius:50%;flex-shrink:0}

/* ── Action bar ── */
.action-bar{position:fixed;bottom:0;left:0;right:0;background:var(--surface);border-top:1px solid var(--border);padding:12px 16px;padding-bottom:calc(12px + env(safe-area-inset-bottom));display:flex;gap:10px;z-index:100;backdrop-filter:blur(12px);-webkit-backdrop-filter:blur(12px)}
.btn{flex:1;padding:14px 8px;border:none;border-radius:10px;font-size:15px;font-weight:600;cursor:pointer;transition:opacity .15s,transform .1s;-webkit-appearance:none;letter-spacing:-.2px}
.btn:active{opacity:.7;transform:scale(.97)}
.btn-primary{background:var(--accent);color:#fff}
.btn-secondary{background:var(--surface2);color:var(--text);border:1px solid var(--border)}
.btn-ghost{background:transparent;color:var(--text3);border:1px solid var(--border)}

/* ── Toast ── */
.toast{position:fixed;top:68px;left:50%;transform:translateX(-50%) translateY(-8px);background:var(--surface2);color:var(--text);padding:10px 20px;border-radius:20px;font-size:14px;font-weight:500;box-shadow:var(--shadow);opacity:0;pointer-events:none;transition:all .22s;white-space:nowrap;border:1px solid var(--border);z-index:200}
.toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
.toast.ok{border-color:var(--green);color:var(--green)}
.toast.err{border-color:var(--accent);color:var(--accent)}

/* ── Responsive ── */
@media(min-width:520px){main{max-width:520px;margin:0 auto}}
</style>
</head>
<body>

<!-- ── Header ─────────────────────────────────────────────────────────── -->
<header>
  <div class="hd-left">
    <div class="hd-icon">&#127950;</div>
    <div>
      <div class="hd-title">Foxbody Taillights</div>
      <div class="hd-sub" id="hd-ip">Connecting&hellip;</div>
    </div>
  </div>
  <div class="badge">
    <div class="dot"></div>
    <span id="hd-mode">AP</span>
  </div>
</header>

<main>

<!-- ── Brightness ─────────────────────────────────────────────────────── -->
<p class="section-label">Display</p>
<div class="card">
  <div class="card-hd">
    <div class="card-icon ci-red">&#9728;&#65039;</div>
    <div><div class="card-title">Brightness</div><div class="card-sub">LED intensity levels</div></div>
  </div>
  <div class="card-body">
    <div class="row">
      <div class="row-lbl"><span>Main Brightness</span><small>Brake, turn &amp; reverse states</small></div>
    </div>
    <div class="slider-wrap">
      <input type="range" id="brightness" min="10" max="255" step="1" value="128">
      <span class="slider-val" id="brightness-v">128</span>
    </div>
    <div style="height:1px;background:var(--border);margin:4px 0"></div>
    <div class="row" style="border:none">
      <div class="row-lbl"><span>Running Light Level</span><small>Dim glow at rest / parked</small></div>
    </div>
    <div class="slider-wrap">
      <input type="range" id="brightness_dim" min="5" max="100" step="1" value="40">
      <span class="slider-val" id="brightness_dim-v">40</span>
    </div>
  </div>
</div>

<!-- ── Colors ─────────────────────────────────────────────────────────── -->
<p class="section-label">Colors</p>
<div class="card">
  <div class="card-hd">
    <div class="card-icon ci-amber">&#127912;</div>
    <div><div class="card-title">Light Colors</div><div class="card-sub">Per-state color overrides</div></div>
  </div>
  <div class="card-body">
    <div class="color-row">
      <div class="color-row-hd">
        <div><div class="color-row-lbl">Brake</div><div class="color-row-sub">Solid brake-light color</div></div>
      </div>
      <div class="color-pick-wrap">
        <input type="color" id="brake_color" value="#ff0000">
        <input type="text"  class="hex-in"   id="brake_hex"   value="#ff0000" maxlength="7" spellcheck="false">
      </div>
    </div>
    <div class="color-row">
      <div class="color-row-hd">
        <div><div class="color-row-lbl">Turn Signal</div><div class="color-row-sub">Sequential sweep &amp; hazard flash</div></div>
      </div>
      <div class="color-pick-wrap">
        <input type="color" id="turn_color" value="#ff6400">
        <input type="text"  class="hex-in"  id="turn_hex"   value="#ff6400" maxlength="7" spellcheck="false">
      </div>
    </div>
    <div class="color-row" style="border:none;padding-bottom:0">
      <div class="color-row-hd">
        <div><div class="color-row-lbl">Reverse</div><div class="color-row-sub">Backup-light color</div></div>
      </div>
      <div class="color-pick-wrap">
        <input type="color" id="reverse_color" value="#ffffff">
        <input type="text"  class="hex-in"     id="reverse_hex"   value="#ffffff" maxlength="7" spellcheck="false">
      </div>
    </div>
    <div style="height:1px;background:var(--border);margin:10px 0 6px"></div>
    <div class="section-label" style="padding:0;margin-bottom:10px">Quick Presets</div>
    <div style="display:flex;gap:6px;flex-wrap:wrap">
      <button class="prev-btn" style="flex:none;padding:9px 14px;font-size:13px" onclick="applyColorPreset('stock')"><span class="prev-dot" style="background:#ff0000"></span>Stock</button>
      <button class="prev-btn" style="flex:none;padding:9px 14px;font-size:13px" onclick="applyColorPreset('cool')"><span class="prev-dot" style="background:#0055ff"></span>Cool</button>
      <button class="prev-btn" style="flex:none;padding:9px 14px;font-size:13px" onclick="applyColorPreset('purple')"><span class="prev-dot" style="background:#aa00ff"></span>Purple</button>
      <button class="prev-btn" style="flex:none;padding:9px 14px;font-size:13px" onclick="applyColorPreset('sport')"><span class="prev-dot" style="background:#ffd700"></span>Sport</button>
      <button class="prev-btn" style="flex:none;padding:9px 14px;font-size:13px" onclick="applyColorPreset('murdered')"><span class="prev-dot" style="background:#550000"></span>Murdered</button>
    </div>
  </div>
</div>

<!-- ── Lens Preset ─────────────────────────────────────────────────────── -->
<p class="section-label">Hardware</p>
<div class="card">
  <div class="card-hd">
    <div class="card-icon ci-purple">&#128663;</div>
    <div><div class="card-title">Lens Preset</div><div class="card-sub">LED zone layout matching your taillight housing</div></div>
  </div>
  <div class="card-body">
    <div class="field" style="border:none;padding-bottom:0">
      <label>Taillight Style</label>
      <select id="lens_preset" onchange="updateLensDesc()">
        <option value="0">Full Panel &mdash; all 256 LEDs active (custom / generic)</option>
        <option value="1">GT Cheese Grater &mdash; 3-section chrome trim (87&ndash;93 Mustang GT)</option>
        <option value="2">LX / Base &mdash; 2-section clean lens (87&ndash;93 Mustang LX)</option>
        <option value="3">Cobra Bar &mdash; horizontal center-bar style (custom / Cobra)</option>
      </select>
      <small id="lens_desc" style="color:var(--text3);font-size:12px;margin-top:8px;display:block;line-height:1.5"></small>
    </div>
  </div>
</div>

<!-- ── Animation Styles ───────────────────────────────────────────────── -->
<p class="section-label">Animations</p>
<div class="card">
  <div class="card-hd">
    <div class="card-icon ci-amber">&#10024;</div>
    <div><div class="card-title">Animation Styles</div><div class="card-sub">How each light state is displayed on the LEDs</div></div>
  </div>
  <div class="card-body">
    <div class="field">
      <label>Brake Animation</label>
      <select id="brake_anim">
        <option value="0">Solid Fill &mdash; instant-on, stays lit</option>
        <option value="1">Pulse / Breathe &mdash; gently fades in and out</option>
        <option value="2">Center-Out Fill &mdash; expands from center then holds</option>
        <option value="3">Strobe Flash &mdash; rapid 8 Hz attention strobe</option>
      </select>
    </div>
    <div class="field">
      <label>Turn Signal Animation</label>
      <select id="turn_anim">
        <option value="0">Sequential Sweep &mdash; classic column-by-column sweep</option>
        <option value="1">Simple Flash &mdash; whole panel blinks on / off</option>
        <option value="2">Group Chase &mdash; 4-column groups sweep in sequence</option>
        <option value="3">Bounce Sweep &mdash; Knight Rider beam bounces across</option>
      </select>
    </div>
    <div class="field">
      <label>Reverse Animation</label>
      <select id="reverse_anim">
        <option value="0">Solid White &mdash; instant-on, stays lit</option>
        <option value="1">Pulse / Breathe &mdash; gently fades in and out</option>
      </select>
    </div>
    <div class="field" style="border:none;padding-bottom:0">
      <label>Running Light Animation</label>
      <select id="run_anim">
        <option value="0">Dim Solid &mdash; constant dim glow at rest</option>
        <option value="1">Breathe &mdash; slow pulse dim glow</option>
      </select>
    </div>
  </div>
</div>

<!-- ── Timing ─────────────────────────────────────────────────────────── -->
<p class="section-label">Timing</p>
<div class="card">
  <div class="card-hd">
    <div class="card-icon ci-amber">&#9201;</div>
    <div><div class="card-title">Animation Timing</div><div class="card-sub">Blink speed &amp; frame rate</div></div>
  </div>
  <div class="card-body">
    <div class="row">
      <div class="row-lbl"><span>Turn Blink Period</span><small>Full on+off cycle time</small></div>
    </div>
    <div class="slider-wrap">
      <input type="range" id="turn_blink_ms" min="200" max="1500" step="50" value="600">
      <span class="slider-val" id="turn_blink_ms-v">600 ms</span>
    </div>
    <div style="height:1px;background:var(--border);margin:4px 0"></div>
    <div class="row" style="border:none">
      <div class="row-lbl"><span>Frame Interval</span><small>Animation update rate</small></div>
    </div>
    <div class="slider-wrap">
      <input type="range" id="frame_ms" min="10" max="100" step="5" value="20">
      <span class="slider-val" id="frame_ms-v">20 ms</span>
    </div>
  </div>
</div>

<!-- ── Preview ────────────────────────────────────────────────────────── -->
<p class="section-label">Preview</p>
<div class="card">
  <div class="card-hd">
    <div class="card-icon ci-green">&#9654;&#65039;</div>
    <div><div class="card-title">Test Animations</div><div class="card-sub">Live preview for 3 seconds (uses current colors)</div></div>
  </div>
  <div class="card-body" style="padding-top:14px">
    <div class="prev-grid">
      <button class="prev-btn" onclick="preview('brake')">
        <span class="prev-dot" style="background:#ff0000"></span>Brake
      </button>
      <button class="prev-btn" onclick="preview('left_turn')">
        <span class="prev-dot" style="background:#ff8c00"></span>Left Turn
      </button>
      <button class="prev-btn" onclick="preview('right_turn')">
        <span class="prev-dot" style="background:#ff8c00"></span>Right Turn
      </button>
      <button class="prev-btn" onclick="preview('hazard')">
        <span class="prev-dot" style="background:#ff8c00"></span>Hazard
      </button>
      <button class="prev-btn" onclick="preview('reverse')">
        <span class="prev-dot" style="background:#eeeeee"></span>Reverse
      </button>
      <button class="prev-btn" onclick="preview('off')">
        <span class="prev-dot" style="background:#3a3a3c"></span>Off
      </button>
    </div>
  </div>
</div>

<!-- ── WiFi ───────────────────────────────────────────────────────────── -->
<p class="section-label">Network</p>
<div class="card">
  <div class="card-hd">
    <div class="card-icon ci-blue">&#128246;</div>
    <div><div class="card-title">WiFi Configuration</div><div class="card-sub">Access point &amp; station settings</div></div>
  </div>
  <div class="card-body">
    <div class="field">
      <label>Mode</label>
      <select id="wifi_mode" onchange="updateWifiSections()">
        <option value="0">Access Point (AP) &mdash; device creates its own hotspot</option>
        <option value="1">Station (STA) &mdash; connect to existing WiFi</option>
      </select>
    </div>
    <!-- AP settings -->
    <div id="ap-sec" class="wifi-sec active">
      <div class="field">
        <label>AP Network Name (SSID)</label>
        <input type="text" class="txt-in" id="ap_ssid" placeholder="Foxbody-Taillights" autocomplete="off">
      </div>
      <div class="field" style="border:none;padding-bottom:0">
        <label>AP Password <span style="font-weight:400;text-transform:none">(min 8 chars)</span></label>
        <input type="password" class="txt-in" id="ap_pass" placeholder="Enter password" autocomplete="new-password">
      </div>
    </div>
    <!-- Station settings -->
    <div id="sta-sec" class="wifi-sec">
      <div class="field">
        <label>Home WiFi SSID</label>
        <input type="text" class="txt-in" id="sta_ssid" placeholder="Your network name" autocomplete="off">
      </div>
      <div class="field" style="border:none;padding-bottom:0">
        <label>Home WiFi Password</label>
        <input type="password" class="txt-in" id="sta_pass" placeholder="Your network password" autocomplete="current-password">
      </div>
    </div>
  </div>
</div>

<!-- ── Device info ────────────────────────────────────────────────────── -->
<div class="card">
  <div class="card-hd">
    <div class="card-icon ci-purple">&#8505;&#65039;</div>
    <div><div class="card-title">Device Info</div><div class="card-sub">ESP32-S3 &middot; FastLED &middot; 2&times;256 px WS2812B</div></div>
  </div>
  <div class="card-body">
    <div class="row"><span>IP Address</span><span id="info-ip" style="color:var(--text3)">&#8211;</span></div>
    <div class="row"><span>WiFi Mode</span><span id="info-mode" style="color:var(--text3)">&#8211;</span></div>
    <div class="row" style="border:none"><span>Uptime</span><span id="info-uptime" style="color:var(--text3)">&#8211;</span></div>
  </div>
</div>

</main>

<!-- ── Action bar ─────────────────────────────────────────────────────── -->
<div class="action-bar">
  <button class="btn btn-ghost"      onclick="resetDefaults()">Reset</button>
  <button class="btn btn-secondary"  onclick="rebootDevice()">Reboot</button>
  <button class="btn btn-primary"    id="save-btn" onclick="saveSettings()">Save</button>
</div>

<!-- ── Toast notification ─────────────────────────────────────────────── -->
<div class="toast" id="toast"></div>

<script>
/* ── Toast ─────────────────────────────────────────────────────────────── */
function toast(msg, type) {
  var t = document.getElementById('toast');
  t.textContent = msg;
  t.className = 'toast show' + (type ? ' ' + type : '');
  clearTimeout(t._t);
  t._t = setTimeout(function(){ t.className = 'toast'; }, 2600);
}

/* ── Slider bindings ───────────────────────────────────────────────────── */
function bindSlider(id, suffix) {
  var el = document.getElementById(id);
  var vl = document.getElementById(id + '-v');
  el.addEventListener('input', function(){ vl.textContent = el.value + (suffix||''); });
}
bindSlider('brightness');
bindSlider('brightness_dim');
bindSlider('turn_blink_ms', ' ms');
bindSlider('frame_ms', ' ms');

/* ── Color picker bindings ─────────────────────────────────────────────── */
function bindColor(pid, hid) {
  var p = document.getElementById(pid);
  var h = document.getElementById(hid);
  p.addEventListener('input', function(){ h.value = p.value; });
  h.addEventListener('input', function(){
    if (/^#[0-9a-fA-F]{6}$/.test(h.value)) p.value = h.value;
  });
}
bindColor('brake_color',   'brake_hex');
bindColor('turn_color',    'turn_hex');
bindColor('reverse_color', 'reverse_hex');

/* ── WiFi section toggle ───────────────────────────────────────────────── */
function updateWifiSections() {
  var m = document.getElementById('wifi_mode').value;
  document.getElementById('ap-sec').className  = 'wifi-sec' + (m==='0' ? ' active' : '');
  document.getElementById('sta-sec').className = 'wifi-sec' + (m==='1' ? ' active' : '');
}

/* ── Lens preset description ───────────────────────────────────────────── */
var LENS_DESCS = [
  'All 256 LEDs active — use for custom or universal builds.',
  '3-section chrome trim layout: outer (cols 0\u20139) \u2022 center (cols 12\u201320) \u2022 inner (cols 23\u201331). Classic 87\u201393 Mustang GT look.',
  '2-section clean-lens layout: wide outer (cols 0\u201313) and inner (cols 18\u201331) panels. Matches the 87\u201393 Mustang LX / notchback.',
  'Center 4-row horizontal bar — illuminates rows 2\u20135 only. Great for Cobra or custom builds with a "bar" taillight appearance.'
];
function updateLensDesc() {
  var v = +document.getElementById('lens_preset').value;
  document.getElementById('lens_desc').textContent = LENS_DESCS[v] || '';
}
updateLensDesc();
document.getElementById('lens_preset').addEventListener('change', updateLensDesc);

/* ── Color presets ─────────────────────────────────────────────────────── */
var COLOR_PRESETS = {
  stock:    { brake:[255,0,0],     turn:[255,100,0],  reverse:[255,255,255] },
  cool:     { brake:[0,80,255],    turn:[0,180,255],  reverse:[200,220,255] },
  purple:   { brake:[180,0,255],   turn:[130,0,200],  reverse:[200,200,255] },
  sport:    { brake:[255,30,0],    turn:[255,200,0],  reverse:[255,255,180] },
  murdered: { brake:[120,0,0],     turn:[140,50,0],   reverse:[70,70,70]   }
};
function applyColorPreset(name) {
  var p = COLOR_PRESETS[name];
  if (!p) return;
  setColor('brake_color',   'brake_hex',   p.brake[0],   p.brake[1],   p.brake[2]);
  setColor('turn_color',    'turn_hex',    p.turn[0],    p.turn[1],    p.turn[2]);
  setColor('reverse_color', 'reverse_hex', p.reverse[0], p.reverse[1], p.reverse[2]);
  toast('Preset applied \u2014 tap Save to keep it');
}

/* ── Helpers ───────────────────────────────────────────────────────────── */
function rgb2hex(r, g, b) {
  return '#' + [r,g,b].map(function(v){ return ('0'+v.toString(16)).slice(-2); }).join('');
}
function hex2rgb(hex) {
  return { r: parseInt(hex.slice(1,3),16), g: parseInt(hex.slice(3,5),16), b: parseInt(hex.slice(5,7),16) };
}
function setSlider(id, val, suffix) {
  if (val == null) return;
  var el = document.getElementById(id);
  var vl = document.getElementById(id + '-v');
  el.value = val;
  if (vl) vl.textContent = val + (suffix||'');
}
function setColor(pid, hid, r, g, b) {
  if (r == null) return;
  var hex = rgb2hex(r, g, b);
  document.getElementById(pid).value = hex;
  document.getElementById(hid).value = hex;
}
function fmtUptime(s) {
  var h = Math.floor(s/3600), m = Math.floor((s%3600)/60), sec = s%60;
  if (h > 0) return h + 'h ' + m + 'm';
  if (m > 0) return m + 'm ' + sec + 's';
  return sec + 's';
}

/* ── Load settings ─────────────────────────────────────────────────────── */
function loadSettings() {
  fetch('/api/settings')
    .then(function(r){ return r.json(); })
    .then(function(s){
      setSlider('brightness',     s.brightness);
      setSlider('brightness_dim', s.brightness_dim);
      setSlider('turn_blink_ms',  s.turn_blink_ms, ' ms');
      setSlider('frame_ms',       s.frame_ms,       ' ms');
      setColor('brake_color',   'brake_hex',   s.brake_r,   s.brake_g,   s.brake_b);
      setColor('turn_color',    'turn_hex',    s.turn_r,    s.turn_g,    s.turn_b);
      setColor('reverse_color', 'reverse_hex', s.reverse_r, s.reverse_g, s.reverse_b);
      if (s.brake_anim   != null) document.getElementById('brake_anim').value   = s.brake_anim;
      if (s.turn_anim    != null) document.getElementById('turn_anim').value    = s.turn_anim;
      if (s.reverse_anim != null) document.getElementById('reverse_anim').value = s.reverse_anim;
      if (s.run_anim     != null) document.getElementById('run_anim').value     = s.run_anim;
      if (s.lens_preset  != null) {
        document.getElementById('lens_preset').value = s.lens_preset;
        updateLensDesc();
      }
      document.getElementById('wifi_mode').value = s.wifi_mode;
      document.getElementById('ap_ssid').value   = s.ap_ssid  || '';
      document.getElementById('ap_pass').value   = '';
      document.getElementById('sta_ssid').value  = s.sta_ssid || '';
      document.getElementById('sta_pass').value  = '';
      updateWifiSections();
      var ip   = s.ip   || '--';
      var mode = s.wifi_mode === 0 ? 'Access Point' : 'Station';
      document.getElementById('hd-ip').textContent     = ip;
      document.getElementById('hd-mode').textContent   = s.wifi_mode === 0 ? 'AP' : 'STA';
      document.getElementById('info-ip').textContent   = ip;
      document.getElementById('info-mode').textContent = mode;
      document.getElementById('info-uptime').textContent = fmtUptime(s.uptime_s || 0);
    })
    .catch(function(){ toast('Could not reach device', 'err'); });
}

/* ── Collect form values ───────────────────────────────────────────────── */
function collectSettings() {
  var br  = hex2rgb(document.getElementById('brake_color').value);
  var tu  = hex2rgb(document.getElementById('turn_color').value);
  var rv  = hex2rgb(document.getElementById('reverse_color').value);
  var obj = {
    brightness:     +document.getElementById('brightness').value,
    brightness_dim: +document.getElementById('brightness_dim').value,
    turn_blink_ms:  +document.getElementById('turn_blink_ms').value,
    frame_ms:       +document.getElementById('frame_ms').value,
    brake_r: br.r, brake_g: br.g, brake_b: br.b,
    turn_r:  tu.r, turn_g:  tu.g, turn_b:  tu.b,
    reverse_r: rv.r, reverse_g: rv.g, reverse_b: rv.b,
    brake_anim:   +document.getElementById('brake_anim').value,
    turn_anim:    +document.getElementById('turn_anim').value,
    reverse_anim: +document.getElementById('reverse_anim').value,
    run_anim:     +document.getElementById('run_anim').value,
    lens_preset:  +document.getElementById('lens_preset').value,
    wifi_mode: +document.getElementById('wifi_mode').value,
    ap_ssid:  document.getElementById('ap_ssid').value,
    ap_pass:  document.getElementById('ap_pass').value,
    sta_ssid: document.getElementById('sta_ssid').value,
    sta_pass: document.getElementById('sta_pass').value
  };
  return obj;
}

/* ── Save settings ─────────────────────────────────────────────────────── */
function saveSettings() {
  var btn = document.getElementById('save-btn');
  btn.textContent = 'Saving\u2026';
  fetch('/api/settings', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(collectSettings())
  })
  .then(function(r){
    if (!r.ok) throw new Error('HTTP ' + r.status);
    toast('Settings saved \u2713', 'ok');
    loadSettings();
  })
  .catch(function(e){ toast('Save failed: ' + e.message, 'err'); })
  .finally(function(){ btn.textContent = 'Save'; });
}

/* ── Reset defaults ────────────────────────────────────────────────────── */
function resetDefaults() {
  if (!confirm('Reset all settings to factory defaults?')) return;
  fetch('/api/reset', { method: 'POST' })
    .then(function(r){
      if (!r.ok) throw new Error('HTTP ' + r.status);
      toast('Reset to defaults \u2713', 'ok');
      loadSettings();
    })
    .catch(function(){ toast('Reset failed', 'err'); });
}

/* ── Reboot ────────────────────────────────────────────────────────────── */
function rebootDevice() {
  if (!confirm('Reboot the device now?')) return;
  fetch('/api/reboot', { method: 'POST' })
    .then(function(){ toast('Rebooting\u2026'); })
    .catch(function(){ toast('Rebooting\u2026'); });
}

/* ── Preview ───────────────────────────────────────────────────────────── */
function preview(state) {
  fetch('/api/preview', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ state: state })
  })
  .then(function(r){
    if (!r.ok) throw new Error('HTTP ' + r.status);
    toast('Previewing: ' + state.replace('_', ' '));
  })
  .catch(function(e){ toast('Preview failed: ' + e.message, 'err'); });
}

/* ── Boot ──────────────────────────────────────────────────────────────── */
loadSettings();
setInterval(loadSettings, 15000);
</script>
</body>
</html>
)html";

// ---------------------------------------------------------------------------
// Helper: set CORS headers so the page works when accessed from any origin
// during development.
// ---------------------------------------------------------------------------
static void addCorsHeaders() {
    _server.sendHeader("Access-Control-Allow-Origin",  "*");
    _server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// ---------------------------------------------------------------------------
// GET /
// ---------------------------------------------------------------------------
static void handleRoot() {
    _server.send_P(200, "text/html", INDEX_HTML);
}

// ---------------------------------------------------------------------------
// GET /api/settings
// Returns all current settings + live status as JSON.
// Passwords are intentionally omitted from the response for security.
// ---------------------------------------------------------------------------
static void handleGetSettings() {
    JsonDocument doc;

    doc["brightness"]     = g_settings.brightness;
    doc["brightness_dim"] = g_settings.brightness_dim;
    doc["turn_blink_ms"]  = g_settings.turn_blink_ms;
    doc["frame_ms"]       = g_settings.frame_ms;

    doc["brake_r"]   = g_settings.brake_r;
    doc["brake_g"]   = g_settings.brake_g;
    doc["brake_b"]   = g_settings.brake_b;

    doc["turn_r"]    = g_settings.turn_r;
    doc["turn_g"]    = g_settings.turn_g;
    doc["turn_b"]    = g_settings.turn_b;

    doc["reverse_r"] = g_settings.reverse_r;
    doc["reverse_g"] = g_settings.reverse_g;
    doc["reverse_b"] = g_settings.reverse_b;

    doc["brake_anim"]   = g_settings.brake_anim;
    doc["turn_anim"]    = g_settings.turn_anim;
    doc["reverse_anim"] = g_settings.reverse_anim;
    doc["run_anim"]     = g_settings.run_anim;
    doc["lens_preset"]  = g_settings.lens_preset;

    doc["wifi_mode"] = g_settings.wifi_mode;
    doc["ap_ssid"]   = g_settings.ap_ssid;
    doc["ap_pass"]   = "";          // never echo passwords
    doc["sta_ssid"]  = g_settings.sta_ssid;
    doc["sta_pass"]  = "";

    doc["uptime_s"]  = millis() / 1000UL;
    doc["ip"]        = (g_settings.wifi_mode == 0)
                       ? WiFi.softAPIP().toString()
                       : WiFi.localIP().toString();

    String out;
    serializeJson(doc, out);
    addCorsHeaders();
    _server.send(200, "application/json", out);
}

// ---------------------------------------------------------------------------
// POST /api/settings
// Accepts a JSON body, updates g_settings, persists to NVS, applies
// brightness immediately.  Only fields present in the body are changed.
// ---------------------------------------------------------------------------
static void handlePostSettings() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Empty body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"Bad JSON\"}");
        return;
    }

    // ── Numeric settings ────────────────────────────────────────────────────
    if (doc["brightness"].is<int>())
        g_settings.brightness     = (uint8_t)constrain(doc["brightness"].as<int>(), 10, 255);
    if (doc["brightness_dim"].is<int>())
        g_settings.brightness_dim = (uint8_t)constrain(doc["brightness_dim"].as<int>(), 5, 100);
    if (doc["turn_blink_ms"].is<int>())
        g_settings.turn_blink_ms  = (uint16_t)constrain(doc["turn_blink_ms"].as<int>(), 200, 1500);
    if (doc["frame_ms"].is<int>())
        g_settings.frame_ms       = (uint8_t)constrain(doc["frame_ms"].as<int>(), 10, 100);

    if (doc["brake_r"].is<int>()) g_settings.brake_r = (uint8_t)constrain(doc["brake_r"].as<int>(), 0, 255);
    if (doc["brake_g"].is<int>()) g_settings.brake_g = (uint8_t)constrain(doc["brake_g"].as<int>(), 0, 255);
    if (doc["brake_b"].is<int>()) g_settings.brake_b = (uint8_t)constrain(doc["brake_b"].as<int>(), 0, 255);

    if (doc["turn_r"].is<int>())  g_settings.turn_r  = (uint8_t)constrain(doc["turn_r"].as<int>(),  0, 255);
    if (doc["turn_g"].is<int>())  g_settings.turn_g  = (uint8_t)constrain(doc["turn_g"].as<int>(),  0, 255);
    if (doc["turn_b"].is<int>())  g_settings.turn_b  = (uint8_t)constrain(doc["turn_b"].as<int>(),  0, 255);

    if (doc["reverse_r"].is<int>()) g_settings.reverse_r = (uint8_t)constrain(doc["reverse_r"].as<int>(), 0, 255);
    if (doc["reverse_g"].is<int>()) g_settings.reverse_g = (uint8_t)constrain(doc["reverse_g"].as<int>(), 0, 255);
    if (doc["reverse_b"].is<int>()) g_settings.reverse_b = (uint8_t)constrain(doc["reverse_b"].as<int>(), 0, 255);

    if (doc["brake_anim"].is<int>())
        g_settings.brake_anim   = (uint8_t)constrain(doc["brake_anim"].as<int>(),   0, 3);
    if (doc["turn_anim"].is<int>())
        g_settings.turn_anim    = (uint8_t)constrain(doc["turn_anim"].as<int>(),    0, 3);
    if (doc["reverse_anim"].is<int>())
        g_settings.reverse_anim = (uint8_t)constrain(doc["reverse_anim"].as<int>(), 0, 1);
    if (doc["run_anim"].is<int>())
        g_settings.run_anim     = (uint8_t)constrain(doc["run_anim"].as<int>(),     0, 1);
    if (doc["lens_preset"].is<int>())
        g_settings.lens_preset  = (uint8_t)constrain(doc["lens_preset"].as<int>(),  0, 3);

    if (doc["wifi_mode"].is<int>())
        g_settings.wifi_mode = (uint8_t)constrain(doc["wifi_mode"].as<int>(), 0, 1);

    // ── String settings (guarded against buffer overrun) ────────────────────
    auto copyStr = [](const char* src, char* dst, size_t dstSize) {
        if (src && src[0] != '\0') {
            strncpy(dst, src, dstSize - 1);
            dst[dstSize - 1] = '\0';
        }
    };

    const char* ap_ssid = doc["ap_ssid"].as<const char*>();
    copyStr(ap_ssid, g_settings.ap_ssid, sizeof(g_settings.ap_ssid));

    // Only update password if non-empty (empty means "leave unchanged").
    // AP passwords require a WPA2 minimum of 8 characters.
    const char* ap_pass = doc["ap_pass"].as<const char*>();
    if (ap_pass && strlen(ap_pass) >= 8)
        copyStr(ap_pass, g_settings.ap_pass, sizeof(g_settings.ap_pass));

    const char* sta_ssid = doc["sta_ssid"].as<const char*>();
    copyStr(sta_ssid, g_settings.sta_ssid, sizeof(g_settings.sta_ssid));

    // Station passwords have no enforced minimum (external networks vary);
    // an empty string means "leave unchanged".
    const char* sta_pass = doc["sta_pass"].as<const char*>();
    if (sta_pass && strlen(sta_pass) > 0)
        copyStr(sta_pass, g_settings.sta_pass, sizeof(g_settings.sta_pass));

    // Apply brightness immediately without waiting for a reboot
    FastLED.setBrightness(g_settings.brightness);

    settings_save();
    addCorsHeaders();
    _server.send(200, "application/json", "{\"ok\":true}");
}

// ---------------------------------------------------------------------------
// POST /api/reset
// Restores compiled-in defaults, persists, re-applies brightness.
// ---------------------------------------------------------------------------
static void handleReset() {
    settings_reset();
    FastLED.setBrightness(g_settings.brightness);
    addCorsHeaders();
    _server.send(200, "application/json", "{\"ok\":true}");
}

// ---------------------------------------------------------------------------
// POST /api/reboot
// Sends the response first, then restarts the ESP32.
// ---------------------------------------------------------------------------
static void handleReboot() {
    addCorsHeaders();
    _server.send(200, "application/json", "{\"ok\":true}");
    _server.client().stop();
    delay(200);
    ESP.restart();
}

// ---------------------------------------------------------------------------
// POST /api/preview
// Body: { "state": "brake" | "left_turn" | "right_turn" | "reverse" |
//                  "hazard" | "brake_left" | "brake_right" | "off" }
// Overrides the light state for 3 seconds so the user can preview animations
// with the current color settings without triggering physical inputs.
// ---------------------------------------------------------------------------
static void handlePreview() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Empty body\"}");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, _server.arg("plain"))) {
        _server.send(400, "application/json", "{\"error\":\"Bad JSON\"}");
        return;
    }

    const char* s = doc["state"].as<const char*>();
    if (!s) {
        _server.send(400, "application/json", "{\"error\":\"Missing state\"}");
        return;
    }

    LightState ls = LightState::OFF;
    if      (strcmp(s, "brake")       == 0) ls = LightState::BRAKE;
    else if (strcmp(s, "left_turn")   == 0) ls = LightState::LEFT_TURN;
    else if (strcmp(s, "right_turn")  == 0) ls = LightState::RIGHT_TURN;
    else if (strcmp(s, "reverse")     == 0) ls = LightState::REVERSE;
    else if (strcmp(s, "hazard")      == 0) ls = LightState::HAZARD;
    else if (strcmp(s, "brake_left")  == 0) ls = LightState::BRAKE_LEFT;
    else if (strcmp(s, "brake_right") == 0) ls = LightState::BRAKE_RIGHT;

    g_preview_state    = ls;
    g_preview_until_ms = millis() + 3000UL;

    addCorsHeaders();
    _server.send(200, "application/json", "{\"ok\":true}");
}

// ---------------------------------------------------------------------------
// Global WifiServer instance
// ---------------------------------------------------------------------------
WifiServer wifiServer;

// ---------------------------------------------------------------------------
void WifiServer::begin() {
    Serial.println(F("[wifi] starting..."));

    if (g_settings.wifi_mode == 0) {
        // ── Access Point mode ────────────────────────────────────────────────
        WiFi.mode(WIFI_AP);
        WiFi.softAP(g_settings.ap_ssid, g_settings.ap_pass);
        Serial.print(F("[wifi] AP \""));
        Serial.print(g_settings.ap_ssid);
        Serial.print(F("\"  IP: "));
        Serial.println(WiFi.softAPIP());
    } else {
        // ── Station mode — connect with 10-second timeout ────────────────────
        WiFi.mode(WIFI_STA);
        WiFi.begin(g_settings.sta_ssid, g_settings.sta_pass);
        Serial.print(F("[wifi] connecting to \""));
        Serial.print(g_settings.sta_ssid);
        Serial.print('"');

        uint8_t tries = 0;
        while (WiFi.status() != WL_CONNECTED && tries < 20) {
            delay(500);
            Serial.print('.');
            tries++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.print(F("\n[wifi] IP: "));
            Serial.println(WiFi.localIP());
        } else {
            // Fall back to AP so the device is always reachable
            Serial.println(F("\n[wifi] connection failed — falling back to AP"));
            WiFi.mode(WIFI_AP);
            WiFi.softAP(g_settings.ap_ssid, g_settings.ap_pass);
            Serial.print(F("[wifi] AP fallback IP: "));
            Serial.println(WiFi.softAPIP());
        }
    }

    // ── Register HTTP routes ─────────────────────────────────────────────────
    _server.on("/",               HTTP_GET,  handleRoot);
    _server.on("/api/settings",   HTTP_GET,  handleGetSettings);
    _server.on("/api/settings",   HTTP_POST, handlePostSettings);
    _server.on("/api/reset",      HTTP_POST, handleReset);
    _server.on("/api/reboot",     HTTP_POST, handleReboot);
    _server.on("/api/preview",    HTTP_POST, handlePreview);

    // Preflight OPTIONS for all routes (browser CORS pre-check)
    _server.onNotFound([]() {
        if (_server.method() == HTTP_OPTIONS) {
            addCorsHeaders();
            _server.send(204);
        } else {
            _server.send(404, "text/plain", "Not found");
        }
    });

    _server.begin(80);
    Serial.println(F("[wifi] HTTP server ready on port 80"));
}

// ---------------------------------------------------------------------------
void WifiServer::handle() {
    _server.handleClient();
}
