// --- HTML File Manager ---
const char fileManagerHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 File Manager</title>
  <style>
    body { font-family: sans-serif; text-align: center; background: #f7f7f7; }
    table { margin: auto; border-collapse: collapse; width: 90%; }
    th, td { border: 1px solid #ccc; padding: 8px; }
    th { background-color: #f0f0f0; }
    button { padding: 4px 8px; margin: 2px; }
    input { margin: 4px; }
  </style>
</head>
<body>
  <h2>ESP32 File Manager</h2>
  <table id="fileTable">
    <tr><th>Name</th><th>Size (bytes)</th><th>Actions</th></tr>
  </table>
  <br>
  <h3>Upload File</h3>
  <form method="POST" action="/upload" enctype="multipart/form-data">
    <input type="file" name="file"><br><br>
    <input type="submit" value="Upload">
  </form>
<script>
function loadFiles() {
  fetch('/list').then(r => r.json()).then(data => {
    let table = document.getElementById("fileTable");
    table.innerHTML = '<tr><th>Name</th><th>Size (bytes)</th><th>Actions</th></tr>';
    data.forEach(f => {
      let row = `<tr>
        <td>${f.name}</td>
        <td>${f.size}</td>
        <td>
          <a href="/download?file=${f.name}">Download</a>
          <button onclick="deleteFile('${f.name}')">Delete</button>
          <button onclick="renameFile('${f.name}')">Rename</button>
        </td>
      </tr>`;
      table.innerHTML += row;
    });
  });
}
function deleteFile(name) {
  fetch('/delete?file=' + name).then(() => loadFiles());
}
function renameFile(oldName) {
  let newName = prompt("New name:", oldName);
  if (newName && newName !== oldName) {
    fetch(`/rename?old=${oldName}&new=${newName}`).then(() => loadFiles());
  }
}
loadFiles();
</script>
</body>
</html>
)rawliteral";


// persian file manager: 
/*
const char fileManagerHtml2[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="fa">
<head>
  <meta charset="UTF-8">
  <title>ESP32 File Manager</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; margin: 20px; background: #f7f7f7; color: #333; }
    h1 { text-align: center; }
    table { width: 100%; border-collapse: collapse; margin-top: 20px; }
    th, td { padding: 10px; text-align: left; border-bottom: 1px solid #ccc; }
    tr:hover { background: #eee; }
    .folder { font-weight: bold; color: #0074D9; cursor: pointer; }
    .actions { display: flex; gap: 5px; }
    .actions button { padding: 5px 10px; }
    form { margin-top: 20px; }
    input[type="text"], input[type="file"] { width: 100%; padding: 10px; margin-bottom: 10px; }
    .mobile { word-break: break-all; }
    .back-link { display: inline-block; margin-bottom: 10px; color: #0074D9; cursor: pointer; }
  </style>
</head>
<body>
  <h1>مدیریت فایل ESP32</h1>
  <div id="path"></div>
  <div id="fileTable"></div>

  <h2>آپلود فایل</h2>
  <form id="uploadForm">
    <input type="file" id="uploadFile" required>
    <button type="submit">آپلود</button>
  </form>

  <h2>ایجاد پوشه جدید</h2>
  <form id="mkdirForm">
    <input type="text" id="newFolderName" placeholder="نام پوشه..." required>
    <button type="submit">ایجاد پوشه</button>
  </form>

  <script>
    let currentPath = "/";

    function loadFiles(path = "/") {
      currentPath = path;
      document.getElementById("path").innerHTML =
        (path !== "/" ? `<span class='back-link' onclick='loadFiles("${path.substring(0, path.lastIndexOf("/")) || "/"}")'>⬅ بازگشت</span>` : "") +
        "<strong>موقعیت فعلی: " + path + "</strong>";

      fetch("/list?dir=" + path).then(res => res.json()).then(data => {
        let html = "<table><tr><th>نام</th><th>اندازه</th><th>عملیات</th></tr>";
        for (let file of data.files) {
          html += `<tr><td class="mobile ${file.isDir ? 'folder' : ''}" onclick='${file.isDir ? `loadFiles("${file.name}")` : ''}'>${file.name.replace(path, "")}</td>
                   <td>${file.isDir ? "-" : file.size + " bytes"}</td>
                   <td class="actions">
                     ${file.isDir ? "" : `<a href="/download?file=${file.name}">⬇</a>`}
                     <button onclick='deleteItem("${file.name}")'>🗑 حذف</button>
                     <button onclick='renameItem("${file.name}")'>✏ تغییر نام</button>
                   </td></tr>`;
        }
        html += "</table>";
        document.getElementById("fileTable").innerHTML = html;
      });
    }

    function deleteItem(name) {
      if (!confirm("آیا مطمئن هستید؟")) return;
      fetch("/delete?file=" + name, { method: "DELETE" }).then(() => loadFiles(currentPath));
    }

    function renameItem(oldName) {
      let newName = prompt("نام جدید:", oldName.substring(oldName.lastIndexOf("/") + 1));
      if (!newName) return;
      fetch("/rename", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ old: oldName, name: newName })
      }).then(() => loadFiles(currentPath));
    }

    document.getElementById("uploadForm").addEventListener("submit", e => {
      e.preventDefault();
      let fileInput = document.getElementById("uploadFile");
      let formData = new FormData();
      formData.append("file", fileInput.files[0]);
      formData.append("path", currentPath);
      fetch("/upload", { method: "POST", body: formData }).then(() => {
        fileInput.value = "";
        loadFiles(currentPath);
      });
    });

    document.getElementById("mkdirForm").addEventListener("submit", e => {
      e.preventDefault();
      let folderName = document.getElementById("newFolderName").value;
      fetch("/mkdir", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ path: currentPath + (currentPath.endsWith("/") ? "" : "/") + folderName })
      }).then(() => {
        document.getElementById("newFolderName").value = "";
        loadFiles(currentPath);
      });
    });

    loadFiles();
  </script>
</body>
</html>

)rawliteral";

*/
