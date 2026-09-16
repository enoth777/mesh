import http from "http";
import { WebSocketServer, WebSocket } from "ws";

const PORT = process.env.PORT || 10000;
const MAX_DEVICES = 7;

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

function createShortId() {
  return Math.floor(
    Math.random() * 0x10000
  )
    .toString(16)
    .toUpperCase()
    .padStart(4, "0");
}

function getFreeSlot(room) {

  const used = new Set(
    [...room.browsers.values()]
      .map(browser => browser.slot)
  );

  for (
    let i = 1;
    i <= MAX_DEVICES;
    i++
  ) {
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

function compactDeviceSlots(room) {

  // Preserve the current ordering by slot.
  const browsers =
    [...room.browsers.entries()]
      .sort(
        (a, b) =>
          a[1].slot - b[1].slot
      );

  let newSlot = 1;

  for (const [ws, browser] of browsers) {

    if (browser.slot !== newSlot) {

      console.log(
        `Device ${browser.slot} reassigned to ${newSlot}`
      );

      browser.slot = newSlot;

      if (
        ws.readyState === WebSocket.OPEN
      ) {
        ws.send(`S,${newSlot}`);
      }
    }

    newSlot++;
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

      if (!room.browsers.has(ws)) {
        return;
      }

      const browser =
        room.browsers.get(ws);

      const disconnectedSlot =
        browser.slot;

      room.browsers.delete(ws);

      console.log(
        `[${roomName}] Device ${disconnectedSlot} disconnected`
      );

      compactDeviceSlots(room);

      broadcastClientCount(room);
    });

    return;
  }


  // =========================
  // Browser
  // =========================

  // if (role === "browser") {

  //   const slot =
  //     getFreeSlot(room);

  //   if (slot === null) {
  //     ws.send("FULL");
  //     ws.close();
  //     return;
  //   }

  //   room.browsers.set(ws, {
  //     slot: slot, 
  //     lastHeartbeat: Date.now()
  //   });

  //   broadcastClientCount(room);

  //   console.log(
  //     `[${roomName}] Device ${slot} connected`
  //   );

  //   ws.send(
  //     `S,${slot}`
  //   );

  //   const espOnline =
  //     room.esp &&
  //     room.esp.readyState === WebSocket.OPEN;

  //   ws.send(
  //     espOnline
  //       ? "E,1"
  //       : "E,0"
  //   );

  //   ws.on("message", data => {


  //     const message = data.toString();
  //     // ========================= Browser heartbeat ========================
  //     if (message === "P") {
  //       const browser = room.browsers.get(ws);

  //       if (browser) {
  //         browser.lastHeartbeat = Date.now();

  //         console.log(
  //           `[${roomName}] Device ${browser.slot} heartbeat`
  //         );
  //       }

  //       return;
  //     }

  //     // ========================= Ignore anything except coordinates ========================
  //     if (!message.startsWith("X,")) {
  //       return;
  //     }

  //     const parts = message.split(",");

  //     if (parts.length !== 3) {
  //       return;
  //     }

  //     let x =
  //       Number(parts[1]);

  //     let y =
  //       Number(parts[2]);

  //     if (
  //       !Number.isFinite(x) ||
  //       !Number.isFinite(y)
  //     ) {
  //       return;
  //     }

  //     x = Math.max(
  //       0,
  //       Math.min(
  //         126,
  //         Math.round(x)
  //       )
  //     );

  //     y = Math.max(
  //       0,
  //       Math.min(
  //         126,
  //         Math.round(y)
  //       )
  //     );

  //     if (
  //       room.esp &&
  //       room.esp.readyState === WebSocket.OPEN
  //     ) {

  //       room.esp.send(
  //         `X,${slot},${x},${y}`
  //       );
  //     }
  //   });

  //   ws.on("close", () => {

  //     room.browsers.delete(ws);

  //     broadcastClientCount(room);

  //     console.log(
  //       `[${roomName}] Device ${slot} disconnected`
  //     );
  //   });

  //   return;
  // }

// =========================
// Browser
// =========================

    if (role === "browser") {

      let registered = false;
      let slot = null;

      ws.on("message", data => {

        const message = data.toString();

        // =========================
        // Registration
        // =========================

        if (!registered) {

          if (!message.startsWith("HELLO,")) {
            return;
          }

          const deviceId =
            message.substring(6).trim();

          if (!deviceId) {
            ws.close();
            return;
          }

          // Look for an existing connection
          // from this same browser tab.
          let oldSocket = null;
          let oldBrowser = null;
          let shortId = null;

          for (
            const [existingWs, browser]
            of room.browsers
          ) {
            if (browser.id === deviceId) {
              oldSocket = existingWs;
              oldBrowser = browser;
              break;
            }
          }

          const existingBrowser =
            [...room.browsers.values()]
              .find(browser => browser.id === deviceId);

          if (
            !existingBrowser &&
            room.browsers.size >= MAX_DEVICES
          ) {
            console.log(
              `[${roomName}] Connection rejected: room full`
            );

            ws.send("FULL");
            ws.close();

            return;
          }

          // Same tab reconnecting / refreshing:
          // keep its existing slot.
          if (oldBrowser) {

            slot = oldBrowser.slot;
            shortId = oldBrowser.shortId;

            room.browsers.delete(oldSocket);
            oldSocket.terminate();

            console.log(
              `[${roomName}] Device ${slot} (${shortId}) reconnected`
            );
          }

          // Completely new tab/device:
          // allocate a free slot.
          else {

            slot = getFreeSlot(room);

            if (slot === null) {
              ws.send("FULL");
              ws.close();
              return;
            }

            shortId = createShortId();

            console.log(
              `[${roomName}] Device ${slot} (${shortId}) connected`
            );
          }
          

          room.browsers.set(ws, {
            slot: slot,
            id: deviceId,
            shortId: shortId,
            lastHeartbeat: Date.now()
          });
          registered = true;

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

          broadcastClientCount(room);

          return;
        }


        // =========================
        // Browser heartbeat
        // =========================

        if (message === "P") {

          const browser =
            room.browsers.get(ws);

          if (browser) {
            browser.lastHeartbeat =
              Date.now();
          }

          return;
        }


        // =========================
        // Coordinates
        // =========================

        if (!message.startsWith("X,")) {
          return;
        }

        const parts =
          message.split(",");

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
          
          const browser = 
            room.browsers.get(ws);

          if (
              browser && 
              room.esp &&
              room.esp.readyState === WebSocket.OPEN
          ) {
            room.esp.send(
              `X,${browser.slot},${x},${y}`
            );
          }
        }
      });


      // ws.on("close", () => {

      //   // Important:
      //   // only remove this socket if it is
      //   // still registered.
      //   if (room.browsers.has(ws)) {

      //     room.browsers.delete(ws);

      //     console.log(
      //       `[${roomName}] Device ${slot} disconnected`
      //     );

      //     broadcastClientCount(room);
      //   }
      // });

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

        compactDeviceSlots(room);

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