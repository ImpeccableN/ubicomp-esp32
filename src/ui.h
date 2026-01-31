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
    <h1>ESP32 Tone Gen</h1>
    <button id="mainBtn" class="btn start">START</button>
    
    <div class="group">
        <h3>Frequency: <span id="freqVal">440</span> Hz</h3>
        <input type="range" id="freqSlider" min="0" max="2000" value="100">
    </div>

    <div class="group">
        <h3>Volume: <span id="volVal">10</span>%</h3>
        <input type="range" id="volSlider" min="0" max="100" value="10">
    </div>

    <div class="group">
        <h3>Microphone Control</h3>
        <button id="micBtn" class="btn start">Enable Mic Control</button>
    </div>

    <div class="group">
        <h3>Volume History (10s)</h3>
        <canvas id="volGraph" width="600" height="200" style="width:100%; height:200px; border-radius:10px; background:#fafafa;"></canvas>
    </div>

    <script src="script.js"></script>
</body>
</html>
)=====";

const char STYLE_CSS[] = R"=====(
body { 
    font-family: sans-serif; 
    text-align: center; 
    background: #fdfdfd; 
    color: #333; 
    padding: 20px; 
}
h1 { color: #222; }
h3 { color: #555; }
.btn { 
    padding: 20px; 
    width: 80%; 
    font-size: 20px; 
    border-radius: 12px; 
    border: none; 
    cursor: pointer; 
    font-weight: bold; 
    margin-bottom: 20px;
    box-shadow: 0 4px 6px rgba(0,0,0,0.1);
}
.start { background: #2ecc71; color: white; }
.stop { background: #e74c3c; color: white; }
.group { 
    margin: 25px 0; 
    background: #ffffff;
    padding: 15px;
    border-radius: 15px;
    box-shadow: 0 2px 10px rgba(0,0,0,0.05);
}
input[type=range] { 
    width: 80%; 
    cursor: pointer; 
    accent-color: #3498db;
}
)=====";

const char SCRIPT_JS[] = R"=====(
let playing = false;
let micActive = false;
let audioCtx, analyser, dataArray, source;
let lastSend = 0;

const btn = document.getElementById('mainBtn');
const micBtn = document.getElementById('micBtn');
const freqSlider = document.getElementById('freqSlider');
const volSlider = document.getElementById('volSlider');
const freqVal = document.getElementById('freqVal');
const volVal = document.getElementById('volVal');
const canvas = document.getElementById('volGraph');
const ctx = canvas.getContext('2d');
const maxPoints = 100;
let history = new Array(maxPoints).fill(0);
let pollInterval = null;

function updateStatus() {
    fetch('/status')
        .then(response => response.json())
        .then(data => {
            // Update UI elements
            if (!micActive && document.activeElement !== freqSlider) {
                freqSlider.value = Math.round(data.frequency);
                freqVal.innerText = Math.round(data.frequency);
            }

            // Update Graph with value from ESP32
            history.push(data.micValue);
            if (history.length > maxPoints) history.shift();
            drawGraph();
        })
        .catch(err => console.error('Status fetch failed', err));
}

btn.onclick = () => {
    playing = !playing;
    btn.innerText = playing ? 'STOP' : 'START';
    btn.className = playing ? 'btn stop' : 'btn start';
    fetch(`/set?playback=${playing ? 1 : 0}`);
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
    if (micActive) {
        micBtn.innerText = "Disable Mic Control";
        micBtn.className = "btn stop";
        fetch('/set?micEnabled=1');
        if (!pollInterval) pollInterval = setInterval(updateStatus, 100);
    } else {
        micBtn.innerText = "Enable Mic Control";
        micBtn.className = "btn start";
        fetch('/set?micEnabled=0');
    }
};

if (!pollInterval) pollInterval = setInterval(updateStatus, 100);

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