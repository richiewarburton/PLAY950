#include "Play950Editor.h"

#include "Play950Controller.h"
#include "content/ContentLoader.h"
#include "workflow/ImageWorkflow.h"
#include "pluginterfaces/gui/iplugview.h"
#include "public.sdk/source/common/pluginview.h"

#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

@interface PLAY950Button : NSButton
@property(nonatomic, strong) NSColor* play950FillColor;
@property(nonatomic, strong) NSColor* play950StrokeColor;
@property(nonatomic, strong) NSColor* play950TextColor;
@property(nonatomic, strong) NSColor* play950SubtitleColor;
@property(nonatomic, strong) NSColor* play950BottomBarColor;
@property(nonatomic, strong) NSImage* play950Icon;
@property(nonatomic, copy) NSString* play950Subtitle;
@property(nonatomic) CGFloat play950IconSize;
@property(nonatomic) CGFloat play950BottomBarHeight;
@end

@implementation PLAY950Button

- (void)setEnabled:(BOOL)enabled {
    [super setEnabled:enabled];
    self.needsDisplay = YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    const BOOL enabled = self.enabled;
    const BOOL pressed = self.cell.isHighlighted;
    NSColor* fill = self.play950FillColor ?: NSColor.controlColor;
    NSColor* stroke = self.play950StrokeColor;
    NSColor* text = self.play950TextColor ?: NSColor.controlTextColor;

    NSBezierPath* shape = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(self.bounds, 0.5, 0.5)
                                                          xRadius:5.0
                                                          yRadius:5.0];
    [[fill colorWithAlphaComponent:enabled ? (pressed ? 0.72 : 1.0) : 0.10] setFill];
    [shape fill];
    if (stroke) {
        [[stroke colorWithAlphaComponent:enabled ? 1.0 : 0.42] setStroke];
        shape.lineWidth = 1.0;
        [shape stroke];
    }

    if (self.play950BottomBarColor && self.play950BottomBarHeight > 0.0) {
        [NSGraphicsContext saveGraphicsState];
        [shape addClip];
        [[self.play950BottomBarColor colorWithAlphaComponent:enabled ? 1.0 : 0.30] setFill];
        const CGFloat barY = self.isFlipped
            ? NSHeight(self.bounds) - self.play950BottomBarHeight
            : 0.0;
        NSRectFill(NSMakeRect(0.0, barY, NSWidth(self.bounds), self.play950BottomBarHeight));
        [NSGraphicsContext restoreGraphicsState];
    }

    NSMutableParagraphStyle* paragraph = [[NSMutableParagraphStyle alloc] init];
    const BOOL hasIcon = self.play950Icon != nil;
    paragraph.alignment = hasIcon ? NSTextAlignmentLeft : NSTextAlignmentCenter;
    NSColor* titleColor = enabled ? text : [text colorWithAlphaComponent:0.42];
    const BOOL hasSubtitle = self.play950Subtitle.length > 0;
    paragraph.lineBreakMode = hasSubtitle ? NSLineBreakByTruncatingMiddle
                                           : NSLineBreakByTruncatingTail;
    NSDictionary* attributes = @{
        NSFontAttributeName: self.font ?: [NSFont systemFontOfSize:12.0],
        NSForegroundColorAttributeName: titleColor,
        NSKernAttributeName: hasSubtitle ? @0.45 : @1.2,
        NSParagraphStyleAttributeName: paragraph
    };
    const NSSize titleSize = [self.title sizeWithAttributes:attributes];
    const CGFloat iconSize = self.play950IconSize > 0.0 ? self.play950IconSize : 28.0;
    NSRect titleRect = NSInsetRect(self.bounds, 11.0, 0.0);
    if (hasIcon) {
        const NSRect iconRect = NSMakeRect(
            NSMinX(titleRect),
            std::floor(NSMidY(self.bounds) - iconSize / 2.0),
            iconSize,
            iconSize);
        [self.play950Icon drawInRect:iconRect
                            fromRect:NSZeroRect
                           operation:NSCompositingOperationSourceOver
                            fraction:enabled ? 1.0 : 0.38
                      respectFlipped:YES
                               hints:@{NSImageHintInterpolation: @(NSImageInterpolationHigh)}];
        titleRect.origin.x += iconSize + 10.0;
        titleRect.size.width -= iconSize + 10.0;
    }
    if (!hasSubtitle) {
        titleRect.origin.y = std::floor(NSMidY(self.bounds) - titleSize.height / 2.0);
        titleRect.size.height = std::ceil(titleSize.height);
        [self.title drawInRect:titleRect withAttributes:attributes];
        return;
    }

    NSMutableParagraphStyle* subtitleParagraph = [paragraph mutableCopy];
    subtitleParagraph.lineBreakMode = NSLineBreakByTruncatingTail;
    NSColor* subtitleColor = self.play950SubtitleColor
        ?: [text colorWithAlphaComponent:0.62];
    if (!enabled)
        subtitleColor = [subtitleColor colorWithAlphaComponent:0.42];
    NSDictionary* subtitleAttributes = @{
        NSFontAttributeName: [NSFont monospacedSystemFontOfSize:9.5
                                                        weight:NSFontWeightRegular],
        NSForegroundColorAttributeName: subtitleColor,
        NSKernAttributeName: @0.5,
        NSParagraphStyleAttributeName: subtitleParagraph
    };
    const NSSize subtitleSize = [self.play950Subtitle sizeWithAttributes:subtitleAttributes];
    const CGFloat stackGap = 2.0;
    const CGFloat stackHeight = titleSize.height + stackGap + subtitleSize.height;
    CGFloat titleY = 0.0;
    CGFloat subtitleY = 0.0;
    if (self.isFlipped) {
        titleY = std::floor(NSMidY(self.bounds) - stackHeight / 2.0);
        subtitleY = titleY + titleSize.height + stackGap;
    } else {
        subtitleY = std::floor(NSMidY(self.bounds) - stackHeight / 2.0);
        titleY = subtitleY + subtitleSize.height + stackGap;
    }
    NSRect mainRect = titleRect;
    mainRect.origin.y = titleY;
    mainRect.size.height = std::ceil(titleSize.height);
    NSRect subtitleRect = titleRect;
    subtitleRect.origin.y = subtitleY;
    subtitleRect.size.height = std::ceil(subtitleSize.height);
    [self.title drawInRect:mainRect withAttributes:attributes];
    [self.play950Subtitle drawInRect:subtitleRect withAttributes:subtitleAttributes];
}

@end

@interface PLAY950Panel : NSView
@property(nonatomic, strong) NSColor* play950FillColor;
@property(nonatomic, strong) NSColor* play950StrokeColor;
@property(nonatomic) CGFloat play950CornerRadius;
@end

@implementation PLAY950Panel

