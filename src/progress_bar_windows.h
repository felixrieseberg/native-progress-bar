#ifndef PROGRESS_BAR_WINDOWS_H
#define PROGRESS_BAR_WINDOWS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ButtonCallback)(int buttonIndex);

void* ShowProgressBarWindows(const char* title, const char* message, const char* style,
                           const char** buttonLabels, int buttonCount, ButtonCallback callback);

void UpdateProgressBarWindows(void* handle, double progress, const char* message,
                            bool updateButtons, const char** buttonLabels, int buttonCount, ButtonCallback callback);

void CloseProgressBarWindows(void* handle);

#ifdef __cplusplus
}
#endif

#endif // PROGRESS_BAR_WINDOWS_H 
