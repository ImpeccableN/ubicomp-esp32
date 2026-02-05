#ifndef UI_H
#define UI_H

const char INDEX_HTML[] = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Tone Gen</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <h1>ESP32 Chladni Generator</h1>
    
    <div class="tabs">
        <button class="tab-btn active" onclick="openTab('presets')">Presets (Manual)</button>
        <button class="tab-btn" onclick="openTab('mic')">Microphone</button>
    </div>

    <!-- PRESETS TAB -->
    <div id="presets" class="tab-content active">
        <button id="manualBtn" class="btn start">START MANUAL MODE</button>

        <div class="group">
            <h3>Frequency Presets</h3>
            
            <div class="preset-card">
                <img src="/img/low.jpg" alt="Low Freq">
                <button class="btn-preset" onclick="setFreq(440)">Low (440 Hz)</button>
            </div>

            <div class="preset-card">
                <img src="/img/mid.jpg" alt="Mid Freq">
                <button class="btn-preset" onclick="setFreq(880)">Mid (880 Hz)</button>
            </div>

            <div class="preset-card">
                <img src="/img/high.jpg" alt="High Freq">
                <button class="btn-preset" onclick="setFreq(1500)">High (1500 Hz)</button>
            </div>
        </div>

        <div class="group">
            <h3>Manual Frequency: <span id="freqVal">440</span> Hz</h3>
            <input type="range" id="freqSlider" min="0" max="2000" value="100">
        </div>
        
        <div class="group">
            <h3>Volume: <span id="volVal">10</span>%</h3>
            <input type="range" id="volSlider" min="0" max="100" value="10">
        </div>
    </div>

    <!-- MIC TAB -->
    <div id="mic" class="tab-content">
        <div class="group">
            <h3>Microphone Control</h3>
            <p>Control frequency with voice pitch/volume.</p>
            <button id="micBtn" class="btn start">START MIC MODE</button>
        </div>

        <div class="group">
            <h3>Volume History (10s)</h3>
            <canvas id="volGraph" width="600" height="200" style="width:100%; height:200px; border-radius:10px; background:#fafafa;"></canvas>
        </div>
    </div>

    <script src="script.js"></script>
</body>
</html>
)=====";