- (void)viewDidChangeEffectiveAppearance {
    [super viewDidChangeEffectiveAppearance];
    self.needsDisplay = YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    NSRect bounds = self.bounds;
    if (NSWidth(bounds) <= 1.0 || NSHeight(bounds) <= 1.0) {
        [self.play950FillColor setFill];
        NSRectFill(bounds);
        return;
    }
    NSBezierPath* shape = [NSBezierPath
        bezierPathWithRoundedRect:NSInsetRect(bounds, 0.5, 0.5)
                         xRadius:self.play950CornerRadius
                         yRadius:self.play950CornerRadius];
    [self.play950FillColor setFill];
    [shape fill];
    if (self.play950StrokeColor) {
        [self.play950StrokeColor setStroke];
        shape.lineWidth = 1.0;
        [shape stroke];
    }
}

@end

@interface PLAY950PopUpButton : NSPopUpButton
@property(nonatomic, strong) NSColor* play950FillColor;
@property(nonatomic, strong) NSColor* play950ForegroundColor;
@end

@implementation PLAY950PopUpButton

- (void)viewDidChangeEffectiveAppearance {
    [super viewDidChangeEffectiveAppearance];
    self.needsDisplay = YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    self.contentTintColor = self.play950ForegroundColor;
    self.layer.backgroundColor = self.play950FillColor.CGColor;
    [super drawRect:dirtyRect];
}

@end

@interface PLAY950UppercaseTextField : NSTextField
@end

@implementation PLAY950UppercaseTextField

- (void)setStringValue:(NSString*)stringValue {
    [super setStringValue:stringValue.uppercaseString];
}

@end

