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

- (void)clearButtons;
- (void)addButton:(const char*)label index:(int)index callback:(ButtonCallback)callback;
- (void)relayoutButtons;
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

- (void)clearButtons {
    for (NSButton* button in self.buttons) {
        [button removeFromSuperview];
    }
    [self.buttons removeAllObjects];
    [self.buttonCallbacks removeAllObjects];
}

- (void)addButton:(const char*)label index:(int)index callback:(ButtonCallback)callback {
    NSString* labelStr = [NSString stringWithUTF8String:label];
    NSButton* button = [[NSButton alloc] init];
    [button setTitle:labelStr];
    [button setBezelStyle:NSBezelStyleRounded];
    [button setTarget:self];
    [button setAction:@selector(buttonClicked:)];
    [[self.panel contentView] addSubview:button];
    [self.buttons addObject:button];
    
    ButtonInfo* info = [[ButtonInfo alloc] init];
    info.index = index;
    info.callback = callback;
    [self.buttonCallbacks addObject:info];
}

- (void)relayoutButtons {
    if (self.buttons.count == 0) {
        // Resize window to smaller height if no buttons
        NSRect frame = self.panel.frame;
        frame.size.height = 140; // Base height without buttons
        [self.panel setFrame:frame display:YES animate:YES];
        return;
    }
    
    // Resize window to accommodate buttons
    NSRect frame = self.panel.frame;
    frame.size.height = 180; // Base height with buttons
    [self.panel setFrame:frame display:YES animate:YES];
    
    // Reposition buttons
    CGFloat buttonWidth = 100;
    CGFloat buttonHeight = 30;
    CGFloat buttonSpacing = 10;
    CGFloat startX = self.panel.frame.size.width - (buttonWidth + 20);
    CGFloat buttonY = 20;
    
    for (NSButton* button in self.buttons) {
        [button setFrame:NSMakeRect(startX, buttonY, buttonWidth, buttonHeight)];
        startX -= (buttonWidth + buttonSpacing);
    }
}
@end

extern "C" __attribute__((visibility("default")))
void* ShowProgressBarMacOS(const char* title, const char* message, const char* style,
                          const char** buttonLabels, int buttonCount, ButtonCallback callback) {
    if (title == nullptr) title = "Progress";
    if (message == nullptr) message = "";
    if (style == nullptr) style = "default";
    
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
void UpdateProgressBarMacOS(void* handle, double progress, const char* message, 
                          const char** buttonLabels, int buttonCount, ButtonCallback callback) {
    
    if (handle == nullptr) {
        return;
    }
    
    @autoreleasepool {
        @try {
            ProgressBarWrapper* wrapper = (__bridge ProgressBarWrapper*)handle;
            if (!wrapper) {
                NSLog(@"Wrapper is null");
                return;
            }
            
            if (!wrapper.progressBar) {
                NSLog(@"Progress bar is null");
                return;
            }
            
            // Create NSString from message outside the async block
            NSString* messageStr = nil;
            if (message != nullptr) {
                messageStr = [NSString stringWithUTF8String:message];
            }
            
            // Verify button data
            if (buttonCount > 0 && buttonLabels == nullptr) {
                NSLog(@"Button labels array is null but count is %d", buttonCount);
                buttonCount = 0;
            }
            
            // Create a local copy of the button data with explicit string copying
            NSMutableArray* pendingButtons = [NSMutableArray array];
            for (int i = 0; i < buttonCount; i++) {
                if (buttonLabels[i] != nullptr) {
                    // Create an explicit copy of the string
                    const char* label = strdup(buttonLabels[i]);
                    NSString* labelStr = [NSString stringWithUTF8String:label];
                    free((void*)label);
                    
                    if (labelStr) {  // Ensure string conversion succeeded
                        [pendingButtons addObject:@{
                            @"label": labelStr,
                            @"index": @(i)
                        }];
                    }
                }
            }
            
            dispatch_async(dispatch_get_main_queue(), ^{
                [wrapper.progressBar setDoubleValue:progress];
                
                if (messageStr != nil && wrapper.messageLabel != nil) {
                    [wrapper.messageLabel setStringValue:messageStr];
                }
                
                [wrapper clearButtons];
                
                if (pendingButtons.count > 0) {
                    for (NSDictionary* buttonData in pendingButtons) {
                        NSString* label = buttonData[@"label"];
                        NSButton* button = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 100, 30)];
                        [button setTitle:label];
                        [button setBezelStyle:NSBezelStyleRounded];
                        [button setTarget:wrapper];
                        [button setAction:@selector(buttonClicked:)];
                        [[wrapper.panel contentView] addSubview:button];
                        [wrapper.buttons addObject:button];
                        
                        ButtonInfo* info = [[ButtonInfo alloc] init];
                        info.index = [buttonData[@"index"] intValue];
                        info.callback = callback;
                        [wrapper.buttonCallbacks addObject:info];
                    }
                }
                
                [wrapper relayoutButtons];
                
                if (progress >= 100) {
                    NSLog(@"Progress complete, closing window");
                    CloseProgressBarMacOS(handle);
                }
            });
        } @catch (NSException *exception) {
            NSLog(@"Exception in UpdateProgressBarMacOS: %@", exception);
            NSLog(@"Exception reason: %@", [exception reason]);
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
            NSLog(@"Exception in CloseProgressBarMacOS: %@", exception);
            NSLog(@"Exception reason: %@", [exception reason]);
        }
    }
}
