let ws = new WebSocket('ws://' + window.location.hostname + ':81/');
let chartData = { labels: [], datasets: [{ label: 'Bluetooth Data', data: [], borderColor: 'blue', fill: false }] };
let chart = new Chart(document.getElementById('dataChart'), {
  type: 'line',
  data: chartData,
  options: { scales: { x: { title: { display: true, text: 'Sample' } }, y: { title: { display: true, text: 'Value' } } } }
});

ws.onmessage = function(event) {
  let data = JSON.parse(event.data);
  document.getElementById('dataDisplay').innerText = 'Latest Data: ' + data.value;
  chartData.labels.push(data.index);
  chartData.datasets[0].data.push(parseFloat(data.value));
  if (chartData.labels.length > 50) {
    chartData.labels.shift();
    chartData.datasets[0].data.shift();
  }
  chart.update();
};

ws.onclose = function() {
  ws = new WebSocket('ws://' + window.location.hostname + ':81/');
};