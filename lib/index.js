const activeProgressBars = new Set();

process.on('exit', () => {
  // Attempt to close any remaining progress bars
  for (const progressBar of activeProgressBars) {
    try {
      progressBar.close();
    } catch (e) {
      // Ignore errors during cleanup
    }
  }
  activeProgressBars.clear();
});

function safeLoad(addonName) {
  try {
    return require("bindings")(addonName);
  } catch (e) {
    console.warn(
      `[native-progress-bar] failed to load '${addonName}' addon`,
      e
    );
  }
}
// bindings - do search addon on each call - so load addon once when module loaded instead of each fn call
const native = safeLoad("progress_bar");

const STYLES = {
  default: "default",
  hud: "hud",
  utility: "utility"
}

const DEFAULT_ARGUMENTS = {
  title: "Progress",
  message: "",
  style: STYLES.default // can be "default", "hud", or "utility"
}

class ProgressBar {
  constructor(args = DEFAULT_ARGUMENTS) {
    const title = args.title || DEFAULT_ARGUMENTS.title;
    const message = args.message || DEFAULT_ARGUMENTS.message;
    const style = args.style || DEFAULT_ARGUMENTS.style;

    this.handle = native.showProgressBar(title, message, style);
    this.closed = false;
    
    // Ensure we can't use the progress bar after it's been garbage collected
    this._validateHandle = () => {
      if (this.closed || !this.handle) {
        throw new Error('Progress bar has been closed');
      }
    };
  }

  update(progress) {
    this._validateHandle();
    
    if (progress < 0 || progress > 100) {
      throw new Error('Progress must be between 0 and 100');
    }
    
    native.updateProgress(this.handle, progress);
    
    if (progress >= 100) {
      this.closed = true;
      this.handle = null;
    }
  }

  close() {
    if (!this.closed && this.handle) {
      native.closeProgress(this.handle);
      this.closed = true;
      this.handle = null;
    }
  }
}

module.exports = {
  ProgressBar
}
