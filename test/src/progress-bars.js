const { ipcMain } = require("electron");
const { ProgressBar } = require("native-progress-bar");

const PROGRESS_BARS = [];

function getDefaultInterval(progressBar) {
  const interval = setInterval(() => {
    if (progressBar.isClosed) {
      clearInterval(interval);
      return;
    }

    if (progressBar.progress >= 100) {
      clearInterval(interval);
      
      // You can dynamically change buttons
      progressBar.buttons = [{
        label: "Done",
        click: (progressBar) => {
          console.log("Done button clicked");
          progressBar.close();
        }
      }]

      return;
    }

    progressBar.progress += 5;
  }, 200);

  return interval;
}

function getDefaultCancelButton() {
  return {
    label: "Cancel",
    click: (progressBar) => {
      console.log("Cancel button clicked");
      progressBar.close();
    },
  };
}

function createProgressBar(type) {
  console.log(`Creating progress bar of type: ${type}`);

  // Check if any progress bars are already open
  if (PROGRESS_BARS.some((bar) => !bar.isClosed)) {
    PROGRESS_BARS.forEach((bar) => bar.close());
  }

  switch (type) {
    case "simple":
      createSimpleProgressBar();
      break;
    case "button":
      createProgressBarWithButton();
      break;
    case "appearing-button":
      createProgressBarWithAppearingButton();
      break;
    case "disappearing-button":
      createProgressBarWithDisappearingButton();
      break;
  }
}

function createSimpleProgressBar() {
  let interval, progressBar;

  progressBar = new ProgressBar({
    title: "Hi!",
    message: "Deleting all kinds of files!",
    style: "default",
    progress: 10,
    onClose: () => {
      clearInterval(interval);
    },
  });

  interval = getDefaultInterval(progressBar);

  PROGRESS_BARS.push(progressBar);
}

function createProgressBarWithButton() {
  let progressBar, interval;

  progressBar = new ProgressBar({
    title: "Hi!",
    message: "Deleting all kinds of files!",
    style: "default",
    progress: 10,
    buttons: [getDefaultCancelButton()],
    onClose: () => {
      clearInterval(interval);
    },
  });
  interval = getDefaultInterval(progressBar);

  PROGRESS_BARS.push(progressBar);
}

function createProgressBarWithAppearingButton() {
  let progressBar, interval;

  progressBar = new ProgressBar({
    title: "Hi!",
    message: "Deleting all kinds of files!",
    style: "default",
    progress: 10,
    onClose: () => {
      clearInterval(interval);
    },
  });

  interval = setInterval(() => {
    if (progressBar.progress === 10) {
      progressBar.buttons = [getDefaultCancelButton()];
    }

    progressBar.progress += 1;
  }, 200);

  PROGRESS_BARS.push(progressBar);
}

function createProgressBarWithDisappearingButton() {
  let progressBar, interval;

  progressBar = new ProgressBar({
    title: "Hi!",
    message: "Deleting all kinds of files!",
    style: "default",
    progress: 10,
    onClose: () => {
      clearInterval(interval);
    },
    buttons: [getDefaultCancelButton()],
  });

  interval = setInterval(() => {
    if (progressBar.progress === 10) {
      progressBar.buttons = [];
    }

    progressBar.progress += 1;
  }, 200);

  PROGRESS_BARS.push(progressBar);
}

async function setup() {
  ipcMain.handle("create-progress-bar", (event, type) =>
    createProgressBar(type),
  );
}

module.exports = {
  setup,
};