namespace {

using e45recordings::play950::Controller;

enum class Play950AppearanceMode : NSInteger {
    system = 0,
    light = 1,
    dark = 2
};

NSString* const play950AppearancePreferenceKey = @"PLAY950AppearanceMode";

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        std::array<char, 64> pattern {};
        std::strcpy(pattern.data(), "/tmp/play950-image.XXXXXX");
        const auto* created = mkdtemp(pattern.data());
        if (!created)
            throw std::runtime_error(std::string("could not create temporary directory: ") +
                                     std::strerror(errno));
        path_ = created;
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

std::vector<e45recordings::play950::content::LoadedProgram> loadImagePrograms(
    const std::filesystem::path& imagePath) {
    std::filesystem::path akaiUtil;
#if defined(PLAY950_BUNDLED_AKAIUTIL)
    Dl_info moduleInfo {};
    if (dladdr(reinterpret_cast<const void*>(&loadImagePrograms), &moduleInfo) != 0 &&
        moduleInfo.dli_fname) {
        const auto bundled = std::filesystem::path(moduleInfo.dli_fname)
                                 .parent_path().parent_path() / "Resources" / "akaiutil";
        if (std::filesystem::is_regular_file(bundled))
            akaiUtil = bundled;
    }
#endif
#if defined(PLAY950_AKAIUTIL_PATH)
    if (akaiUtil.empty() && std::filesystem::is_regular_file(PLAY950_AKAIUTIL_PATH))
        akaiUtil = PLAY950_AKAIUTIL_PATH;
#endif
    if (akaiUtil.empty())
        throw std::runtime_error(
            "AKAI Util is unavailable; reinstall PLAY950 or open a P9 directly");

    TemporaryDirectory output;
    @autoreleasepool {
        NSTask* task = [[NSTask alloc] init];
        task.executableURL = [NSURL fileURLWithPath:
            [NSString stringWithUTF8String:akaiUtil.c_str()]];
        task.arguments = @[@"-r", [NSString stringWithUTF8String:imagePath.c_str()]];

        NSPipe* input = [NSPipe pipe];
        NSPipe* outputPipe = [NSPipe pipe];
        task.standardInput = input;
        task.standardOutput = outputPipe;
        task.standardError = outputPipe;

        NSError* launchError = nil;
        if (![task launchAndReturnError:&launchError])
            throw std::runtime_error(launchError.localizedDescription.UTF8String);

        const auto commands = "lcd " + output.path().string() + "\ngetall\nq\n";
        [[input fileHandleForWriting]
            writeData:[NSData dataWithBytes:commands.data() length:commands.size()]];
        [[input fileHandleForWriting] closeFile];
        NSData* logData = [[outputPipe fileHandleForReading] readDataToEndOfFile];
        [task waitUntilExit];
        if (task.terminationStatus != 0) {
            NSString* log = [[NSString alloc] initWithData:logData
                                                  encoding:NSUTF8StringEncoding];
            throw std::runtime_error(log.length ? log.UTF8String : "AKAI Util failed");
        }
    }
    return e45recordings::play950::content::loadP9ProgramsInDirectory(output.path());
}

bool isImage(const std::filesystem::path& path) {
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return extension == ".img";
}

std::vector<NSInteger> releaseVersionComponents(NSString* value) {
    NSString* trimmed = [value stringByTrimmingCharactersInSet:
        NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if ([[trimmed lowercaseString] hasPrefix:@"v"])
        trimmed = [trimmed substringFromIndex:1];
    trimmed = [trimmed componentsSeparatedByCharactersInSet:
        [NSCharacterSet characterSetWithCharactersInString:@"-+"]].firstObject;

    std::vector<NSInteger> result;
    for (NSString* component in [trimmed componentsSeparatedByString:@"."]) {
        NSScanner* scanner = [NSScanner scannerWithString:component];
        NSInteger number = 0;
        if (![scanner scanInteger:&number] || !scanner.isAtEnd)
            return {};
        result.push_back(number);
    }
    return result;
}

bool isReleaseVersionNewer(NSString* remote, NSString* local) {
    const auto remoteParts = releaseVersionComponents(remote);
    const auto localParts = releaseVersionComponents(local);
    if (remoteParts.empty() || localParts.empty())
        return false;
    const auto count = std::max(remoteParts.size(), localParts.size());
    for (std::size_t index = 0; index < count; ++index) {
        const auto remotePart = index < remoteParts.size() ? remoteParts[index] : 0;
        const auto localPart = index < localParts.size() ? localParts[index] : 0;
        if (remotePart != localPart)
            return remotePart > localPart;
    }
    return false;
}

std::vector<std::string> stringVector(NSArray<NSString*>* values) {
    std::vector<std::string> result;
    result.reserve(values.count);
    for (NSString* value in values)
        result.emplace_back(value.fileSystemRepresentation);
    return result;
}

NSArray<NSString*>* stringArray(const std::vector<std::string>& values) {
    NSMutableArray<NSString*>* result = [NSMutableArray arrayWithCapacity:values.size()];
    for (const auto& value : values)
        [result addObject:[NSString stringWithUTF8String:value.c_str()]];
    return result;
}

NSColor* play950Color(unsigned int rgb, CGFloat alpha = 1.0) {
    return [NSColor colorWithSRGBRed:((rgb >> 16) & 0xff) / 255.0
                              green:((rgb >> 8) & 0xff) / 255.0
                               blue:(rgb & 0xff) / 255.0
                              alpha:alpha];
}

NSColor* play950AdaptiveColor(unsigned int darkRGB, unsigned int lightRGB,
                              CGFloat darkAlpha = 1.0, CGFloat lightAlpha = 1.0) {
    return [NSColor colorWithName:nil dynamicProvider:^NSColor*(NSAppearance* appearance) {
        const NSAppearanceName match = [appearance bestMatchFromAppearancesWithNames:@[
            NSAppearanceNameDarkAqua, NSAppearanceNameAqua
        ]];
        return [match isEqualToString:NSAppearanceNameDarkAqua]
            ? play950Color(darkRGB, darkAlpha)
            : play950Color(lightRGB, lightAlpha);
    }];
}

NSFont* play950Font(CGFloat size, NSFontWeight weight = NSFontWeightRegular) {
    size *= 1.25;
    NSArray<NSString*>* candidates = nil;
    if (weight >= NSFontWeightBold) {
        candidates = @[@"JetBrainsMono-Bold", @"JetBrains Mono Bold"];
    } else if (weight >= NSFontWeightMedium) {
        candidates = @[@"JetBrainsMono-Medium", @"JetBrains Mono Medium"];
    } else {
        candidates = @[@"JetBrainsMono-Regular", @"JetBrains Mono"];
    }
    for (NSString* name in candidates) {
        if (NSFont* font = [NSFont fontWithName:name size:size])
            return font;
    }
    NSFont* font = [NSFont monospacedSystemFontOfSize:size weight:weight];
    return font ?: [NSFont fontWithName:@"Menlo" size:size];
}

NSTextField* play950Label(NSString* text, NSRect frame, CGFloat size,
                          NSFontWeight weight, NSColor* color) {
    NSTextField* label = [NSTextField labelWithString:text.uppercaseString];
    label.frame = frame;
    label.font = play950Font(size, weight);
    label.textColor = color;
    label.lineBreakMode = NSLineBreakByTruncatingTail;
    label.allowsDefaultTighteningForTruncation = NO;
    return label;
}

NSTextField* play950TrackedLabel(NSString* text, NSRect frame, CGFloat size,
                                 NSFontWeight weight, CGFloat tracking,
                                 NSColor* color) {
    NSTextField* label = play950Label(text, frame, size, weight, color);
    label.attributedStringValue = [[NSAttributedString alloc]
        initWithString:text.uppercaseString
            attributes:@{NSFontAttributeName: play950Font(size, weight),
                         NSForegroundColorAttributeName: color,
                         NSKernAttributeName: @(tracking)}];
    return label;
}

NSView* play950Panel(NSRect frame, NSColor* fill, NSColor* stroke,
                     CGFloat cornerRadius = 6.0) {
    PLAY950Panel* panel = [[PLAY950Panel alloc] initWithFrame:frame];
    panel.wantsLayer = YES;
    panel.play950FillColor = fill;
    panel.play950StrokeColor = stroke;
    panel.play950CornerRadius = cornerRadius;
    return panel;
}

void stylePlay950Button(PLAY950Button* button, NSColor* fill, NSColor* foreground,
                        NSColor* stroke, NSColor* bottomBar = nil) {
    button.font = play950Font(11.0, NSFontWeightMedium);
    button.bordered = NO;
    button.focusRingType = NSFocusRingTypeNone;
    [button setButtonType:NSButtonTypeMomentaryChange];
    button.play950FillColor = fill;
    button.play950StrokeColor = stroke;
    button.play950TextColor = foreground;
    button.play950BottomBarColor = bottomBar;
    button.play950BottomBarHeight = bottomBar ? 4.0 : 0.0;
    button.play950IconSize = 34.0;
    button.wantsLayer = YES;
}

void stylePlay950Menu(NSPopUpButton* menu, NSColor* foreground, NSColor* fill) {
    menu.font = play950Font(12.0, NSFontWeightMedium);
    menu.bordered = NO;
    menu.focusRingType = NSFocusRingTypeNone;
    menu.wantsLayer = YES;
    menu.layer.cornerRadius = 6.0;
    menu.layer.masksToBounds = YES;
    if ([menu isKindOfClass:PLAY950PopUpButton.class]) {
        PLAY950PopUpButton* playMenu = (PLAY950PopUpButton*)menu;
        playMenu.play950ForegroundColor = foreground;
        playMenu.play950FillColor = fill;
    } else {
        menu.contentTintColor = foreground;
        menu.layer.backgroundColor = fill.CGColor;
    }
}

NSImage* play950ResourceImage(NSBundle* bundle, NSString* resource) {
    NSURL* url = [bundle URLForResource:resource withExtension:@"png"];
    return url ? [[NSImage alloc] initWithContentsOfURL:url] : nil;
}

NSImageView* play950ImageView(NSImage* image, NSRect frame) {
    NSImageView* view = [[NSImageView alloc] initWithFrame:frame];
    view.image = image;
    view.imageScaling = NSImageScaleProportionallyUpOrDown;
    view.imageAlignment = NSImageAlignCenter;
    view.wantsLayer = YES;
    view.layer.minificationFilter = kCAFilterTrilinear;
    return view;
}

void addPlay950GroupHeading(NSView* parent, NSString* text, NSRect frame,
                            NSColor* ink, NSColor* accent) {
    NSTextField* label = play950TrackedLabel(text,
        NSMakeRect(NSMinX(frame), NSMinY(frame), 112.0, NSHeight(frame)),
        10.5, NSFontWeightMedium, 2.4, ink);
    [parent addSubview:label];
    NSView* rule = play950Panel(
        NSMakeRect(NSMinX(frame) + 118.0, NSMinY(frame) + 6.0,
                   std::max<CGFloat>(0.0, NSWidth(frame) - 118.0), 3.0),
        accent, nil, 1.5);
    [parent addSubview:rule];
}

void addPlay950AccentBar(NSView* parent, NSRect frame, NSColor* accent) {
    [parent addSubview:play950Panel(frame, accent, nil, 1.5)];
}

} // namespace

@interface PLAY950ContentView : NSView
- (instancetype)initWithFrame:(NSRect)frame controller:(Controller*)controller;
- (void)reloadProgramMenu;
- (void)reloadRecentMenu;
- (void)updateToolButtonLabels;
- (void)checkForUpdates;
- (void)applyResolvedAppearance;
- (void)loadPath:(std::string)selectedPath reload:(BOOL)isReload;
- (void)loadPath:(std::string)selectedPath reload:(BOOL)isReload preferredProgram:(NSString*)preferredProgram;
@end

@implementation PLAY950ContentView {
    Controller* _controller;
    NSTextField* _status;
    NSTextField* _headerSource;
    PLAY950Button* _openButton;
    PLAY950Button* _reloadButton;
    PLAY950Button* _editorButton;
    PLAY950Button* _find950Button;
    PLAY950Button* _updateButton;
    NSURL* _availableReleaseURL;
    NSPopUpButton* _recentMenu;
    NSPopUpButton* _programMenu;
    NSPopUpButton* _midiReceiveMenu;
    NSPopUpButton* _basicMidiChannelMenu;
    NSPopUpButton* _pitchBendRangeMenu;
    NSPopUpButton* _themeMenu;
    NSView* _statusLight;
    NSColor* _canvasColor;
    NSColor* _borderColor;
    NSColor* _statusAccentColor;
}

- (instancetype)initWithFrame:(NSRect)frame controller:(Controller*)controller {
    self = [super initWithFrame:frame];
    if (!self)
        return nil;
    _controller = controller;
    _controller->addRef();
    [[NSDistributedNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(receiveLibraryLoad:)
               name:@"com.e45recordings.PLAY950.LoadContent"
             object:nil
 suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];

    NSInteger storedAppearanceMode = [[NSUserDefaults standardUserDefaults]
        integerForKey:play950AppearancePreferenceKey];
    if (storedAppearanceMode < 0 || storedAppearanceMode > 2)
        storedAppearanceMode = 0;
    const auto appearanceMode = static_cast<Play950AppearanceMode>(storedAppearanceMode);
    if (appearanceMode == Play950AppearanceMode::light)
        self.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    else if (appearanceMode == Play950AppearanceMode::dark)
        self.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    else
        self.appearance = nil;

    // Adaptive 950TOOLS palette. Colour remains reserved for rules, value bars,
    // active control surfaces and the approved product marks.
    NSColor* canvas = play950AdaptiveColor(0x0a0a0c, 0xf3f2ed);
    NSColor* header = play950AdaptiveColor(0x0e0e11, 0xfbfaf6);
    NSColor* slab = play950AdaptiveColor(0x17171c, 0xe7e5de);
    NSColor* slab2 = play950AdaptiveColor(0x202027, 0xdcd9d1);
    NSColor* rule = play950AdaptiveColor(0xffffff, 0x000000, 0.09, 0.10);
    NSColor* rule2 = play950AdaptiveColor(0xffffff, 0x000000, 0.18, 0.20);
    NSColor* ink = play950AdaptiveColor(0xf1f1f5, 0x17181b);
    NSColor* label = play950AdaptiveColor(0x8d939d, 0x505761);
    NSColor* unit = play950AdaptiveColor(0x5e636d, 0x737983);
    NSColor* red = play950AdaptiveColor(0xff2010, 0xd52a1c);
    NSColor* yellow = play950AdaptiveColor(0xffc400, 0xb77600);
    NSColor* blue = play950AdaptiveColor(0x3a53ff, 0x2648d8);
    _canvasColor = canvas;
    _borderColor = rule2;
    _statusAccentColor = yellow;

    self.wantsLayer = YES;
    self.layer.borderWidth = 1.0;
    self.layer.cornerRadius = 10.0;
    self.layer.masksToBounds = YES;
    [self applyResolvedAppearance];

    NSView* headerPanel = play950Panel(NSMakeRect(0, 436, 760, 64), header, nil, 0.0);
    [self addSubview:headerPanel];
    [self addSubview:play950Panel(NSMakeRect(0, 435, 760, 1), rule, nil, 0.0)];

    // Header: approved PLAY950 mark, tracked wordmark and source slot.
    NSBundle* bundle = [NSBundle bundleForClass:[self class]];
    NSImage* playLogo = play950ResourceImage(bundle, @"PLAY950Logo");
    NSImage* findLogo = play950ResourceImage(bundle, @"FIND950Logo");
    NSImage* editLogo = play950ResourceImage(bundle, @"EDIT950Logo");
    if (playLogo)
        [self addSubview:play950ImageView(playLogo, NSMakeRect(18, 448, 40, 40))];
    [self addSubview:play950TrackedLabel(@"PLAY950", NSMakeRect(70, 462, 110, 19),
        15.0, NSFontWeightBold, 3.4, ink)];
    [self addSubview:play950TrackedLabel(
        [NSString stringWithFormat:@"V%s · CLICKFIX3", PLAY950_VERSION],
        NSMakeRect(70, 447, 110, 12), 8.5, NSFontWeightRegular, 1.0, unit)];

    [self addSubview:play950Panel(NSMakeRect(184, 449, 558, 38), slab, nil, 6.0)];
    [self addSubview:play950TrackedLabel(@"PROGRAM", NSMakeRect(196, 463, 70, 13),
        9.0, NSFontWeightRegular, 1.8, label)];
    _headerSource = [[PLAY950UppercaseTextField alloc] initWithFrame:NSMakeRect(274, 457, 330, 20)];
    _headerSource.bezeled = NO;
    _headerSource.editable = NO;
    _headerSource.selectable = NO;
    _headerSource.drawsBackground = NO;
    _headerSource.font = play950Font(11.5, NSFontWeightMedium);
    _headerSource.textColor = ink;
    _headerSource.lineBreakMode = NSLineBreakByTruncatingTail;
    _headerSource.stringValue = @"NO PROGRAM LOADED";
    [self addSubview:_headerSource];
    [self addSubview:play950TrackedLabel(@"IMG · P9 · S9", NSMakeRect(620, 463, 110, 13),
        8.5, NSFontWeightRegular, 1.2, unit)];

    // SOURCE group.
    addPlay950GroupHeading(self, @"SOURCE", NSMakeRect(18, 407, 724, 15), ink, blue);

    _openButton = [[PLAY950Button alloc] initWithFrame:NSMakeRect(18, 353, 178, 42)];
    _openButton.target = self;
    _openButton.action = @selector(openContent:);
    _openButton.title = @"LOAD IMG OR P9";
    stylePlay950Button(_openButton, slab, ink, nil, blue);
    [self addSubview:_openButton];

    _recentMenu = [[PLAY950PopUpButton alloc] initWithFrame:NSMakeRect(208, 353, 322, 42)
                                                   pullsDown:YES];
    stylePlay950Menu(_recentMenu, ink, slab);
    [self addSubview:_recentMenu];
    addPlay950AccentBar(self, NSMakeRect(208, 353, 322, 4), blue);

    _reloadButton = [[PLAY950Button alloc] initWithFrame:NSMakeRect(542, 353, 200, 42)];
    _reloadButton.target = self;
    _reloadButton.action = @selector(reloadImage:);
    _reloadButton.title = @"RELOAD SOURCE";
    stylePlay950Button(_reloadButton, slab, ink, nil, blue);
    [self addSubview:_reloadButton];

    // PROGRAM group.
    addPlay950GroupHeading(self, @"PROGRAM", NSMakeRect(18, 319, 724, 15), ink, yellow);
    _programMenu = [[PLAY950PopUpButton alloc] initWithFrame:NSMakeRect(18, 265, 724, 44)
                                                    pullsDown:NO];
    _programMenu.target = self;
    _programMenu.action = @selector(selectProgram:);
    stylePlay950Menu(_programMenu, ink, slab);
    [self addSubview:_programMenu];
    addPlay950AccentBar(self, NSMakeRect(18, 265, 724, 4), yellow);
    [self reloadProgramMenu];

    // SYSTEM group.
    addPlay950GroupHeading(self, @"SYSTEM", NSMakeRect(18, 231, 350, 15), ink, blue);
    [self addSubview:play950Panel(NSMakeRect(18, 46, 350, 171), slab, nil, 6.0)];
    [self addSubview:play950TrackedLabel(@"SYSTEM STATUS", NSMakeRect(30, 190, 160, 14),
        9.5, NSFontWeightRegular, 1.6, label)];
    _statusLight = play950Panel(NSMakeRect(30, 164, 8, 8), yellow, nil, 4.0);
    _statusLight.layer.shadowOpacity = 0.60;
    _statusLight.layer.shadowRadius = 4.0;
    _statusLight.layer.shadowOffset = CGSizeZero;
    [self addSubview:_statusLight];
    _status = [[PLAY950UppercaseTextField alloc] initWithFrame:NSMakeRect(50, 105, 306, 73)];
    _status.bezeled = NO;
    _status.editable = NO;
    _status.selectable = NO;
    _status.drawsBackground = NO;
    _status.font = play950Font(10.5, NSFontWeightRegular);
    _status.textColor = ink;
    _status.lineBreakMode = NSLineBreakByWordWrapping;
    _status.maximumNumberOfLines = 4;
    _status.stringValue = [NSString stringWithUTF8String:_controller->statusText().c_str()];
    [self addSubview:_status];
    [self addSubview:play950Panel(NSMakeRect(30, 91, 326, 1), rule, nil, 0.0)];
    _themeMenu = [[PLAY950PopUpButton alloc] initWithFrame:NSMakeRect(30, 55, 172, 30)
                                                  pullsDown:NO];
    [_themeMenu addItemsWithTitles:@[@"THEME · SYSTEM", @"THEME · LIGHT", @"THEME · DARK"]];
    [_themeMenu selectItemAtIndex:static_cast<NSInteger>(appearanceMode)];
    _themeMenu.target = self;
    _themeMenu.action = @selector(changeAppearance:);
    _themeMenu.toolTip = @"Follow macOS appearance, or force PLAY950 light or dark.";
    stylePlay950Menu(_themeMenu, ink, slab2);
    [self addSubview:_themeMenu];
    _updateButton = [[PLAY950Button alloc] initWithFrame:NSMakeRect(216, 55, 140, 30)];
    _updateButton.target = self;
    _updateButton.action = @selector(openAvailableRelease:);
    _updateButton.title = @"UPDATE AVAILABLE ↗";
    _updateButton.font = play950Font(8.5, NSFontWeightMedium);
    _updateButton.hidden = YES;
    _updateButton.accessibilityLabel = @"Open the latest PLAY950 release on GitHub";
    stylePlay950Button(_updateButton, slab2, ink, rule2, blue);
    [self addSubview:_updateButton];

    // 950TOOLS group with the approved FIND950 and EDIT950 identities.
    addPlay950GroupHeading(self, @"950TOOLS", NSMakeRect(380, 231, 362, 15), ink, yellow);
    _find950Button = [[PLAY950Button alloc] initWithFrame:NSMakeRect(380, 151, 175, 66)];
    _find950Button.target = self;
    _find950Button.action = @selector(openFIND950:);
    _find950Button.title = @"BROWSE LIBRARY";
    _find950Button.play950Subtitle = @"OPEN FIND950 ↗";
    _find950Button.play950Icon = findLogo;
    stylePlay950Button(_find950Button, slab, ink, nil, blue);
    _find950Button.font = play950Font(8.6, NSFontWeightBold);
    _find950Button.play950SubtitleColor = label;
    _find950Button.play950IconSize = 38.0;
    [self addSubview:_find950Button];

    _editorButton = [[PLAY950Button alloc] initWithFrame:NSMakeRect(567, 151, 175, 66)];
    _editorButton.target = self;
    _editorButton.action = @selector(openInEditor:);
    _editorButton.title = @"EDIT THIS IMG";
    _editorButton.play950Subtitle = @"OPEN IN EDIT950 ↗";
    _editorButton.play950Icon = editLogo;
    stylePlay950Button(_editorButton, slab, ink, nil, yellow);
    _editorButton.font = play950Font(8.6, NSFontWeightBold);
    _editorButton.play950SubtitleColor = label;
    _editorButton.play950IconSize = 38.0;
    [self addSubview:_editorButton];

    // MIDI group: hardware-style receive mode and base channel plus bend range.
    addPlay950GroupHeading(self, @"MIDI", NSMakeRect(380, 119, 362, 15), ink, red);
    [self addSubview:play950Panel(NSMakeRect(380, 46, 362, 59), slab, nil, 6.0)];

    [self addSubview:play950TrackedLabel(@"RECEIVE", NSMakeRect(392, 87, 108, 12),
        8.5, NSFontWeightRegular, 1.2, label)];
    _midiReceiveMenu = [[PLAY950PopUpButton alloc] initWithFrame:NSMakeRect(392, 50, 108, 34)
                                                         pullsDown:NO];
    [_midiReceiveMenu addItemWithTitle:@"OMNI"];
    [_midiReceiveMenu addItemWithTitle:@"KG CHANNELS"];
    [_midiReceiveMenu selectItemAtIndex:_controller->midiOmni() ? 0 : 1];
    _midiReceiveMenu.target = self;
    _midiReceiveMenu.action = @selector(changeMidiReceive:);
    _midiReceiveMenu.toolTip = @"Omni plays by note range; KG Channels respects each P9 keygroup's channel offset.";
    stylePlay950Menu(_midiReceiveMenu, ink, slab2);
    [self addSubview:_midiReceiveMenu];

    [self addSubview:play950TrackedLabel(@"BASIC CH", NSMakeRect(507, 87, 108, 12),
        8.5, NSFontWeightRegular, 1.2, label)];
    _basicMidiChannelMenu = [[PLAY950PopUpButton alloc] initWithFrame:NSMakeRect(507, 50, 108, 34)
                                                              pullsDown:NO];
    for (int channel = 1; channel <= 16; ++channel) {
        [_basicMidiChannelMenu addItemWithTitle:[NSString stringWithFormat:@"CH %02d", channel]];
    }
    [_basicMidiChannelMenu selectItemAtIndex:_controller->basicMidiChannel() - 1];
    _basicMidiChannelMenu.target = self;
    _basicMidiChannelMenu.action = @selector(changeBasicMidiChannel:);
    _basicMidiChannelMenu.toolTip = @"S950 Basic MIDI Channel; P9 keygroup offsets are relative to this channel.";
    stylePlay950Menu(_basicMidiChannelMenu, ink, slab2);
    [self addSubview:_basicMidiChannelMenu];

    [self addSubview:play950TrackedLabel(@"BEND", NSMakeRect(622, 87, 108, 12),
        8.5, NSFontWeightRegular, 1.2, label)];
    _pitchBendRangeMenu = [[PLAY950PopUpButton alloc] initWithFrame:NSMakeRect(622, 50, 108, 34)
                                                            pullsDown:NO];
    for (int semitones = 1; semitones <= 12; ++semitones) {
        [_pitchBendRangeMenu addItemWithTitle:[NSString stringWithFormat:@"%d ST", semitones]];
    }
    [_pitchBendRangeMenu selectItemAtIndex:_controller->pitchBendRangeSemitones() - 1];
    _pitchBendRangeMenu.target = self;
    _pitchBendRangeMenu.action = @selector(changePitchBendRange:);
    stylePlay950Menu(_pitchBendRangeMenu, ink, slab2);
    [self addSubview:_pitchBendRangeMenu];
    addPlay950AccentBar(self, NSMakeRect(380, 46, 362, 4), red);

    [self reloadRecentMenu];
    const auto sourcePath = _controller->sourcePath();
    const bool hasImage = !sourcePath.empty() && isImage(sourcePath);
    _reloadButton.enabled = hasImage;
    _editorButton.enabled = hasImage;
    [self updateToolButtonLabels];
    if (!hasImage && _controller->programCount() > 0) {
        _reloadButton.toolTip = @"Open the source IMG once to reconnect this older Set.";
        _editorButton.toolTip = @"Open the source IMG once, then save the Set to retain its path.";
        _status.stringValue = [NSString stringWithFormat:
            @"%@ Source path unavailable; open the IMG once and resave the Set.",
            _status.stringValue];
    }
    [self checkForUpdates];
    return self;
}

- (void)viewDidChangeEffectiveAppearance {
    [super viewDidChangeEffectiveAppearance];
    [self applyResolvedAppearance];
}

- (void)applyResolvedAppearance {
    if (!_canvasColor || !_borderColor || !self.layer)
        return;
    [self.effectiveAppearance performAsCurrentDrawingAppearance:^{
        self.layer.backgroundColor = self->_canvasColor.CGColor;
        self.layer.borderColor = self->_borderColor.CGColor;
        if (self->_statusLight)
            self->_statusLight.layer.shadowColor = self->_statusAccentColor.CGColor;
    }];
    NSMutableArray<NSView*>* pending = [NSMutableArray arrayWithObject:self];
    while (pending.count > 0) {
        NSView* view = pending.lastObject;
        [pending removeLastObject];
        view.needsDisplay = YES;
        [pending addObjectsFromArray:view.subviews];
    }
}

- (void)changeAppearance:(id)sender {
    (void)sender;
    const NSInteger selected = std::max<NSInteger>(
        0, std::min<NSInteger>(2, _themeMenu.indexOfSelectedItem));
    [[NSUserDefaults standardUserDefaults] setInteger:selected
                                               forKey:play950AppearancePreferenceKey];
    const auto mode = static_cast<Play950AppearanceMode>(selected);
    if (mode == Play950AppearanceMode::light)
        self.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    else if (mode == Play950AppearanceMode::dark)
        self.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    else
        self.appearance = nil;
    [self applyResolvedAppearance];
}

- (void)checkForUpdates {
    NSURL* endpoint = [NSURL URLWithString:
        @"https://api.github.com/repos/richiewarburton/PLAY950/releases/latest"];
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:endpoint];
    request.timeoutInterval = 12.0;
    request.cachePolicy = NSURLRequestReloadRevalidatingCacheData;
    [request setValue:@"application/vnd.github+json" forHTTPHeaderField:@"Accept"];
    [request setValue:@"2022-11-28" forHTTPHeaderField:@"X-GitHub-Api-Version"];
    [request setValue:[NSString stringWithFormat:@"PLAY950/%s 950TOOLS", PLAY950_VERSION]
        forHTTPHeaderField:@"User-Agent"];

    __weak PLAY950ContentView* weakSelf = self;
    NSURLSessionDataTask* task = [NSURLSession.sharedSession
        dataTaskWithRequest:request
        completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
            if (error || !data)
                return;
            NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
            if (![httpResponse isKindOfClass:NSHTTPURLResponse.class] ||
                httpResponse.statusCode < 200 || httpResponse.statusCode >= 300)
                return;
            NSError* jsonError = nil;
            NSDictionary* payload = [NSJSONSerialization JSONObjectWithData:data
                                                                     options:0
                                                                       error:&jsonError];
            NSString* tagName = [payload isKindOfClass:NSDictionary.class]
                ? payload[@"tag_name"] : nil;
            NSString* page = [payload isKindOfClass:NSDictionary.class]
                ? payload[@"html_url"] : nil;
            NSURL* pageURL = page.length > 0 ? [NSURL URLWithString:page] : nil;
            NSString* localVersion = [NSString stringWithUTF8String:PLAY950_VERSION];
            if (jsonError || tagName.length == 0 || !pageURL ||
                !isReleaseVersionNewer(tagName, localVersion))
                return;

            dispatch_async(dispatch_get_main_queue(), ^{
                PLAY950ContentView* strongSelf = weakSelf;
                if (!strongSelf)
                    return;
                strongSelf->_availableReleaseURL = pageURL;
                strongSelf->_updateButton.title = [NSString stringWithFormat:
                    @"%@ AVAILABLE ↗", tagName.uppercaseString];
                strongSelf->_updateButton.toolTip = [NSString stringWithFormat:
                    @"Open the PLAY950 %@ release page on GitHub.", tagName];
                strongSelf->_updateButton.hidden = NO;
                strongSelf->_updateButton.needsDisplay = YES;
            });
        }];
    [task resume];
}

