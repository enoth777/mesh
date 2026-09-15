import http from "http";
import { WebSocketServer, WebSocket } from "ws";

const PORT = process.env.PORT || 10000;

const server = http.createServer((req, res) => {
  res.writeHead(200, { "Content-Type": "text/plain" });
  res.end("MESH relay running");
});

const wss = new WebSocketServer({ server });

const rooms = new Map();

function getRoom(name) {
  if (!rooms.has(name)) {
    rooms.set(name, {
      esp: null,
      espLastHeartbeat: 0,
      browsers: new Map()
    });
  }

  return rooms.get(name);
}

function getFreeSlot(room) {

  const used = new Set(
    [...room.browsers.values()].map(browser => browser.slot)
  );
  for (let i = 1; i <= 8; i++) {
    if (!used.has(i)) {
      return i;
    }
  }

  return null;
}

function broadcastEspStatus(room, connected) {
  const msg = connected ? "E,1" : "E,0";

  for (const ws of room.browsers.keys()) {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(msg);
    }
  }
}

function broadcastClientCount(room) {

  const msg =
    `C,${room.browsers.size}`;

  // Send count to all browsers
  for (const ws of room.browsers.keys()) {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(msg);
    }
  }

  // Send count to ESP
  if (
    room.esp &&
    room.esp.readyState === WebSocket.OPEN
  ) {
    room.esp.send(msg);
  }
}

wss.on("connection", (ws, req) => {

  const url = new URL(
    req.url,
    `http://${req.headers.host}`
  );

  if (url.pathname !== "/ws") {
    ws.close();
    return;
  }

  const role =
    url.searchParams.get("role");

  const roomName =
    url.searchParams.get("room") || "mesh";

  const room =
    getRoom(roomName);


  // =========================
  // ESP32
  // =========================

  if (role === "esp") {

    console.log(
      `[${roomName}] ESP connected`
    );

    if (
      room.esp &&
      room.esp.readyState === WebSocket.OPEN
    ) {
      room.esp.close();
    }

    room.esp = ws;
    room.espLastHeartbeat = Date.now();

    broadcastEspStatus(
      room,
      true
    );

    ws.on("message", data => {

      const message =
        data.toString();

    if (message === "H") {

      room.espLastHeartbeat = Date.now();

      console.log(
        `[${roomName}] ESP heartbeat received`
      );
    }

    });

    ws.on("close", () => {

      if (room.esp === ws) {

        room.esp = null;

        console.log(
          `[${roomName}] ESP disconnected`
        );

        broadcastEspStatus(
          room,
          false
        );
      }
    });

    return;
  }


  // =========================
  // Browser
  // =========================

  if (role === "browser") {

    const slot =
      getFreeSlot(room);

    if (slot === null) {
      ws.send("FULL");
      ws.close();
      return;
    }

    room.browsers.set(ws, {
      slot: slot, 
      lastHeartbeat: Date.now()
    });

    broadcastClientCount(room);

    console.log(
      `[${roomName}] Device ${slot} connected`
    );

    ws.send(
      `S,${slot}`
    );

    const espOnline =
      room.esp &&
      room.esp.readyState === WebSocket.OPEN;

    ws.send(
      espOnline
        ? "E,1"
        : "E,0"
    );

    ws.on("message", data => {


      const message = data.toString();
      // ========================= Browser heartbeat ========================
      if (message === "P") {
        const browser = room.browsers.get(ws);

        if (browser) {
          browser.lastHeartbeat = Date.now();

          console.log(
            `[${roomName}] Device ${browser.slot} heartbeat`
          );
        }

        return;
      }

      // ========================= Ignore anything except coordinates ========================
      if (!message.startsWith("X,")) {
        return;
      }

      const parts = message.split(",");

      if (parts.length !== 3) {
        return;
      }

      let x =
        Number(parts[1]);

      let y =
        Number(parts[2]);

      if (
        !Number.isFinite(x) ||
        !Number.isFinite(y)
      ) {
        return;
      }

      x = Math.max(
        0,
        Math.min(
          126,
          Math.round(x)
        )
      );

      y = Math.max(
        0,
        Math.min(
          126,
          Math.round(y)
        )
      );

      if (
        room.esp &&
        room.esp.readyState === WebSocket.OPEN
      ) {

        room.esp.send(
          `X,${slot},${x},${y}`
        );
      }
    });

    ws.on("close", () => {

      room.browsers.delete(ws);

      broadcastClientCount(room);

      console.log(
        `[${roomName}] Device ${slot} disconnected`
      );
    });

    return;
  }


  ws.close();
});


// =========================
// Heartbeat monitor
// =========================

setInterval(() => {

  const now = Date.now();

  for (const [name, room] of rooms) {

    // ESP heartbeat
    const esp = room.esp;

    if (
      esp &&
      now - room.espLastHeartbeat > 6000
    ) {
      console.log(
        `[${name}] ESP heartbeat timeout`
      );

      if (room.esp === esp) {
        room.esp = null;
      }

      esp.terminate();

      broadcastEspStatus(
        room,
        false
      );
    }

    // Browser heartbeats
    for (const [ws, browser] of room.browsers) {

      if (
        now - browser.lastHeartbeat > 6000
      ) {
        console.log(
          `[${name}] Device ${browser.slot} heartbeat timeout`
        );

        room.browsers.delete(ws);
        ws.terminate();

        broadcastClientCount(room);
      }
    }
  }

}, 50);

server.listen(
  PORT,
  "0.0.0.0",
  () => {
    console.log(
      `MESH relay running on port ${PORT}`
    );
  }
);