const char STYLE_CSS[] = R"=====(
body { 
    font-family: 'Segoe UI', sans-serif; 
    text-align: center; 
    background: #fdfdfd; 
    color: #333; 
    padding: 20px; 
    max-width: 600px; 
    margin: 0 auto; 
}
h1 { color: #222; }
img { width: 150px; height: 150px; border-radius: 8px; margin-bottom: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }

/* Tabs */
.tabs { display: flex; justify-content: center; margin-bottom: 20px; }
.tab-btn {
    background: #eee; border: none; padding: 10px 20px; 
    cursor: pointer; font-size: 16px; margin: 0 5px; border-radius: 8px;
    font-weight: bold; color: #777;
}
.tab-btn.active { background: #3498db; color: white; }
.tab-content { display: none; animation: fadeEffect 0.5s; }
.tab-content.active { display: block; }

@keyframes fadeEffect { from {opacity: 0;} to {opacity: 1;} }

/* Buttons */
.btn { 
    padding: 15px; width: 100%; font-size: 18px; 
    border-radius: 12px; border: none; cursor: pointer; 
    font-weight: bold; margin-bottom: 20px;
    box-shadow: 0 4px 6px rgba(0,0,0,0.1);
}
.start { background: #2ecc71; color: white; }
.stop { background: #e74c3c; color: white; }

.preset-card {
    display: inline-block;
    margin: 10px;
    padding: 10px;
    background: #f9f9f9;
    border-radius: 10px;
}
.btn-preset {
    display: block; width: 100%; padding: 8px;
    background: #34495e; color: white; border: none;
    border-radius: 5px; cursor: pointer;
}

.group { 
    margin: 20px 0; background: #ffffff; padding: 15px;
    border-radius: 15px; box-shadow: 0 2px 10px rgba(0,0,0,0.05);
}
input[type=range] { width: 90%; accent-color: #3498db; cursor: pointer; }
)=====";

const char SCRIPT_JS[] = R"=====(
let playing = false;
let micActive = false;
let lastSend = 0;

const manualBtn = document.getElementById('manualBtn');
const micBtn = document.getElementById('micBtn');
const freqSlider = document.getElementById('freqSlider');
const volSlider = document.getElementById('volSlider');
const freqVal = document.getElementById('freqVal');
const volVal = document.getElementById('volVal');

// Reset everything to stop
function stopAll() {
    playing = false;
    micActive = false;
    
    manualBtn.innerText = 'START MANUAL MODE';
    manualBtn.className = 'btn start';
    
    micBtn.innerText = 'START MIC MODE';
    micBtn.className = 'btn start';
    
    fetch('/set?playback=0&micEnabled=0');
    
    if (pollInterval) {
        clearInterval(pollInterval);
        pollInterval = null;
    }
}

// Tabs
function openTab(tabName) {
    stopAll(); // Stop audio when switching tabs
    
    document.querySelectorAll('.tab-content').forEach(d => d.style.display = 'none');
    document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
    
    document.getElementById(tabName).style.display = 'block';
    
    const btns = document.querySelectorAll('.tab-btn');
    if(tabName === 'presets') btns[0].classList.add('active');
    else btns[1].classList.add('active');
}

// Presets
function setFreq(val) {
    freqSlider.value = val;
    freqVal.innerText = val;
    fetch(`/set?frequency=${val}`);
}

// Graph
const canvas = document.getElementById('volGraph');
const ctx = canvas.getContext('2d');
const maxPoints = 100;
let history = new Array(maxPoints).fill(0);
let pollInterval = null;

function updateStatus() {
    fetch('/status')
        .then(response => response.json())
        .then(data => {
            if (!micActive && document.activeElement !== freqSlider) {
                freqSlider.value = Math.round(data.frequency);
                freqVal.innerText = Math.round(data.frequency);
            }
            history.push(data.micValue);
            if (history.length > maxPoints) history.shift();
            drawGraph();
        })
        .catch(err => console.error('Status fetch failed', err));
}

manualBtn.onclick = () => {
    playing = !playing;
    // Ensure mic is off if manual started (safety)
    micActive = false; 
    
    manualBtn.innerText = playing ? 'STOP MANUAL MODE' : 'START MANUAL MODE';
    manualBtn.className = playing ? 'btn stop' : 'btn start';
    
    fetch(`/set?playback=${playing ? 1 : 0}&micEnabled=0`);
    
    // Also start polling if playing manually to see frequency updates
    if (playing) {
         if (!pollInterval) pollInterval = setInterval(updateStatus, 200);   
    } else {
        if (pollInterval) { clearInterval(pollInterval); pollInterval = null; }
    }
};

freqSlider.oninput = (e) => {
    freqVal.innerText = e.target.value;
    fetch(`/set?frequency=${e.target.value}`);
};

volSlider.oninput = (e) => {
    volVal.innerText = e.target.value;
    fetch(`/set?volume=${e.target.value / 100}`);
};

micBtn.onclick = () => {
    micActive = !micActive;
    playing = micActive; // Mic mode implies playing
    
    if (micActive) {
        micBtn.innerText = "STOP MIC MODE";
        micBtn.className = "btn stop";
        fetch('/set?playback=1&micEnabled=1');
        if (!pollInterval) pollInterval = setInterval(updateStatus, 100);
    } else {
        micBtn.innerText = "START MIC MODE";
        micBtn.className = "btn start";
        fetch('/set?playback=0&micEnabled=0');
        if (pollInterval) { clearInterval(pollInterval); pollInterval = null; }
    }
};

function drawGraph() {
    const w = canvas.width;
    const h = canvas.height;
    ctx.clearRect(0, 0, w, h);
    ctx.beginPath();
    ctx.strokeStyle = '#3498db';
    ctx.lineWidth = 2;
    const slice = w / (maxPoints - 1);
    const maxY = 4096; 
    for (let i = 0; i < history.length; i++) {
        const x = i * slice;
        let val = history[i];
        if (val > maxY) val = maxY;
        const y = h - (val / maxY * h);
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    }
    ctx.stroke();
}
)=====";

#endif