- (void)openAvailableRelease:(id)sender {
    (void)sender;
    if (_availableReleaseURL)
        [NSWorkspace.sharedWorkspace openURL:_availableReleaseURL];
}

- (void)updateToolButtonLabels {
    _find950Button.title = @"BROWSE LIBRARY";
    _find950Button.play950Subtitle = @"OPEN FIND950 ↗";
    _find950Button.toolTip = @"Browse the AKAI image library in FIND950.";
    _find950Button.accessibilityLabel = @"Browse library in FIND950";

    const auto sourcePath = _controller->sourcePath();
    const bool hasImage = !sourcePath.empty() && isImage(sourcePath);
    if (hasImage) {
        const auto path = std::filesystem::path(sourcePath);
        NSString* filename = [NSString stringWithUTF8String:
            path.filename().string().c_str()];
        _editorButton.title = [NSString stringWithFormat:@"EDIT %@",
            filename.uppercaseString];
        _editorButton.toolTip = [NSString stringWithFormat:
            @"Open %@ for editing in EDIT950.", filename];
        _editorButton.accessibilityLabel = [NSString stringWithFormat:
            @"Edit %@ in EDIT950", filename];
    } else {
        _editorButton.title = @"EDIT THIS IMG";
        _editorButton.toolTip = @"Load an IMG to open it for editing in EDIT950.";
        _editorButton.accessibilityLabel = @"Edit this image in EDIT950";
    }
    _editorButton.play950Subtitle = @"OPEN IN EDIT950 ↗";
    _find950Button.needsDisplay = YES;
    _editorButton.needsDisplay = YES;
}

