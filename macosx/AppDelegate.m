//
//  AppDelegate.m
//  osxapp
//
//  Created by Minh Nguyen on 2015-11-21.
//  Copyright © 2015 Mapbox. All rights reserved.
//

#import "AppDelegate.h"

#import <mbgl/osx/MGLAccountManager.h>
#import <mbgl/osx/MGLMapView.h>
#import <mbgl/osx/MGLStyle.h>

@interface AppDelegate () <NSSharingServicePickerDelegate>

@property (weak) IBOutlet NSWindow *window;
@property (strong) IBOutlet MGLMapView *mapView;

@property (weak) IBOutlet NSWindow *preferencesWindow;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Set access token, unless MGLAccountManager already read it in from Info.plist.
    if (![MGLAccountManager accessToken]) {
        NSString *accessToken = [[NSProcessInfo processInfo] environment][@"MAPBOX_ACCESS_TOKEN"];
        if (accessToken) {
            // Store to preferences so that we can launch the app later on without having to specify
            // token.
            [[NSUserDefaults standardUserDefaults] setObject:accessToken forKey:@"MGLMapboxAccessToken"];
        } else {
            // Try to retrieve from preferences, maybe we've stored them there previously and can reuse
            // the token.
            accessToken = [[NSUserDefaults standardUserDefaults] stringForKey:@"MGLMapboxAccessToken"];
        }
        if (!accessToken) {
            NSAlert *alert = [[NSAlert alloc] init];
            alert.messageText = @"Access token required";
            alert.informativeText = @"To load Mapbox-hosted tiles and styles, enter your Mapbox access token in Preferences.";
            [alert addButtonWithTitle:@"Open Preferences"];
            [alert runModal];
            [self showPreferences:nil];
        }
        
        [MGLAccountManager setAccessToken:accessToken];
    }
    
    self.mapView = [[MGLMapView alloc] initWithFrame:self.window.contentView.bounds];
    self.mapView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self.window.contentView addSubview:self.mapView];
    [self.window makeFirstResponder:self.mapView];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
}

- (IBAction)showShareMenu:(id)sender {
    NSSharingServicePicker *picker = [[NSSharingServicePicker alloc] initWithItems:@[self.shareURL]];
    picker.delegate = self;
    [picker showRelativeToRect:[sender bounds] ofView:sender preferredEdge:NSMinYEdge];
}

- (NSURL *)shareURL {
    NSArray *components = self.mapView.styleURL.pathComponents;
    CLLocationCoordinate2D centerCoordinate = self.mapView.centerCoordinate;
    return [NSURL URLWithString:
            [NSString stringWithFormat:@"https://api.mapbox.com/styles/v1/%@/%@.html?access_token=%@#%.2f/%.5f/%.5f/%.f",
             components[1], components[2], [MGLAccountManager accessToken],
             self.mapView.zoomLevel, centerCoordinate.latitude, centerCoordinate.longitude, self.mapView.direction]];
}

- (IBAction)setStyle:(id)sender {
    NSInteger tag;
    if ([sender isKindOfClass:[NSMenuItem class]]) {
        tag = [sender tag];
    } else if ([sender isKindOfClass:[NSPopUpButton class]]) {
        tag = [sender selectedTag];
    }
    NSURL *styleURL;
    switch (tag) {
        case 1:
            styleURL = [MGLStyle streetsStyleURL];
            break;
        case 2:
            styleURL = [MGLStyle emeraldStyleURL];
            break;
        case 3:
            styleURL = [MGLStyle lightStyleURL];
            break;
        case 4:
            styleURL = [MGLStyle darkStyleURL];
            break;
        case 5:
            styleURL = [MGLStyle satelliteStyleURL];
            break;
        case 6:
            styleURL = [MGLStyle hybridStyleURL];
            break;
        default:
            NSAssert(NO, @"Cannot set style from control with tag %li", (long)tag);
            break;
    }
    self.mapView.styleURL = styleURL;
    [self.window.toolbar validateVisibleItems];
}

