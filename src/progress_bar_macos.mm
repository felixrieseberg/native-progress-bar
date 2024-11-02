#import <Cocoa/Cocoa.h>
#include "progress_bar_macos.h"

@interface ProgressBarWrapper : NSObject
@property NSPanel* panel;
@property NSProgressIndicator* progressBar;
@end

@implementation ProgressBarWrapper
@end

extern "C" __attribute__((visibility("default")))
void* ShowProgressBarMacOS(const char* title, const char* message, const char* style) {
    if (title == nullptr) {
        title = "Progress";
    }
    if (message == nullptr) {
        message = "";
    }
    if (style == nullptr) {
        style = "default";
    }
    
    ProgressBarWrapper* wrapper = [[ProgressBarWrapper alloc] init];
    
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
    
    // Base style mask
    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskNonactivatingPanel;
    
    // Add additional style based on parameter
    NSString* styleStr = [NSString stringWithUTF8String:style];
    if ([styleStr isEqualToString:@"hud"]) {
        styleMask |= NSWindowStyleMaskHUDWindow;
    } else if ([styleStr isEqualToString:@"utility"]) {
        styleMask |= NSWindowStyleMaskUtilityWindow;
    }
    
    NSPanel *panel = [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, 400, 100)
                                               styleMask:styleMask
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];
    
    // If HUD style, set appearance to vibrant dark
    if ([styleStr isEqualToString:@"hud"]) {
        panel.appearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark];
    }
    
    [panel setTitle:[NSString stringWithUTF8String:title]];
    [panel setLevel:NSFloatingWindowLevel];
    [panel setHidesOnDeactivate:NO];
    
    NSTextField *messageLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 60, 360, 20)];
    [messageLabel setStringValue:[NSString stringWithUTF8String:message]];
    [messageLabel setBezeled:NO];
    [messageLabel setDrawsBackground:NO];
    [messageLabel setEditable:NO];
    [messageLabel setSelectable:NO];
    
    NSProgressIndicator *progressBar = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(20, 20, 360, 20)];
    [progressBar setIndeterminate:NO];
    [progressBar setMinValue:0.0];
    [progressBar setMaxValue:100.0];
    [progressBar setDoubleValue:0.0];
    
    [[panel contentView] addSubview:messageLabel];
    [[panel contentView] addSubview:progressBar];
    
    [panel center];
    [panel makeKeyAndOrderFront:nil];
    
    wrapper.panel = panel;
    wrapper.progressBar = progressBar;
    
    return (__bridge_retained void*)wrapper;
}

extern "C" __attribute__((visibility("default")))
void UpdateProgressBarMacOS(void* handle, int progress) {
    if (handle == nullptr) {
        return;
    }
    
    @autoreleasepool {
        @try {
            ProgressBarWrapper* wrapper = (__bridge ProgressBarWrapper*)handle;
            if (wrapper.progressBar) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [wrapper.progressBar setDoubleValue:progress];
                    
                    if (progress >= 100) {
                        CloseProgressBarMacOS(handle);
                    }
                });
            }
        } @catch (NSException *exception) {
            // Log or handle the exception if needed
        }
    }
}

extern "C" __attribute__((visibility("default")))
void CloseProgressBarMacOS(void* handle) {
    if (handle == nullptr) {
        return;
    }
    
    @autoreleasepool {
        @try {
            ProgressBarWrapper* wrapper = (__bridge ProgressBarWrapper*)handle;
            if (wrapper.panel) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [wrapper.panel close];
                    wrapper.panel = nil;
                    wrapper.progressBar = nil;
                });
            }
        } @catch (NSException *exception) {
            // Log or handle the exception if needed
        }
    }
}