- (void)changePitchBendRange:(id)sender {
    (void)sender;
    _controller->setPitchBendRangeSemitones(
        static_cast<int>(_pitchBendRangeMenu.indexOfSelectedItem) + 1);
}

- (void)changeMidiReceive:(id)sender {
    (void)sender;
    _controller->setMidiOmni(_midiReceiveMenu.indexOfSelectedItem == 0);
}

- (void)changeBasicMidiChannel:(id)sender {
    (void)sender;
    _controller->setBasicMidiChannel(
        static_cast<int>(_basicMidiChannelMenu.indexOfSelectedItem) + 1);
}

- (NSArray<NSString*>*)recentImagePaths {
    NSArray* stored = [[NSUserDefaults standardUserDefaults]
        stringArrayForKey:@"PLAY950RecentImages"];
    if (!stored) {
        stored = [[NSUserDefaults standardUserDefaults]
            stringArrayForKey:@"TRUE950RecentImages"];
    }
    return stored ?: @[];
}

- (void)rememberImagePath:(const std::string&)path {
    const auto recents = e45recordings::play950::workflow::rememberRecentImage(
        stringVector([self recentImagePaths]), path);
    [[NSUserDefaults standardUserDefaults] setObject:stringArray(recents)
                                              forKey:@"PLAY950RecentImages"];
    [self reloadRecentMenu];
}