- (IBAction)chooseCustomStyle:(id)sender {
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Apply custom style";
    alert.informativeText = @"Enter the URL to a JSON file that conforms to the Mapbox GL style specification:";
    NSTextField *textField = [[NSTextField alloc] initWithFrame:NSZeroRect];
    [textField sizeToFit];
    NSRect textFieldFrame = textField.frame;
    textFieldFrame.size.width = 300;
    textField.frame = textFieldFrame;
    NSString *savedURLString = [[NSUserDefaults standardUserDefaults] stringForKey:@"MBXCustomStyleURL"];
    if (savedURLString) {
        textField.stringValue = savedURLString;
    }
    alert.accessoryView = textField;
    [alert addButtonWithTitle:@"Apply"];
    [alert addButtonWithTitle:@"Cancel"];
    if ([alert runModal] == NSAlertFirstButtonReturn) {
        [[NSUserDefaults standardUserDefaults] setObject:textField.stringValue forKey:@"MBXCustomStyleURL"];
        self.mapView.styleURL = [NSURL URLWithString:textField.stringValue];
        [self.window.toolbar validateVisibleItems];
    }
}

- (IBAction)zoomIn:(id)sender {
    [self.mapView setZoomLevel:self.mapView.zoomLevel + 1 animated:YES];
}

- (IBAction)zoomOut:(id)sender {
    [self.mapView setZoomLevel:self.mapView.zoomLevel - 1 animated:YES];
}

- (IBAction)snapToNorth:(id)sender {
    [self.mapView setDirection:0 animated:YES];
}

- (IBAction)reload:(id)sender {
    NSURL *styleURL = self.mapView.styleURL;
    if ([styleURL isEqual:[MGLStyle streetsStyleURL]]) {
        self.mapView.styleURL = [MGLStyle satelliteStyleURL];
    } else {
        self.mapView.styleURL = nil;
    }
    self.mapView.styleURL = styleURL;
}

- (IBAction)toggleTileEdges:(id)sender {
    self.mapView.showsTileEdges = !self.mapView.showsTileEdges;
}

- (IBAction)toggleCollisionBoxes:(id)sender {
    self.mapView.showsCollisionBoxes = !self.mapView.showsCollisionBoxes;
}

- (IBAction)showShortcuts:(id)sender {
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Mapbox GL Help";
    alert.informativeText = @"\
• To scroll, swipe with two fingers, drag the cursor, or press the arrow keys.\n\
• To zoom, pinch with two fingers, or hold down Shift while dragging the cursor up and down.\n\
• To rotate, move two fingers opposite each other in a circle, or hold down Option while dragging the cursor left and right.\n\
• To tilt, hold down Option while dragging the cursor up and down.\
";
    [alert runModal];
}

- (IBAction)giveFeedback:(id)sender {
    CLLocationCoordinate2D centerCoordinate = self.mapView.centerCoordinate;
    NSURL *feedbackURL = [NSURL URLWithString:[NSString stringWithFormat:@"https://www.mapbox.com/map-feedback/#/%.5f/%.5f/%.0f",
                                               centerCoordinate.longitude, centerCoordinate.latitude, round(self.mapView.zoomLevel)]];
    [[NSWorkspace sharedWorkspace] openURL:feedbackURL];
}

- (IBAction)showPreferences:(id)sender {
    [self.preferencesWindow makeKeyAndOrderFront:sender];
}

