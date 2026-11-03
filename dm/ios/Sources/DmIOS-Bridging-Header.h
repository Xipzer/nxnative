// The Objective-C functions Launcher.swift calls (CloudSaves.h). project.yml points SWIFT_OBJC_BRIDGING_HEADER here.
#import "CloudSaves.h"
// InGameSettings.swift: settings changed while the game runs (dm/host/dm_live.h) and the frame-rate counter (front_ios.m)
void dm_live_set(const char *k, const char *v);
const char *dm_live_getenv(const char *k);
void dm_front_set_fps(int on);