- (void)reloadRecentMenu {
    [_recentMenu removeAllItems];
    [_recentMenu addItemWithTitle:@"RECENT IMAGES"];
    _recentMenu.lastItem.enabled = NO;
    NSFileManager* manager = NSFileManager.defaultManager;
    for (NSString* path in [self recentImagePaths]) {
        const BOOL exists = [manager fileExistsAtPath:path];
        NSString* title = exists ? path.lastPathComponent.uppercaseString
                                 : [NSString stringWithFormat:@"%@ — MISSING",
                                                              path.lastPathComponent.uppercaseString];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title
                                                     action:@selector(openRecentImage:)
                                              keyEquivalent:@""];
        item.target = self;
        item.representedObject = path;
        item.enabled = exists;
        [_recentMenu.menu addItem:item];
    }
    if ([self recentImagePaths].count > 0) {
        [_recentMenu.menu addItem:NSMenuItem.separatorItem];
        NSMenuItem* removeMissing = [[NSMenuItem alloc]
            initWithTitle:@"REMOVE MISSING ITEMS"
                   action:@selector(removeMissingRecents:)
            keyEquivalent:@""];
        removeMissing.target = self;
        [_recentMenu.menu addItem:removeMissing];
    }
}

- (void)reloadProgramMenu {
    [_programMenu removeAllItems];
    const auto count = _controller->programCount();
    if (count == 0) {
        [_programMenu addItemWithTitle:@"NO IMAGE PROGRAMS LOADED"];
        _headerSource.stringValue = @"NO PROGRAM LOADED";
        _programMenu.enabled = NO;
        return;
    }
    for (std::size_t index = 0; index < count; ++index) {
        const auto name = _controller->programDisplayName(index);
        NSString* displayName = [NSString stringWithUTF8String:name.c_str()];
        [_programMenu addItemWithTitle:displayName.uppercaseString];
    }
    [_programMenu selectItemAtIndex:static_cast<NSInteger>(_controller->selectedProgramIndex())];
    _headerSource.stringValue = _programMenu.titleOfSelectedItem ?: @"PROGRAM LOADED";
    _programMenu.enabled = count > 1;
}

