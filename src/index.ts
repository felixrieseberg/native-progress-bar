import bindings from "bindings";

const native = bindings("progress_bar");
const activeProgressBars = new Set<ProgressBar>();

process.on("exit", () => {
  // Attempt to close any remaining progress bars
  for (const progressBar of activeProgressBars) {
    if (progressBar.isClosed) {
      continue;
    }

    try {
      progressBar.close();
    } catch (e) {
      // Ignore errors during cleanup
    }
  }

  activeProgressBars.clear();
});

type ProgressBarStyle = "default" | "hud" | "utility";

export interface ProgressBarUpdateArguments {
  progress?: number;
  message?: string;
  buttons?: ProgressBarButtonArguments[];
}
export interface ProgressBarArguments extends ProgressBarUpdateArguments {
  title?: string;
  style?: ProgressBarStyle;
  onClose?: (progressBar: ProgressBar) => void;
}

export interface ProgressBarButtonArguments {
  label: string;
  click: (progressBar: ProgressBar) => void;
}

export interface InternalProgressBarButtonArguments {
  label: string;
  click: () => void;
  source: ProgressBarButtonArguments;
}

const DEFAULT_ARGUMENTS: Required<ProgressBarArguments> = {
  title: "Progress",
  message: "",
  style: "default",
  progress: 0,
  buttons: [],
  onClose: () => {},
};

export class ProgressBar {
  public readonly title: string;
  public readonly style: string;
  public handle: number | null = null;
  public isClosed: boolean = false;
  public onClose?: (progressBar: ProgressBar) => void;

  /**
   * The progress of the progress bar, between 0 and 100
   */
  public get progress() {
    return this._progress;
  }
  public set progress(value: number) {
    if (value < 0 || value > 100) {
      throw new Error("Progress must be between 0 and 100");
    }

    this._progress = value;
    this.update();
  }
  private _progress: number = 0;

  /**
   * The message of the progress bar
   */
  public get message() {
    return this._message;
  }
  public set message(value: string) {
    this._message = value;
    this.update();
  }
  private _message: string = "";

  /**
   * The buttons of the progress bar. Can be dynamically set.
   */
  public get buttons() {
    return this._buttons;
  }
  public set buttons(value: ProgressBarButtonArguments[]) {
    this._buttons = value;
    this.update({ buttons: value });
  }
  private _buttons: ProgressBarButtonArguments[] = [];
  private _internalButtons: InternalProgressBarButtonArguments[] = [];

  constructor(args: ProgressBarArguments = DEFAULT_ARGUMENTS) {
    const title = args.title || DEFAULT_ARGUMENTS.title;
    const style = args.style || DEFAULT_ARGUMENTS.style;
    this._buttons = args.buttons || DEFAULT_ARGUMENTS.buttons;
    this._message = args.message || DEFAULT_ARGUMENTS.message;
    this.onClose = args.onClose;

    this.handle = native.showProgressBar(
      title,
      this._message,
      style,
      this.getButtons(this._buttons),
    );

    // Prevent general GC from closing the progress bar
    activeProgressBars.add(this);
  }

  public update(args?: ProgressBarUpdateArguments) {
    if (!this.validateHandle()) {
      return;
    }

    this._progress = args?.progress || this._progress;
    this._message = args?.message || this._message;

    const shouldUpdateButtons = this.getButtonsUpdateNecessary(args?.buttons);
    const buttons = shouldUpdateButtons ? this.getButtons(args?.buttons) : [];

    native.updateProgress(
      this.handle,
      this._progress,
      this._message,
      shouldUpdateButtons,
      buttons,
    );
  }

  public close() {
    if (!this.isClosed && this.handle) {
      native.closeProgress(this.handle);
      this.isClosed = true;
      this.handle = null;
      this.onClose?.(this);

      // Allow general GC to close the progress bar
      activeProgressBars.delete(this);
    }
  }

  private validateHandle() {
    if (this.isClosed || !this.handle) {
      return false;
    }

    return true;
  }

  /**
   * Get updated button elements with a transformed click
   */
  private getButtons(newButtons?: ProgressBarButtonArguments[]) {
    if (!newButtons || newButtons.length === 0) {
      return [];
    }

    this._internalButtons = newButtons!.map((button) => ({
      label: button.label,
      click: () => {
        if (button.click) {
          button.click(this);
        }
      },
      source: button,
    }));

    return this._internalButtons;
  }

  /**
   * Determine whether or not we even need to create a new buttons element.
   *
   * @returns {boolean} true if we need to update, false otherwise
   */
  private getButtonsUpdateNecessary(newButtons?: ProgressBarButtonArguments[]) {
    if (!newButtons || newButtons.length === 0) {
      return false;
    }

    if (newButtons.length !== this._internalButtons.length) {
      return true;
    }

    for (let i = 0; i < newButtons.length; i++) {
      if (newButtons[i] !== this._internalButtons[i].source) {
        return true;
      }
    }

    return false;
  }
}
