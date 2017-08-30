#include "amigadiskviz.h"

// Global variables
int caps_flags = DI_LOCK_DENVAR|DI_LOCK_DENNOISE|DI_LOCK_NOISE|DI_LOCK_UPDATEFD|DI_LOCK_TYPE|DI_LOCK_OVLBIT;
char *ipfFileName;
char *topFilename;
char *bottomFilename;;
static struct CapsVersionInfo cvi;
struct CapsImageInfo ci;
struct CapsDateTimeExt *cdt;
FILE *ipfFile;
long ipfLen;
void *ipfData;
int ipfType;
int ipfDrive;
int ipfTracks;
struct CapsTrackInfoT2 *tracks;

void caps_init() {
    void *capsImg = dlopen("capsimg.so", RTLD_NOW|RTLD_LOCAL);

    if (!capsImg) {
        printf("ERROR: Unable to load capsimg.so\n");
        exit(1);
    }

    pCAPSInit = (CAPSINIT) dlsym(capsImg, "CAPSInit");
    if (!pCAPSInit) {
        // We'll check if the first one worked, to make sure we are vaguely sane. No point checking all of these
        printf("ERROR: Unable to load CAPSInit\n");
        exit(1);
    }
    pCAPSAddImage = (CAPSADDIMAGE) dlsym(capsImg, "CAPSAddImage");
    pCAPSLockImageMemory = (CAPSLOCKIMAGEMEMORY) dlsym(capsImg, "CAPSLockImageMemory");
    pCAPSUnlockImage = (CAPSUNLOCKIMAGE) dlsym(capsImg, "CAPSUnlockImage");
    pCAPSLoadImage = (CAPSLOADIMAGE) dlsym(capsImg, "CAPSLoadImage");
    pCAPSGetImageInfo = (CAPSGETIMAGEINFO) dlsym(capsImg, "CAPSGetImageInfo");
    pCAPSLockTrack = (CAPSLOCKTRACK) dlsym(capsImg, "CAPSLockTrack");
    pCAPSUnlockTrack = (CAPSUNLOCKTRACK) dlsym(capsImg, "CAPSUnlockTrack");
    pCAPSUnlockAllTracks = (CAPSUNLOCKALLTRACKS) dlsym(capsImg, "CAPSUnlockAllTracks");
    pCAPSGetVersionInfo = (CAPSGETVERSIONINFO) dlsym(capsImg, "CAPSGetVersionInfo");
    pCAPSGetInfo = (CAPSGETINFO) dlsym(capsImg, "CAPSGetInfo");
    pCAPSSetRevolution = (CAPSSETREVOLUTION) dlsym(capsImg, "CAPSSetRevolution");
    pCAPSGetImageTypeMemory = (CAPSGETIMAGETYPEMEMORY) dlsym(capsImg, "CAPSGetImageTypeMemory");

    pCAPSGetVersionInfo(&cvi, 0);
    printf("CAPS library version %d.%d\n", cvi.release, cvi.revision);

}

void caps_loadfile() {
    ipfFile = fopen(ipfFileName, "r");
    if (fseek(ipfFile, 0, SEEK_END) != 0) {
        printf("ERROR: Unable to seek to the end of %s\n", ipfFileName);
        exit(1);
    }
    ipfLen = ftell(ipfFile);
    fseek(ipfFile, 0, SEEK_SET);

    ipfData = malloc(ipfLen);
    fread(ipfData, ipfLen, 1, ipfFile);

    printf("Read %ld bytes from %s\n", ipfLen, ipfFileName);

    ipfType = pCAPSGetImageTypeMemory(ipfData, ipfLen);
    switch(ipfType) {
        case citError:
        case citUnknown:
        case citKFStream:
        case citDraft:
            printf("ERROR: Unsupported CAPS image type: %d\n", ipfType);
            exit(1);
            break;
        default:
            printf("CAPS Image type: %d\n", ipfType);
            break;
    }

    ipfDrive = pCAPSAddImage();
    int ret = pCAPSLockImageMemory(ipfDrive, ipfData, ipfLen, 0);
    //free(ipfData);

    if (ret != imgeOk) {
        printf("ERROR: Unable to read CAPS data\n");
        exit(1);
    }
    printf("CAPSLockImageMemory() returned %d\n", ret);

    ret = pCAPSGetImageInfo(&ci, ipfDrive);

    ipfTracks = (ci.maxcylinder - ci.mincylinder + 1) * (ci.maxhead - ci.minhead + 1);
    printf("Number of tracks: %d\n", ipfTracks);

    ret = pCAPSLoadImage(ipfDrive, caps_flags);
    cdt = &ci.crdt;
    printf("Date info: %d.%d.%d %02d:%d:%d\n", cdt->day, cdt->month, cdt->year, cdt->hour, cdt->min, cdt->sec);
}

