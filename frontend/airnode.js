const ws = new WebSocket('ws://localhost:9001');

ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);

  if (msg.type === 'landmarks') {
    updateLandmarkSVG(msg.points);
  }

  if (msg.type === 'gesture') {
    document.getElementById('gesture-label').textContent =
      'DETECTING — ' + msg.gesture.toUpperCase();
  }

  if (msg.type === 'metrics') {
    document.getElementById('lat').textContent  = msg.latency_ms + 'ms';
    document.getElementById('conf').textContent = msg.confidence + '%';
    document.getElementById('fps-val').textContent = msg.fps;
  }

  if (msg.type === 'event') {
    appendLog(msg.tag, msg.body);
  }

  if (msg.type === 'confidence') {
    const bars = {
        pointer: 'pointer', click: 'click',
        scroll: 'scroll', drag: 'drag', zoom: 'zoom'
    };
    for (const [key, id] of Object.entries(bars)) {
        const val = msg[key] || 0;
        const pct = Math.round(val * 100);
        document.getElementById('bar-' + id).style.width = pct + '%';
        document.getElementById('val-' + id).textContent = val.toFixed(2);
    }
  }
};

function appendLog(tag, body) {
  const log = document.getElementById('log');
  const now = performance.now() / 1000;
  const m = Math.floor(now / 60);
  const s = (now % 60).toFixed(2).padStart(5, '0');
  const line = document.createElement('div');
  line.className = 'log-line';
  line.innerHTML = `<span class="log-time">${String(m).padStart(2,'0')}:${s}</span>
                    <span class="log-tag">${tag}</span>
                    <span>${body}</span>`;
  log.insertBefore(line, log.firstChild);
  if (log.children.length > 6) log.removeChild(log.lastChild);
}

const CONNECTIONS = [
  [0,1],[1,2],[2,3],[3,4],
  [0,5],[5,6],[6,7],[7,8],
  [0,9],[9,10],[10,11],[11,12],
  [0,13],[13,14],[14,15],[15,16],
  [0,17],[17,18],[18,19],[19,20],
  [5,9],[9,13],[13,17]
];

function updateLandmarkSVG(points) {
  const svg = document.querySelector('.hand-vis svg');
  if (!svg) return;
  const W = 120, H = 160, pad = 12;
  let minX = 1, maxX = 0, minY = 1, maxY = 0;
  points.forEach(p => {
      minX = Math.min(minX, p.x); maxX = Math.max(maxX, p.x);
      minY = Math.min(minY, p.y); maxY = Math.max(maxY, p.y);
  });
  const rangeX = maxX - minX || 1, rangeY = maxY - minY || 1;
  const px = p => pad + ((p.x - minX) / rangeX) * (W - pad*2);
  const py = p => pad + ((p.y - minY) / rangeY) * (H - pad*2);
  let html = '';
  CONNECTIONS.forEach(([a, b]) => {
      html += `<line x1="${px(points[a]).toFixed(1)}" y1="${py(points[a]).toFixed(1)}" x2="${px(points[b]).toFixed(1)}" y2="${py(points[b]).toFixed(1)}" stroke="#222" stroke-width="1.5"/>`;
  });
  points.forEach((p, i) => {
      const color = i === 8 ? '#1D9E75' : i === 4 ? '#378ADD' : i === 0 ? '#1D9E75' : '#333';
      html += `<circle cx="${px(p).toFixed(1)}" cy="${py(p).toFixed(1)}" r="${i === 0 ? 4 : 2.5}" fill="${color}"/>`;
  });
  svg.innerHTML = html;
}