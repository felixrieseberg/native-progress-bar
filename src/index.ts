import bindings from 'bindings';

const native = bindings('progress_bar');
const activeProgressBars = new Set<ProgressBar>();

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

type ProgressBarStyle = 'default' | 'hud' | 'utility';

export interface ProgressBarUpdateArguments {
  progress?: number;
  message?: string;
  buttons?: ProgressBarButtonArguments[];
}
export interface ProgressBarArguments extends ProgressBarUpdateArguments {
  title?: string;
  style?: ProgressBarStyle;
}

export interface ProgressBarButtonArguments {
  label: string;
  click: () => void;
}

const DEFAULT_ARGUMENTS: Required<ProgressBarArguments> = {
  title: "Progress",
  message: "",
  style: "default",
  progress: 0,
  buttons: []
}

export class ProgressBar {
  public readonly title: string;
  public readonly style: string;
  public handle: number | null = null;
  public isClosed: boolean = false;

  private _progress: number = 0;
  public get progress() {
    return this._progress;
  }
  public set progress(value: number) {
    if (value < 0 || value > 100) {
      throw new Error('Progress must be between 0 and 100');
    }

    this._progress = value;
    this.update();
  }

  private _message: string = "";
  public get message() {
    return this._message;
  }
  public set message(value: string) {
    this._message = value;
    this.update();
  }

  private _buttons: ProgressBarButtonArguments[] = [];
  public get buttons() {
    return this._buttons;
  }
  public set buttons(value: ProgressBarButtonArguments[]) {
    this._buttons = value;
    this.update();
  }

  constructor(args: ProgressBarArguments = DEFAULT_ARGUMENTS) {
    const title = args.title || DEFAULT_ARGUMENTS.title;
    const style = args.style || DEFAULT_ARGUMENTS.style;
    this._buttons = args.buttons || DEFAULT_ARGUMENTS.buttons;
    this._message = args.message || DEFAULT_ARGUMENTS.message;

    this.handle = native.showProgressBar(title, this._message, style, this._buttons);
  }

  public update(args?: ProgressBarUpdateArguments) {
    this.validateHandle();

    this._progress = args?.progress || this._progress;
    this._message = args?.message || this._message;
    this._buttons = args?.buttons || this._buttons;

    native.updateProgress(this.handle, this._progress, this._message, this._buttons);
  }

  public close() {
    if (!this.isClosed && this.handle) {
      native.closeProgress(this.handle);
      this.isClosed = true;
      this.handle = null;
    }
  }

  private validateHandle() {
    if (this.isClosed || !this.handle) {
      throw new Error('Progress bar has been closed');
    }
  }
}
