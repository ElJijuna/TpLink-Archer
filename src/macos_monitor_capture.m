#import <CoreWLAN/CoreWLAN.h>
#import <Foundation/Foundation.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

static volatile sig_atomic_t interrupted = 0;
static void stop_capture(int signo) { (void)signo; interrupted = 1; }

int main(int argc, const char **argv) {
    @autoreleasepool {
        BOOL inspect = argc == 2 && strcmp(argv[1], "--inspect") == 0;
        BOOL capture = argc == 3 && strcmp(argv[1], "--capture-channel9") == 0;
        if (!inspect && !capture) {
            fprintf(stderr, "Usage: %s --inspect | --capture-channel9 NEW_DIRECTORY\n", argv[0]);
            return 2;
        }
        CWInterface *wifi = [[CWWiFiClient sharedWiFiClient] interfaceWithName:@"en0"];
        if (!wifi) { fprintf(stderr, "en0 Wi-Fi unavailable\n"); return 1; }
        CWChannel *original = [wifi wlanChannel];
        CWChannel *target = nil;
        for (CWChannel *candidate in [wifi supportedWLANChannels]) {
            if (candidate.channelNumber == 9 && candidate.channelWidth == kCWChannelWidth20MHz &&
                candidate.channelBand == kCWChannelBand2GHz) target = candidate;
        }
        printf("en0: current_channel=%lu channel9_20MHz_supported=%s\n",
               (unsigned long)original.channelNumber, target ? "yes" : "no");
        if (inspect) return target ? 0 : 1;
        if (geteuid() != 0 || !target || !original) {
            fprintf(stderr, "Requires sudo, supported channel9 and known original channel\n"); return 1;
        }
        NSString *directory = [[NSString stringWithUTF8String:argv[2]] stringByStandardizingPath];
        NSError *error = nil;
        if (mkdir(directory.fileSystemRepresentation, 0755) != 0) {
            perror("New output directory required"); return 1;
        }
        NSString *path = [directory stringByAppendingPathComponent:@"monitor-channel9.pcap"];
        signal(SIGINT, stop_capture); signal(SIGTERM, stop_capture);
        int result = 1;
        NSTask *task = [[NSTask alloc] init];
        @try {
            [wifi disassociate];
            if (![wifi setWLANChannel:target error:&error] || [wifi wlanChannel].channelNumber != 9) {
                fprintf(stderr, "Channel9 not confirmed: %s\n", error.description.UTF8String);
            } else {
                task.executableURL = [NSURL fileURLWithPath:@"/usr/sbin/tcpdump"];
                task.arguments = @[@"-i", @"en0", @"-I", @"-y", @"IEEE802_11_RADIO",
                                   @"-s", @"2048", @"-U", @"-w", path,
                                   @"type mgt or type ctl"];
                if ([task launchAndReturnError:&error]) {
                    fprintf(stderr, "Passive capture: channel9/20MHz, maximum20 seconds; no Archer TX\n");
                    for (int i = 0; i < 200 && task.running && !interrupted; ++i) usleep(100000);
                    if (task.running) kill(task.processIdentifier, SIGINT);
                    for (int i = 0; i < 30 && task.running; ++i) usleep(100000);
                    if (task.running) kill(task.processIdentifier, SIGKILL);
                    [task waitUntilExit];
                    result = task.terminationStatus == 0 && !interrupted ? 0 : 1;
                    printf("capture_exit=%d file=%s\n", task.terminationStatus, path.UTF8String);
                } else fprintf(stderr, "tcpdump launch failed: %s\n", error.description.UTF8String);
            }
        } @finally {
            if (task.running) { kill(task.processIdentifier, SIGKILL); [task waitUntilExit]; }
            error = nil;
            BOOL restored = [wifi setWLANChannel:original error:&error];
            printf("original_channel_restore=%s\n", restored ? "yes" : "failed");
            if (!restored) { fprintf(stderr, "%s\n", error.description.UTF8String); result = 1; }
            fprintf(stderr, "Reconnect the Mac Wi-Fi network if it did not reconnect automatically.\n");
        }
        return result;
    }
}
