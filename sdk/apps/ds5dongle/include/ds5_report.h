//
// Created by awalol on 2026/9/30.
//

#ifndef SDK_DS5_REPORT_H
#define SDK_DS5_REPORT_H

#include "asm/cpu.h"

/*
 * DualSense payload layouts shared by USB and Bluetooth code.
 * Offsets below are relative to the payload, excluding the report ID and any
 * Bluetooth transport header/CRC. Transport-specific lengths must be handled
 * by the caller; sizeof(SetStateData) includes the reserved tail.
 *
 * C enums do not have a fixed byte size. Wire fields use uint8_t and the enum
 * constants below. Packed bitfields follow the little-endian pi32v2 ABI;
 * these definitions are not a portable wire decoder for other ABIs.
 */

typedef enum PowerState {
    Discharging         = 0x00, // Use PowerPercent
    Charging            = 0x01, // Use PowerPercent
    Complete            = 0x02, // PowerPercent not valid? assume 100%?
    AbnormalVoltage     = 0x0A, // PowerPercent not valid?
    AbnormalTemperature = 0x0B, // PowerPercent not valid?
    ChargingError       = 0x0F  // PowerPercent not valid?
} PowerState;

typedef enum Direction {
    North = 0,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest,
    None = 8
} Direction;

typedef struct __attribute__((packed)) TouchFingerData { // 4
    /*0.0*/ uint32_t Index : 7;
    /*0.7*/ uint32_t NotTouching : 1;
    /*1.0*/ uint32_t FingerX : 12;
    /*2.4*/ uint32_t FingerY : 12;
} TouchFingerData;

typedef struct __attribute__((packed)) TouchData { // 9
    /*0*/ TouchFingerData Finger[2];
    /*8*/ uint8_t Timestamp;
} TouchData;

