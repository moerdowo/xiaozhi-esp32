#pragma once
#include <cstdint>
enum DeviceState { kDeviceStateIdle, kDeviceStateListening, kDeviceStateSpeaking };

struct AudioService {
    uint8_t in = 0, out = 0;
    uint8_t GetInputLevel() const { return in; }
    uint8_t GetOutputLevel() const { return out; }
};
struct Application {
    DeviceState state = kDeviceStateIdle;
    AudioService audio;
    static Application& GetInstance() { static Application a; return a; }
    DeviceState GetDeviceState() const { return state; }
    AudioService& GetAudioService() { return audio; }
};
