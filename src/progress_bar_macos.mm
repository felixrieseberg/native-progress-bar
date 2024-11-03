#import <Cocoa/Cocoa.h>
#include "progress_bar_macos.h"

typedef void (*ButtonCallback)(int buttonIndex);

@interface ButtonInfo : NSObject
@property (nonatomic) int index;
@property (nonatomic) ButtonCallback callback;
@end

@implementation ButtonInfo
@end

@interface ProgressBarWrapper : NSObject
@property NSPanel* panel;
@property NSProgressIndicator* progressBar;
@property NSTextField* messageLabel;
@property NSMutableArray<NSButton*>* buttons;
@property NSMutableArray<ButtonInfo*>* buttonCallbacks;
@end

@implementation ProgressBarWrapper
- (void)buttonClicked:(NSButton*)sender {
    NSUInteger index = [self.buttons indexOfObject:sender];
    if (index != NSNotFound && index < self.buttonCallbacks.count) {
        ButtonInfo* info = self.buttonCallbacks[index];
        if (info.callback) {
            info.callback(info.index);
        }
    }
}
@end

extern "C" __attribute__((visibility("default")))
void* ShowProgressBarMacOS(const char* title, const char* message, const char* style,
                          const char** buttonLabels, int buttonCount, ButtonCallback callback) {
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
    wrapper.buttons = [NSMutableArray array];
    wrapper.buttonCallbacks = [NSMutableArray array];
    
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
    
    double width = 400;
    double height = 100 + (buttonCount * 30) + (buttonCount > 0 ? 10 : 0);

    NSPanel *panel = [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, width, height)
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
    
    // Position message label at the top
    NSTextField *messageLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, height - 40, 360, 20)];
    [messageLabel setStringValue:[NSString stringWithUTF8String:message]];
    [messageLabel setBezeled:NO];
    [messageLabel setDrawsBackground:NO];
    [messageLabel setEditable:NO];
    [messageLabel setSelectable:NO];
    
    // Position progress bar below message
    NSProgressIndicator *progressBar = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(20, height - 70, 360, 20)];
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
    wrapper.messageLabel = messageLabel;
    
    // Add buttons if provided
    if (buttonLabels && buttonCount > 0) {
        CGFloat buttonWidth = 100;
        CGFloat buttonHeight = 30;
        CGFloat buttonSpacing = 10;
        CGFloat startX = panel.frame.size.width - (buttonWidth + 20);
        CGFloat buttonY = 20;  // Position buttons at the bottom
        
        for (int i = 0; i < buttonCount; i++) {
            NSString* label = [NSString stringWithUTF8String:buttonLabels[i]];
            NSButton* button = [[NSButton alloc] initWithFrame:NSMakeRect(startX, buttonY, buttonWidth, buttonHeight)];
            [button setTitle:label];
            [button setBezelStyle:NSBezelStyleRounded];
            [button setTarget:wrapper];
            [button setAction:@selector(buttonClicked:)];
            [[panel contentView] addSubview:button];
            [wrapper.buttons addObject:button];
            
            ButtonInfo* info = [[ButtonInfo alloc] init];
            info.index = i;
            info.callback = callback;
            [wrapper.buttonCallbacks addObject:info];
            
            startX -= (buttonWidth + buttonSpacing);
        }
    }
    
    return (__bridge_retained void*)wrapper;
}

extern "C" __attribute__((visibility("default")))
void UpdateProgressBarMacOS(void* handle, int progress, const char* message) {
    if (handle == nullptr) {
        return;
    }
    
    @autoreleasepool {
        @try {
            ProgressBarWrapper* wrapper = (__bridge ProgressBarWrapper*)handle;
            if (wrapper.progressBar) {
                // Create NSString from message outside the async block
                NSString* messageStr = nil;
                if (message != nullptr) {
                    messageStr = [NSString stringWithUTF8String:message];
                }
                
                dispatch_async(dispatch_get_main_queue(), ^{
                    [wrapper.progressBar setDoubleValue:progress];
                    
                    // Update message if we have one
                    if (messageStr != nil && wrapper.messageLabel != nil) {
                        [wrapper.messageLabel setStringValue:messageStr];
                    }
                    
                    if (progress >= 100) {
                        CloseProgressBarMacOS(handle);
                    }
                });
            }
        } @catch (NSException *exception) {
            NSLog(@"Exception in UpdateProgressBarMacOS: %@", exception);
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
                    wrapper.messageLabel = nil;
                });
            }
        } @catch (NSException *exception) {
            // Log or handle the exception if needed
        }
    }
}