typedef struct __attribute__((packed)) USBGetStateData { // 63
    /* 0  */ uint8_t LeftStickX;
    /* 1  */ uint8_t LeftStickY;
    /* 2  */ uint8_t RightStickX;
    /* 3  */ uint8_t RightStickY;
    /* 4  */ uint8_t TriggerLeft;
    /* 5  */ uint8_t TriggerRight;
    /* 6  */ uint8_t SeqNo; // always 0x01 on BT
    /* 7.0*/ uint8_t DPad : 4; // Direction
    /* 7.4*/ uint8_t ButtonSquare : 1;
    /* 7.5*/ uint8_t ButtonCross : 1;
    /* 7.6*/ uint8_t ButtonCircle : 1;
    /* 7.7*/ uint8_t ButtonTriangle : 1;
    /* 8.0*/ uint8_t ButtonL1 : 1;
    /* 8.1*/ uint8_t ButtonR1 : 1;
    /* 8.2*/ uint8_t ButtonL2 : 1;
    /* 8.3*/ uint8_t ButtonR2 : 1;
    /* 8.4*/ uint8_t ButtonCreate : 1;
    /* 8.5*/ uint8_t ButtonOptions : 1;
    /* 8.6*/ uint8_t ButtonL3 : 1;
    /* 8.7*/ uint8_t ButtonR3 : 1;
    /* 9.0*/ uint8_t ButtonHome : 1;
    /* 9.1*/ uint8_t ButtonPad : 1;
    /* 9.2*/ uint8_t ButtonMute : 1;
    /* 9.3*/ uint8_t UNK1 : 1; // appears unused
    /* 9.4*/ uint8_t ButtonLeftFunction : 1; // DualSense Edge
    /* 9.5*/ uint8_t ButtonRightFunction : 1; // DualSense Edge
    /* 9.6*/ uint8_t ButtonLeftPaddle : 1; // DualSense Edge
    /* 9.7*/ uint8_t ButtonRightPaddle : 1; // DualSense Edge
    /*10  */ uint8_t UNK2; // appears unused
    /*11  */ uint32_t UNK_COUNTER; // Linux driver calls this reserved, tools leak calls the 2 high bytes "random"
    /*15  */ int16_t AngularVelocityX;
    /*17  */ int16_t AngularVelocityZ;
    /*19  */ int16_t AngularVelocityY;
    /*21  */ int16_t AccelerometerX;
    /*23  */ int16_t AccelerometerY;
    /*25  */ int16_t AccelerometerZ;
    /*27  */ uint32_t SensorTimestamp;
    /*31  */ int8_t Temperature; // reserved2 in Linux driver
    /*32  */ TouchData Touch;
    /*41.0*/ uint8_t TriggerRightStopLocation: 4; // trigger stop can be a range from 0 to 9 (F/9.0 for Apple interface)
    /*41.4*/ uint8_t TriggerRightStatus: 4;
    /*42.0*/ uint8_t TriggerLeftStopLocation: 4;
    /*42.4*/ uint8_t TriggerLeftStatus: 4; // 0 feedbackNoLoad
                                       // 1 feedbackLoadApplied
                                       // 0 weaponReady
                                       // 1 weaponFiring
                                       // 2 weaponFired
                                       // 0 vibrationNotVibrating
                                       // 1 vibrationIsVibrating
    /*43  */ uint32_t HostTimestamp; // mirrors data from report write
    /*47.0*/ uint8_t TriggerRightEffect: 4; // Active trigger effect, previously we thought this was status max
    /*47.4*/ uint8_t TriggerLeftEffect: 4;  // 0 for reset and all other effects
                                        // 1 for feedback effect
                                        // 2 for weapon effect
                                        // 3 for vibration
    /*48  */ uint32_t DeviceTimeStamp;
    /*52.0*/ uint8_t PowerPercent : 4; // 0x00-0x0A
    /*52.4*/ uint8_t Power : 4; // PowerState
    /*53.0*/ uint8_t PluggedHeadphones : 1;
    /*53.1*/ uint8_t PluggedMic : 1;
    /*53.2*/ uint8_t MicMuted: 1; // Mic muted by powersave/mute command
    /*53.3*/ uint8_t PluggedUsbData : 1;
    /*53.4*/ uint8_t PluggedUsbPower : 1; // appears that this cannot be 1 if PluggedUsbData is 1
    /*53.5*/ uint8_t UsbPowerOnBT : 1; // appears this is only 1 if BT connected and USB powered
    /*53.6*/ uint8_t DockDetect : 1;
    /*53.7*/ uint8_t PluggedUnk : 1;
    /*54.0*/ uint8_t PluggedExternalMic : 1; // Is external mic active (automatic in mic auto mode)
    /*54.1*/ uint8_t HapticLowPassFilter : 1; // Is the Haptic Low-Pass-Filter active?
    /*54.2*/ uint8_t PluggedUnk3 : 6;
    /*55  */ uint8_t AesCmac[8];
} USBGetStateData;

typedef enum MuteLight {
    Off = 0,
    On,
    Breathing,
    DoNothing, // literally nothing, this input is ignored,
               // though it might be a faster blink in other versions
    NoAction4,
    NoAction5,
    NoAction6,
    NoAction7 = 7
} MuteLight;

typedef enum LightBrightness {
    Bright = 0,
    Mid,
    Dim,
    NoAction3
    /* NoAction4..NoAction7 share the same values defined in MuteLight.
     * C enum constants share one scope, so they cannot be declared twice. */
} LightBrightness;

typedef enum LightFadeAnimation {
    Nothing = 0,
    FadeIn, // from black to blue
    FadeOut // from blue to black
} LightFadeAnimation;