- (IBAction)openAccessTokenManager:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://www.mapbox.com/studio/account/tokens/"]];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    if (menuItem.action == @selector(setStyle:)) {
        NSURL *styleURL = self.mapView.styleURL;
        NSCellStateValue state;
        switch (menuItem.tag) {
            case 1:
                state = [styleURL isEqual:[MGLStyle streetsStyleURL]];
                break;
            case 2:
                state = [styleURL isEqual:[MGLStyle emeraldStyleURL]];
                break;
            case 3:
                state = [styleURL isEqual:[MGLStyle lightStyleURL]];
                break;
            case 4:
                state = [styleURL isEqual:[MGLStyle darkStyleURL]];
                break;
            case 5:
                state = [styleURL isEqual:[MGLStyle satelliteStyleURL]];
                break;
            case 6:
                state = [styleURL isEqual:[MGLStyle hybridStyleURL]];
                break;
            default:
                return NO;
        }
        menuItem.state = state;
        return YES;
    }
    if (menuItem.action == @selector(chooseCustomStyle:)) {
        menuItem.state = self.indexOfStyleInToolbarItem == NSNotFound;
        return YES;
    }
    if (menuItem.action == @selector(zoomIn:)) {
        return self.mapView.zoomLevel < self.mapView.maximumZoomLevel;
    }
    if (menuItem.action == @selector(zoomOut:)) {
        return self.mapView.zoomLevel > self.mapView.minimumZoomLevel;
    }
    if (menuItem.action == @selector(snapToNorth:)) {
        return self.mapView.direction != 0;
    }
    if (menuItem.action == @selector(reload:)) {
        return YES;
    }
    if (menuItem.action == @selector(toggleTileEdges:)) {
        menuItem.title = self.mapView.showsTileEdges ? @"Hide Tile Edges" : @"Show Tile Edges";
        return YES;
    }
    if (menuItem.action == @selector(toggleCollisionBoxes:)) {
        menuItem.title = self.mapView.showsCollisionBoxes ? @"Hide Collision Boxes" : @"Show Collision Boxes";
        return YES;
    }
    if (menuItem.action == @selector(showShortcuts:)) {
        return YES;
    }
    if (menuItem.action == @selector(giveFeedback:)) {
        return YES;
    }
    if (menuItem.action == @selector(showPreferences:)) {
        return YES;
    }
    return NO;
}

- (NSUInteger)indexOfStyleInToolbarItem {
    NSArray *styleURLs = @[
        [MGLStyle streetsStyleURL],
        [MGLStyle emeraldStyleURL],
        [MGLStyle lightStyleURL],
        [MGLStyle darkStyleURL],
        [MGLStyle satelliteStyleURL],
        [MGLStyle hybridStyleURL],
    ];
    return [styleURLs indexOfObject:self.mapView.styleURL];
}

- (BOOL)validateToolbarItem:(NSToolbarItem *)toolbarItem {
    if (!self.mapView) {
        return NO;
    }
    
    if (toolbarItem.action == @selector(showShareMenu:)) {
        [(NSButton *)toolbarItem.view sendActionOn:NSLeftMouseDownMask];
        NSURL *styleURL = self.mapView.styleURL;
        return ([styleURL.scheme isEqualToString:@"mapbox"]
                && [styleURL.pathComponents.firstObject isEqualToString:@"styles"]
                && [MGLAccountManager accessToken]);
    }
    if (toolbarItem.action == @selector(setStyle:)) {
        NSPopUpButton *popUpButton = (NSPopUpButton *)toolbarItem.view;
        NSUInteger index = self.indexOfStyleInToolbarItem;
        if (index == NSNotFound) {
            [popUpButton addItemWithTitle:@"Custom"];
            index = [popUpButton numberOfItems] - 1;
        }
        [popUpButton selectItemAtIndex:index];
    }
    return NO;
}

#pragma mark NSSharingServicePickerDelegate methods

- (NS_ARRAY_OF(NSSharingService *) *)sharingServicePicker:(NSSharingServicePicker *)sharingServicePicker sharingServicesForItems:(NSArray *)items proposedSharingServices:(NS_ARRAY_OF(NSSharingService *) *)proposedServices {
    NSURL *shareURL = self.shareURL;
    NSURL *browserURL = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:shareURL];
    NSImage *browserIcon = [[NSWorkspace sharedWorkspace] iconForFile:browserURL.path];
    NSString *browserName = [[NSFileManager defaultManager] displayNameAtPath:browserURL.path];
    NSString *browserServiceName = [NSString stringWithFormat:@"Open in %@", browserName];
    
    NSSharingService *browserService = [[NSSharingService alloc] initWithTitle:browserServiceName
                                                                         image:browserIcon
                                                                alternateImage:nil
                                                                       handler:^{
        [[NSWorkspace sharedWorkspace] openURL:self.shareURL];
    }];
    
    NSMutableArray *sharingServices = [proposedServices mutableCopy];
    [sharingServices insertObject:browserService atIndex:0];
    return sharingServices;
}

@end

@interface ValidatedToolbarItem : NSToolbarItem

@end

@implementation ValidatedToolbarItem

- (void)validate {
    [(AppDelegate *)self.toolbar.delegate validateToolbarItem:self];
}

@end
