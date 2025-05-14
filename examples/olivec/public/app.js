const usernameInput = document.getElementById("username-input");
const startButton = document.getElementById("start-button");
const nameDialog = document.getElementById("name-dialog");

const drawCanvas = document.getElementById("paint-canvas");
const cursorCanvas = document.createElement("canvas");
document.body.appendChild(cursorCanvas);

const drawCtx = drawCanvas.getContext("2d");
const cursorCtx = cursorCanvas.getContext("2d");

const controls = document.getElementById("controls");
const colorPicker = document.getElementById("color-picker");
const brushSizeInput = document.getElementById("brush-size");
const clearBtn = document.getElementById("clear-btn");

let socket = null;
let username = "";
let isDrawing = false;
let lastX = null, lastY = null;

function getRandomHexColor() {
  const r = Math.floor(Math.random() * 200 + 30);  // Avoid too-dark
  const g = Math.floor(Math.random() * 200 + 30);
  const b = Math.floor(Math.random() * 200 + 30);
  return `#${[r, g, b].map(c => c.toString(16).padStart(2, '0')).join('')}`;
}

let brushColor = getRandomHexColor();
let brushSize = 6;

colorPicker.value = brushColor;
brushSizeInput.value = brushSize;

let cursors = {}; // userId => { x, y, name, lastSeen }

const virtualCanvas = document.createElement("canvas");
const virtualCtx = virtualCanvas.getContext("2d");

function ensureVirtualCanvasSize(w, h) {
  if (w > virtualCanvas.width || h > virtualCanvas.height) {
    const temp = document.createElement("canvas");
    temp.width = Math.max(w, virtualCanvas.width);
    temp.height = Math.max(h, virtualCanvas.height);
    const tempCtx = temp.getContext("2d");
    tempCtx.drawImage(virtualCanvas, 0, 0);
    virtualCanvas.width = temp.width;
    virtualCanvas.height = temp.height;
    virtualCtx.drawImage(temp, 0, 0);
  }
}

function resizeCanvases() {
  const w = window.innerWidth;
  const h = window.innerHeight;
  ensureVirtualCanvasSize(w, h);
  drawCanvas.width = cursorCanvas.width = w;
  drawCanvas.height = cursorCanvas.height = h;
  drawCtx.drawImage(virtualCanvas, 0, 0);
}
window.addEventListener("resize", resizeCanvases);
resizeCanvases();

startButton.addEventListener("click", () => {
  const name = usernameInput.value.trim();
  if (name) {
    username = name;
    nameDialog.style.display = "none";
    controls.classList.remove("hidden");
    startWebSocket();
  }
});

colorPicker.addEventListener("input", () => brushColor = colorPicker.value);
brushSizeInput.addEventListener("input", () => brushSize = parseInt(brushSizeInput.value));
clearBtn.addEventListener("click", () => send("clear", {}));

function drawLine(x1, y1, x2, y2, color = "black", size = 3) {
  [drawCtx, virtualCtx].forEach(ctx => {
    ctx.strokeStyle = color;
    ctx.lineWidth = size;
    ctx.lineCap = "round";
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.stroke();
  });
}

function send(type, data) {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify({ type, ...data }));
  }
}

function startWebSocket() {
  socket = new WebSocket("ws://localhost:9000");
  console.log("Connecting to WebSocket at:", "ws://" + window.location.host);


  socket.onopen = () => {
    send("join", { name: username });

    drawCanvas.addEventListener("mousedown", (e) => {
      isDrawing = true;
      lastX = e.clientX;
      lastY = e.clientY;
    });

    drawCanvas.addEventListener("mousemove", (e) => {
      const x = e.clientX, y = e.clientY;
      if (isDrawing) {
        drawLine(lastX, lastY, x, y, brushColor, brushSize);
        send("draw", { fromX: lastX, fromY: lastY, toX: x, toY: y, color: brushColor, size: brushSize });
        lastX = x;
        lastY = y;
      }
      send("cursor", { x, y });
      cursors["self"] = { x, y, name: username, lastSeen: Date.now() };
    });

    document.addEventListener("mouseup", () => isDrawing = false);
    window.addEventListener("blur", () => isDrawing = false);
  };

  socket.onmessage = (event) => {
    console.log("[WS] Message:", event.data);
    const msg = JSON.parse(event.data);
    if (msg.type === "draw") {
      drawLine(msg.fromX, msg.fromY, msg.toX, msg.toY, msg.color, msg.size);
    } else if (msg.type === "cursor") {
      cursors[msg.userId] = { x: msg.x, y: msg.y, name: msg.name, lastSeen: Date.now() };
    } else if (msg.type === "clear") {
      drawCtx.clearRect(0, 0, drawCanvas.width, drawCanvas.height);
      virtualCtx.clearRect(0, 0, virtualCanvas.width, virtualCanvas.height);
    }
  };
}

function renderCursors() {
  cursorCtx.clearRect(0, 0, cursorCanvas.width, cursorCanvas.height);
  const now = Date.now();

  for (const [id, cursor] of Object.entries(cursors)) {
    if (id === "self") continue;
    if (now - cursor.lastSeen > 15000) continue;

    const { x, y, name } = cursor;

    const tipX = x;
    const tipY = y;
    const baseOffset = 14;

    // Define triangle points: tip, base-right, base-left
    const baseRightX = tipX + baseOffset;
    const baseRightY = tipY + baseOffset;

    const baseLeftX = tipX + baseOffset * 0.3;
    const baseLeftY = tipY + baseOffset * 1.3;

    cursorCtx.beginPath();
    cursorCtx.moveTo(tipX, tipY);                // tip
    cursorCtx.lineTo(baseRightX, baseRightY);    // bottom right
    cursorCtx.lineTo(baseLeftX, baseLeftY);      // bottom left
    cursorCtx.closePath();

    cursorCtx.fillStyle = "rgba(255, 50, 50, 0.9)";
    cursorCtx.shadowColor = "black";
    cursorCtx.shadowBlur = 3;
    cursorCtx.fill();

    cursorCtx.strokeStyle = "#000";
    cursorCtx.lineWidth = 1.5;
    cursorCtx.stroke();

    // Username label
    cursorCtx.font = "13px sans-serif";
    cursorCtx.lineWidth = 2;
    cursorCtx.strokeStyle = "black";
    cursorCtx.strokeText(name, tipX + 10, tipY - 10);
    cursorCtx.fillStyle = "white";
    cursorCtx.fillText(name, tipX + 10, tipY - 10);
  }

  requestAnimationFrame(renderCursors);
}


renderCursors();
