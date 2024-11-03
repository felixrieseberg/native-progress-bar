#ifndef PROGRESS_BAR_MACOS_H
#define PROGRESS_BAR_MACOS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ButtonCallback)(int buttonIndex);

extern "C" __attribute__((visibility("default")))
void* ShowProgressBarMacOS(const char* title, const char* message, const char* style, 
                          const char** buttonLabels, int buttonCount, ButtonCallback callback);

void UpdateProgressBarMacOS(void* handle, double progress, const char* message,
                          const char** buttonLabels, int buttonCount, ButtonCallback callback);

extern "C" __attribute__((visibility("default")))
void CloseProgressBarMacOS(void* handle);

#ifdef __cplusplus
}
#endif

#endif // PROGRESS_BAR_MACOS_H
