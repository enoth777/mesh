import http from "http";
import { WebSocketServer } from "ws";

const PORT = process.env.PORT || 10000;

const server = http.createServer((req, res) => {
  res.writeHead(200, {
    "Content-Type": "text/plain"
  });

  res.end("MESH relay running");
});

const wss = new WebSocketServer({
  server
});

wss.on("connection", socket => {
  console.log("WebSocket client connected");

  socket.on("message", message => {
    console.log("Received:", message.toString());
  });

  socket.on("close", () => {
    console.log("Client disconnected");
  });
});

server.listen(PORT, "0.0.0.0", () => {
  console.log(`MESH relay running on port ${PORT}`);
});