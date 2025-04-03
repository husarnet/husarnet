const WebSocketServer = require('ws').Server;

const wss = new WebSocketServer({ port: 8080 });

wss.on('connection', function connection(ws) {
  ws.on('error', console.error);

  ws.on('message', function message(data) {
    ws.send('pong')
    console.log('received: %s', data);
  });
  ws.ping();
});