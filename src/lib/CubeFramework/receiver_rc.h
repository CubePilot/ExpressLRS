#pragma once
#include <CubePilotFW/ReceiverTypes.h>

// Fixed latest-frame state. The adapter serializes access with its local PAL
// critical section; encoding and transport happen after copying a snapshot.
class ElrsCfRcPublisher {
public:
    static constexpr uint32_t CHANNEL_UNSET=0xffff, CHANNEL_EMPTY=0;
    static constexpr uint32_t MAX_AGE_US=20000; // Queue residence, not the RF failsafe timeout.
    void configure(uint32_t session,uint32_t revision,bool ready);
    bool publish(bool available,bool modelMatch,bool inhibited,const uint32_t *channels,uint32_t nowUs);
    bool snapshot(uint32_t nowUs,cfRxFrame_t *out);
    void sent(uint32_t sequence);
    uint32_t drops() const { return drops_; }
private:
    cfRxFrame_t frame_{};
    uint32_t session_=0,revision_=0,sequence_=0,stagedUs_=0,drops_=0;
    bool ready_=false,pending_=false;
};