- (void)selectProgram:(id)sender {
    (void)sender;
    const auto index = static_cast<std::size_t>(_programMenu.indexOfSelectedItem);
    if (_controller->selectProgram(index)) {
        _headerSource.stringValue = _programMenu.titleOfSelectedItem ?: @"PROGRAM LOADED";
        _status.stringValue = [NSString stringWithUTF8String:_controller->statusText().c_str()];
    } else {
        _status.stringValue = @"The processor is busy; try again.";
        [_programMenu selectItemAtIndex:
            static_cast<NSInteger>(_controller->selectedProgramIndex())];
    }
}

- (void)dealloc {
    [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
    _controller->release();
}

- (void)receiveLibraryLoad:(NSNotification*)notification {
    NSString* path = notification.userInfo[@"path"];
    NSString* program = notification.userInfo[@"program"];
    if (path.length == 0)
        return;
    [self loadPath:path.fileSystemRepresentation reload:NO preferredProgram:program];
}

- (void)openRecentImage:(NSMenuItem*)sender {
    NSString* path = sender.representedObject;
    if (path.length)
        [self loadPath:path.fileSystemRepresentation reload:NO];
}

- (void)removeMissingRecents:(id)sender {
    (void)sender;
    const auto retained = e45recordings::play950::workflow::removeMissingImages(
        stringVector([self recentImagePaths]));
    [[NSUserDefaults standardUserDefaults] setObject:stringArray(retained)
                                              forKey:@"PLAY950RecentImages"];
    [self reloadRecentMenu];
}

- (void)reloadImage:(id)sender {
    (void)sender;
    const auto path = _controller->sourcePath();
    if (!path.empty() && isImage(path))
        [self loadPath:path reload:YES];
}

- (void)openInEditor:(id)sender {
    (void)sender;
    const auto path = _controller->sourcePath();
    if (path.empty() || !isImage(path))
        return;
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    NSURL* application = [workspace
        URLForApplicationWithBundleIdentifier:@"com.e45recordings.EDIT950"];
    if (!application) {
        application = [workspace
            URLForApplicationWithBundleIdentifier:@"com.local.AKAIImageManager"];
    }
    if (!application) {
        NSArray<NSString*>* candidates = @[
            @"/Applications/EDIT950.app",
            @"/Applications/AKAI Image Manager.app"
        ];
        for (NSString* candidate in candidates) {
            if ([[NSFileManager defaultManager] fileExistsAtPath:candidate]) {
                application = [NSURL fileURLWithPath:candidate];
                break;
            }
        }
    }
    if (!application) {
        _status.stringValue = @"EDIT950 is not installed.";
        return;
    }
    NSURL* imageURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
    NSWorkspaceOpenConfiguration* configuration =
        [NSWorkspaceOpenConfiguration configuration];
    [[NSWorkspace sharedWorkspace]
        openURLs:@[imageURL]
        withApplicationAtURL:application
        configuration:configuration
        completionHandler:^(NSRunningApplication* applicationResult, NSError* error) {
            (void)applicationResult;
            if (error) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    self->_status.stringValue = [NSString stringWithFormat:
                        @"Could not open EDIT950: %@", error.localizedDescription];
                });
            }
        }];
}

