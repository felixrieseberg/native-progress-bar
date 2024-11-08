# native-progress-bar

This module allows your Electron app to display native dialogs with progress bars in them on Windows and macOS.
It has no dependencies other than `binding` and creates a ~85KB Node addon.

To see various examples for progress bars, check out `test/src/progress-bars.js`.

## Screenshots

![Default Style Progress Bar](docs/screenshots/windows.png)

![HUD Style Progress Bar](docs/screenshots/macos-default.png)

![Utility Style Progress Bar](docs/screenshots/macos-hud.png)

## Application required (macOS)

On macOS, the module _only_ works in Node.js environments that run within a proper application (like Electron).
It therefore does not run in simple Node.js scripts that you might execute with `node myscript.js`.

## API

```ts
import { ProgressBar } from "native-progress-bar"

let progressBar, interval;

// All arguments are optional
progressBar = new ProgressBar({
  // Window title
  title: "Running disk operation",
  // Message, shown above the progress bar
  message: "Running format C:",
  // Initial progress value
  progress: 0,
  // Can be "hud", "utility", or "default". Only of effect on macOS.
  style: "hud"
  // Zero or more buttons
  buttons: [{
    label: "Cancel",
    click: (progressBar) => {
      console.log("Cancel button clicked");
      progressBar.close();
    }
  }],
  // A function called when the dialog is closed. Useful to cleanup intervals.
  onClose: () => {
    clearInterval(interval);
  },
});

interval = setInterval(() => {
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

    return
  }

  progressBar.progress += 1;
}, 200);
```

## What about Linux?

I didn't need Linux but I'd welcome PRs implementing it there.
