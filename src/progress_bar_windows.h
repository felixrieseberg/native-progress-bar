#ifndef PROGRESS_BAR_WINDOWS_H
#define PROGRESS_BAR_WINDOWS_H

#ifdef __cplusplus
extern "C" {
#endif

void* ShowProgressBarWindows(
    const char* title,
    const char* message,
    const char* style,
    const char** buttonLabels,
    size_t buttonCount,
    void (*callback)(int)
);

void UpdateProgressBarWindows(
    void* handle,
    int progress,
    const char* message,
    bool updateButtons,
    const char** buttonLabels,
    size_t buttonCount,
    void (*callback)(int)
);

void CloseProgressBarWindows(void* handle);

#ifdef __cplusplus
}
#endif

#endif // PROGRESS_BAR_WINDOWS_H 
