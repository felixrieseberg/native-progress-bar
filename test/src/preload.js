const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("native", {
  createProgressBar: function (type) {
    return ipcRenderer.invoke("create-progress-bar", type);
  },
});
