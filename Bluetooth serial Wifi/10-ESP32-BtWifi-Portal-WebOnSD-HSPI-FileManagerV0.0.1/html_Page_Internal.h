// HTML page with Chart.js and connection statuses
  const char* htmlPageInternal = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <title>ESP32 Bluetooth Data Plot</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
      body { font-family: Arial, sans-serif; text-align: center; }
      #dataDisplay { margin: 20px; font-size: 20px; }
      #connectionStatus, #bluetoothStatus { margin: 10px; font-size: 16px; }
      #connectionStatus { color: red; }
      #bluetoothStatus { color: red; }
      #chartContainer { width: 80%; margin: auto; }
    </style>
  </head>
  <body>
    <h1>ESP32 Bluetooth Serial Data (internal memory!)</h1>
    <br>
    <h1>هیوا | حس خوب زندگی</h1>
    <br>
    <h3>شرکت سجنش طب هیلا :)</h3>
    <div id="connectionStatus">No WebSocket Clients Connected</div>
    <div id="bluetoothStatus">Bluetooth Disconnected</div>
    <div id="dataDisplay">Latest Data: Waiting for data...</div>
    <div id="chartContainer">
      <canvas id="dataChart"></canvas>
    </div>
    <script>
      let ws = new WebSocket('ws://' + window.location.hostname + ':81/');
      let chartData = { labels: [], datasets: [{ label: 'Bluetooth Data', data: [], borderColor: 'blue', fill: false }] };
      let chart = new Chart(document.getElementById('dataChart'), {
        type: 'line',
        data: chartData,
        options: { scales: { x: { title: { display: true, text: 'Sample' } }, y: { title: { display: true, text: 'Value' } } } }
      });

      ws.onopen = function() {
        document.getElementById('connectionStatus').style.color = 'green';
      };

      ws.onmessage = function(event) {
        let data = JSON.parse(event.data);
        if (data.type === 'connection') {
          if (data.device === 'websocket') {
            document.getElementById('connectionStatus').innerText = data.message;
            document.getElementById('connectionStatus').style.color = data.connected ? 'green' : 'red';
          } else if (data.device === 'bluetooth') {
            document.getElementById('bluetoothStatus').innerText = data.message;
            document.getElementById('bluetoothStatus').style.color = data.connected ? 'green' : 'red';
          }
        } else {
          document.getElementById('dataDisplay').innerText = 'Latest Data: ' + data.value;
          chartData.labels.push(data.index);
          chartData.datasets[0].data.push(parseFloat(data.value));
          if (chartData.labels.length > 50) {
            chartData.labels.shift();
            chartData.datasets[0].data.shift();
          }
          chart.update();
        }
      };

      ws.onclose = function() {
        document.getElementById('connectionStatus').innerText = 'No WebSocket Clients Connected';
        document.getElementById('connectionStatus').style.color = 'red';
        ws = new WebSocket('ws://' + window.location.hostname + ':81/');
      };
    </script>
  </body>
  </html>
  )rawliteral";
