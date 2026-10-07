//
// Created by awalol on 2026/10/6.
//

#define LOG_TAG "[DS5_STATE]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"
#include "ds5_report.h"
#include "ds5_state.h"

static u8 spk_vol = 100; // 0-100
static u8 mic_vol = 48;  // 0-48

static SetStateData state = {
    .AllowHeadphoneVolume = true,
    .AllowSpeakerVolume = true,
    .AllowMicVolume = true,
    .SpeakerCompPreGain = 1
};

// spk: 0-100, mic:0-48, ignore: 255
void ds5_state_set_vol(const u8 spk, const u8 mic) {
    if (spk != 255) {
        spk_vol = spk;
    }
    if (mic != 255) {
        mic_vol = mic;
    }
}

void ds5_state_get_vol(u8 *spk, u8 *mic) {
    if (spk) {
        *spk = spk_vol;
    }
    if (mic) {
        *mic = mic_vol;
    }
}

void ds5_state_copy_volume_state(u8* buf,u8 len) {
    state.VolumeSpeaker = state.VolumeHeadphones = spk_vol;
    state.VolumeMic = mic_vol;
    memcpy(buf,&state,MIN(len,sizeof(SetStateData)));
}

