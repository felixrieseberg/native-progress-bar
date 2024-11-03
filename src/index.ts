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
}
export interface ProgressBarArguments extends ProgressBarUpdateArguments {
  title?: string;
  style?: ProgressBarStyle;
} 

const DEFAULT_ARGUMENTS: Required<ProgressBarArguments> = {
  title: "Progress",
  message: "",
  style: "default",
  progress: 0
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

  constructor(args: ProgressBarArguments = DEFAULT_ARGUMENTS) {
    const title = args.title || DEFAULT_ARGUMENTS.title;
    const style = args.style || DEFAULT_ARGUMENTS.style;
    this._message = args.message || DEFAULT_ARGUMENTS.message;

    this.handle = native.showProgressBar(title, this._message, style);
  }

  public update(args?: ProgressBarUpdateArguments) {
    this.validateHandle();

    this._progress = args?.progress || this._progress;
    this._message = args?.message || this._message;
    
    native.updateProgress(this.handle, this._progress, this._message);
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