void mfmcopy(uae_u16 *mfm, uae_u8 *data, int len) {
    int memlen = (len + 7) / 8;
    for (int i = 0; i < memlen; i+=2) {
        if (i + 1 < memlen)
            *mfm++ = (data[i] << 8) + data[i + 1];
        else
            *mfm++ = (data[i] << 8);
    }
}

void load(struct CapsTrackInfoT2 *ci2, int track) {
    ci2->type = 1;
    if (pCAPSLockTrack((PCAPSTRACKINFO)ci2, ipfDrive, track / 2, track & 1, caps_flags) != imgeOk) {
        printf("ERROR: Unable to lock track %d\n", track);
        exit(1);
    }
}

struct CapsTrackInfoT2 caps_loadtrack(int track) {
    struct CapsTrackInfoT2 ci2;
    struct CapsRevolutionInfo pinfo;

    load(&ci2, track);

    pCAPSGetInfo(&pinfo, ipfDrive, track/2, track&1, cgiitRevolution, 0);

    printf("Track %03d: ", track);
    printf("Length: %d ", ci2.tracklen);
    printf("Gap Offset: %d ", ci2.overlap >= 0 ? ci2.overlap : -1);
    printf("\n");

    return ci2;
}

void write_side(char *filename, int maxTrackLen, int numTracks, int offset) {
    FILE *fp = fopen(filename, "w");
    int padding = 0;
    int padChar[1];
    padChar[0] = 0;
    if (!fp) {
        printf("ERROR: Unable to open %s for writing\n", filename);
        exit(1);
    }
    for (int i = offset; i < numTracks / 2; i+=2) {
        struct CapsTrackInfoT2 track = tracks[i];
        fwrite(track.trackbuf, track.tracklen, 1, fp);
        padding = maxTrackLen - track.tracklen;
        if (padding > 0) {
            fwrite(padChar, 1, padding, fp);
        }
    }
    fclose(fp);
}

int main(int argc, char *argv[]) {
    // Initialise loading capsimg.so
    caps_init();

    // Check and store our arguments
    if (argc < 4) {
        printf("ERROR: Usage: amigaviz foo.ipf top.data bottom.data\n");
        exit(1);
    }
    ipfFileName = argv[1];
    topFilename = argv[2];
    bottomFilename = argv[3];

    // Load the IPF file into CAPS
    caps_loadfile();

    // Allocate storage for the tracks
    tracks = malloc(sizeof(struct CapsTrackInfoT2) * ipfTracks);

    int maxTrackLen = 0;

    capsTrack *aTrack;
    for (int curTrack = 0; curTrack < ipfTracks; curTrack++) {
        int trackLen;
        tracks[curTrack] = caps_loadtrack(curTrack);
        trackLen = tracks[curTrack].tracklen;

        if (trackLen > maxTrackLen) maxTrackLen = trackLen;
    }

    write_side(bottomFilename, maxTrackLen, ipfTracks, 0);
    write_side(topFilename, maxTrackLen, ipfTracks, 1);

    printf("Maximum track length: %d\n", maxTrackLen);

    return 0;
}
