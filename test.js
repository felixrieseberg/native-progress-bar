const progressBar = require('./build/Release/progress_bar');

console.log(progressBar);

// Show progress dialog
progressBar.showProgressBar("Progress", 50);

setTimeout(() => {
  process.exit(0);
}, 5000);
