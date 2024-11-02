#ifndef PROGRESS_BAR_MACOS_H
#define PROGRESS_BAR_MACOS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
extern "C" __attribute__((visibility("default")))
void* ShowProgressBarMacOS(const char* title, const char* message, const char* style);

extern "C" __attribute__((visibility("default")))
void UpdateProgressBarMacOS(void* handle, int progress);

extern "C" __attribute__((visibility("default")))
void CloseProgressBarMacOS(void* handle);
#endif

#ifdef __cplusplus
}
#endif

#endif // PROGRESS_BAR_MACOS_H