typedef struct __attribute__((packed)) SetStateData { // 63 = 47 control bytes + 16 reserved bytes
    /*    */ // Report Set Flags
    /*    */ // These flags are used to indicate what contents from this report should be processed
    /* 0.0*/ uint8_t EnableRumbleEmulation: 1; // Suggest halving rumble strength
    /* 0.1*/ uint8_t UseRumbleNotHaptics:   1; //
    /*    */
    /* 0.2*/ uint8_t AllowRightTriggerFFB: 1; // Enable setting RightTriggerFFB
    /* 0.3*/ uint8_t AllowLeftTriggerFFB: 1;  // Enable setting LeftTriggerFFB
    /*    */
    /* 0.4*/ uint8_t AllowHeadphoneVolume: 1; // Enable setting VolumeHeadphones
    /* 0.5*/ uint8_t AllowSpeakerVolume: 1;   // Enable setting VolumeSpeaker
    /* 0.6*/ uint8_t AllowMicVolume: 1;       // Enable setting VolumeMic
    /*    */
    /* 0.7*/ uint8_t AllowAudioControl: 1; // Enable setting AudioControl section
    /* 1.0*/ uint8_t AllowMuteLight: 1;    // Enable setting MuteLightMode
    /* 1.1*/ uint8_t AllowAudioMute: 1;    // Enable setting MuteControl section
    /*    */
    /* 1.2*/ uint8_t AllowLedColor: 1; // Enable RGB LED section
    /*    */
    /* 1.3*/ uint8_t ResetLights: 1; // Release the LEDs from Wireless firmware control
    /*    */                         // When in wireless mode this must be signaled to control LEDs
    /*    */                         // This cannot be applied during the BT pair animation.
    /*    */                         // SDL2 waits until the SensorTimestamp value is >= 10200000
    /*    */                         // before pulsing this bit once.
    /*    */
    /* 1.4*/ uint8_t AllowPlayerIndicators: 1; // Enable setting PlayerIndicators section
    /* 1.5*/ uint8_t AllowHapticLowPassFilter: 1; // Enable HapticLowPassFilter
    /* 1.6*/ uint8_t AllowMotorPowerLevel: 1; // MotorPowerLevel reductions for trigger/haptic
    /* 1.7*/ uint8_t AllowAudioControl2: 1; // Enable setting AudioControl2 section
    /*    */
    /* 2  */ uint8_t RumbleEmulationRight; // emulates the light weight
    /* 3  */ uint8_t RumbleEmulationLeft; // emulated the heavy weight
    /*    */
    /* 4  */ uint8_t VolumeHeadphones; // max 0x7f
    /* 5  */ uint8_t VolumeSpeaker; // PS5 appears to only use the range 0x3d-0x64
    /* 6  */ uint8_t VolumeMic; // not linear, seems to max at 64, 0 is fully muted only in chat mode
    /*    */
    /*    */ // AudioControl
    /* 7.0*/ uint8_t MicSelect: 2; // 0 Auto
    /*    */                       // 1 Internal Only
    /*    */                       // 2 External Only
    /*    */                       // 3 Unclear, sets external mic flag but might use internal mic, do test
    /* 7.2*/ uint8_t EchoCancelEnable: 1;
    /* 7.3*/ uint8_t NoiseCancelEnable: 1;
    /* 7.4*/ uint8_t OutputPathSelect: 2; // 0 L_R_X
    /*    */                              // 1 L_L_X
    /*    */                              // 2 L_L_R
    /*    */                              // 3 X_X_R
    /* 7.6*/ uint8_t InputPathSelect: 2;  // 0 CHAT_ASR
    /*    */                              // 1 CHAT_CHAT
    /*    */                              // 2 ASR_ASR
    /*    */                              // 3 Does Nothing, invalid
    /*    */
    /* 8  */ uint8_t MuteLightMode; // MuteLight
    /*    */
    /*    */ // MuteControl
    /* 9.0*/ uint8_t TouchPowerSave: 1;
    /* 9.1*/ uint8_t MotionPowerSave: 1;
    /* 9.2*/ uint8_t HapticPowerSave: 1; // AKA BulletPowerSave
    /* 9.3*/ uint8_t AudioPowerSave: 1;
    /* 9.4*/ uint8_t MicMute: 1;
    /* 9.5*/ uint8_t SpeakerMute: 1;
    /* 9.6*/ uint8_t HeadphoneMute: 1;
    /* 9.7*/ uint8_t HapticMute: 1; // AKA BulletMute
    /*    */
    /*10  */ uint8_t RightTriggerFFB[11];
    /*21  */ uint8_t LeftTriggerFFB[11];
    /*32  */ uint32_t HostTimestamp; // mirrored into report read
    /*    */
    /*    */ // MotorPowerLevel
    /*36.0*/ uint8_t RumbleMotorPowerReduction : 4; // 0x0-0x7 (no 0x8?) Applied in 12.5% reductions
    /*36.4*/ uint8_t TriggerMotorPowerReduction : 4;  // 0x0-0xA
    /*    */
    /*    */ // AudioControl2
    /*37.0*/ uint8_t SpeakerCompPreGain: 3; // additional speaker volume boost
    /*37.3*/ uint8_t BeamformingEnable: 1; // Probably for MIC given there's 2, might be more bits, can't find what it does
    /*37.4*/ uint8_t UnkAudioControl2: 4; // some of these bits might apply to the above
    /*    */
    /*38.0*/ uint8_t AllowLightBrightnessChange: 1; // LED_BRIHTNESS_CONTROL
    /*38.1*/ uint8_t AllowColorLightFadeAnimation: 1; // LIGHTBAR_SETUP_CONTROL
    /*38.2*/ uint8_t EnableImprovedRumbleEmulation: 1; // Use instead of EnableRumbleEmulation
                                                   // requires FW >= 0x0224
                                                   // No need to halve rumble strength
    /*38.3*/ uint8_t UseRumbleNotHaptics2: 1; // 在 NinjaGaiden4 出现，与 UseRumbleNotHaptics 工作效果相同
    /*38.4*/ uint8_t UNKBITC: 4; // unused
    /*    */
    /*39.0*/ uint8_t HapticLowPassFilter: 1;
    /*39.1*/ uint8_t UNKBIT: 7;
    /*    */
    /*40  */ uint8_t UNKBYTE; // previous notes suggested this was HLPF, was probably off by 1
    /*    */
    /*41  */ uint8_t LightFadeAnimation; // LightFadeAnimation
    /*42  */ uint8_t LightBrightness; // LightBrightness
    /*    */
    /*    */ // PlayerIndicators
    /*    */ // These bits control the white LEDs under the touch pad.
    /*    */ // Note the reduction in functionality for later revisions.
    /*    */ // Generation 0x03 - Full Functionality
    /*    */ // Generation 0x04 - Mirrored Only
    /*    */ // Suggested detection: (HardwareInfo & 0x00FFFF00) == 0X00000400
    /*    */ //
    /*    */ // Layout used by PS5:
    /*    */ // 0x04 - -x- -  Player 1
    /*    */ // 0x06 - x-x -  Player 2
    /*    */ // 0x15 x -x- x  Player 3
    /*    */ // 0x1B x x-x x  Player 4
    /*    */ // 0x1F x xxx x  Player 5* (Unconfirmed)
    /*    */ //
    /*    */ //                        // HW 0x03 // HW 0x04
    /*43.0*/ uint8_t PlayerLight1 : 1; // x --- - // x --- x
    /*43.1*/ uint8_t PlayerLight2 : 1; // - x-- - // - x-x -
    /*43.2*/ uint8_t PlayerLight3 : 1; // - -x- - // - -x- -
    /*43.3*/ uint8_t PlayerLight4 : 1; // - --x - // - x-x -
    /*43.4*/ uint8_t PlayerLight5 : 1; // - --- x // x --- x
    /*43.5*/ uint8_t PlayerLightFade: 1; // if low player lights fade in, if high player lights instantly change
    /*43.6*/ uint8_t PlayerLightUNK : 2;
    /*    */
    /*    */ // RGB LED
    /*44  */ uint8_t LedRed;
    /*45  */ uint8_t LedGreen;
    /*46  */ uint8_t LedBlue;
    /*47  */ uint8_t reserved[16]; // Transport padding; not part of the 47-byte control block
} SetStateData;

void ds5_report_feature_update(u8 report_id, const u8* data, u8 size);
u8 ds5_report_feature_copy(u8 report_id, u8* dst, u8 size);
void ds5_report_state_update(const u8* data, u8 len);
u8 ds5_report_state_copy(u8* dst, u8 len);
void ds5_report_state_reset();
void ds5_report_send_state(const SetStateData* state);
void ds5_report_send_haptics(const s8* data);
void ds5_report_send_speaker_haptics(const u8* speaker,const s8* haptics);
void ds5_report_send_state_speaker_haptics(const u8* speaker,const s8* haptics);

#endif //SDK_DS5_REPORT_H