- (void)openFIND950:(id)sender {
    (void)sender;
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    NSURL* application = [workspace
        URLForApplicationWithBundleIdentifier:@"com.e45recordings.FIND950"];
    if (!application) {
        application = [workspace
            URLForApplicationWithBundleIdentifier:@"com.e45recordings.S950LibraryBrowser"];
    }
    if (!application) {
        NSArray<NSString*>* candidates = @[
            @"/Applications/FIND950.app",
            @"/Applications/S950 Library Browser.app",
            [NSHomeDirectory() stringByAppendingPathComponent:
                @"Applications/FIND950.app"],
            [NSHomeDirectory() stringByAppendingPathComponent:
                @"Applications/S950 Library Browser.app"]
        ];
        for (NSString* path in candidates) {
            if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
                application = [NSURL fileURLWithPath:path];
                break;
            }
        }
    }
    if (!application) {
        _status.stringValue = @"FIND950 is not installed. Run its install-browser-app script once.";
        return;
    }
    NSWorkspaceOpenConfiguration* configuration =
        [NSWorkspaceOpenConfiguration configuration];
    configuration.activates = YES;
    [workspace openApplicationAtURL:application
                      configuration:configuration
                  completionHandler:^(NSRunningApplication* runningApplication, NSError* error) {
        (void)runningApplication;
        if (error) {
            dispatch_async(dispatch_get_main_queue(), ^{
                self->_status.stringValue = [NSString stringWithFormat:
                    @"Could not open FIND950: %@", error.localizedDescription];
            });
        }
    }];
}

- (void)loadPath:(std::string)selectedPath reload:(BOOL)isReload {
    [self loadPath:selectedPath reload:isReload preferredProgram:nil];
}

- (void)loadPath:(std::string)selectedPath reload:(BOOL)isReload preferredProgram:(NSString*)preferredProgram {
    NSString* requestedProgram = [preferredProgram copy];
    _openButton.enabled = NO;
    _reloadButton.enabled = NO;
    _editorButton.enabled = NO;
    _recentMenu.enabled = NO;
    _status.stringValue = isReload ? @"Reloading IMG…" : @"Loading…";

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        try {
            const auto path = std::filesystem::path(selectedPath);
            auto programs = isImage(path)
                ? loadImagePrograms(path)
                : std::vector<e45recordings::play950::content::LoadedProgram> {
                      e45recordings::play950::content::loadP9Program(path)};
            auto sharedPrograms = std::make_shared<
                std::vector<e45recordings::play950::content::LoadedProgram>>(
                    std::move(programs));
            const std::string sourceName = path.filename().string();
            dispatch_async(dispatch_get_main_queue(), ^{
                const bool sent = isReload
                    ? self->_controller->reloadAvailablePrograms(std::move(*sharedPrograms))
                    : self->_controller->setAvailablePrograms(
                          std::move(*sharedPrograms), sourceName, selectedPath);
                self->_status.stringValue = sent
                    ? [NSString stringWithUTF8String:self->_controller->statusText().c_str()]
                    : @"The processor could not prepare this program; previous content retained.";
                if (sent && isImage(path))
                    [self rememberImagePath:selectedPath];
                if (sent && requestedProgram.length > 0) {
                    for (std::size_t index = 0; index < self->_controller->programCount(); ++index) {
                        const auto displayName = self->_controller->programDisplayName(index);
                        NSString* candidate = [NSString stringWithUTF8String:displayName.c_str()];
                        if ([candidate caseInsensitiveCompare:requestedProgram] == NSOrderedSame) {
                            self->_controller->selectProgram(index);
                            break;
                        }
                    }
                    self->_status.stringValue = [NSString stringWithUTF8String:
                        self->_controller->statusText().c_str()];
                }
                [self reloadProgramMenu];
                const auto currentPath = self->_controller->sourcePath();
                const bool hasImage = !currentPath.empty() && isImage(currentPath);
                self->_openButton.enabled = YES;
                self->_reloadButton.enabled = hasImage;
                self->_editorButton.enabled = hasImage;
                [self updateToolButtonLabels];
                self->_recentMenu.enabled = YES;
            });
        } catch (const std::exception& error) {
            const std::string message = error.what();
            dispatch_async(dispatch_get_main_queue(), ^{
                self->_status.stringValue = [NSString stringWithFormat:
                    isReload ? @"Reload failed; previous content retained: %s"
                             : @"Load failed: %s", message.c_str()];
                const auto currentPath = self->_controller->sourcePath();
                const bool hasImage = !currentPath.empty() && isImage(currentPath);
                self->_openButton.enabled = YES;
                self->_reloadButton.enabled = hasImage;
                self->_editorButton.enabled = hasImage;
                [self updateToolButtonLabels];
                self->_recentMenu.enabled = YES;
            });
        }
    });
}

- (void)openContent:(id)sender {
    (void)sender;
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowsMultipleSelection = NO;
    panel.allowedContentTypes = @[[UTType typeWithFilenameExtension:@"img"],
                                  [UTType typeWithFilenameExtension:@"p9"]];
    panel.message = @"Choose an AKAI IMG or P9 program";

    void (^completionHandler)(NSModalResponse) = ^(NSModalResponse response) {
        if (response != NSModalResponseOK || !panel.URL)
            return;
        const std::string selectedPath = panel.URL.fileSystemRepresentation;
        [self loadPath:selectedPath reload:NO];
    };

    // Ableton hosts plug-in editors in floating windows. A separate app-modal
    // open panel can therefore appear behind the editor. Attach it to the editor
    // as a sheet so AppKit owns the ordering and disables the parent correctly.
    if (self.window)
        [panel beginSheetModalForWindow:self.window completionHandler:completionHandler];
    else
        [panel beginWithCompletionHandler:completionHandler];
}

@end

namespace e45recordings::play950 {
namespace {

class EditorView final : public Steinberg::CPluginView {
public:
    explicit EditorView(Controller& controller)
        : CPluginView(&initialRect), controller_(controller) {
        controller_.addRef();
    }

    ~EditorView() override {
        controller_.release();
    }

    Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override {
        return Steinberg::FIDStringsEqual(type, Steinberg::kPlatformTypeNSView)
                   ? Steinberg::kResultTrue
                   : Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) override {
        if (!parent || isPlatformTypeSupported(type) != Steinberg::kResultTrue)
            return Steinberg::kInvalidArgument;
        const auto result = CPluginView::attached(parent, type);
        if (result != Steinberg::kResultOk)
            return result;
        NSView* parentView = (__bridge NSView*)parent;
        contentView_ = [[PLAY950ContentView alloc]
            initWithFrame:NSMakeRect(0, 0, initialRect.getWidth(), initialRect.getHeight())
               controller:&controller_];
        contentView_.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [parentView addSubview:contentView_];
        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API removed() override {
        [contentView_ removeFromSuperview];
        contentView_ = nil;
        return CPluginView::removed();
    }

    Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* newSize) override {
        const auto result = CPluginView::onSize(newSize);
        if (newSize && contentView_)
            contentView_.frame = NSMakeRect(0, 0, newSize->getWidth(), newSize->getHeight());
        return result;
    }

private:
    inline static Steinberg::ViewRect initialRect {0, 0, 760, 500};
    Controller& controller_;
    __strong NSView* contentView_ = nil;
};

} // namespace

Steinberg::IPlugView* createPlay950Editor(Controller& controller) {
    return new EditorView(controller);
}

} // namespace e45recordings::play950
