#include "receiver_rc.h"
void ElrsCfRcPublisher::configure(uint32_t session,uint32_t revision,bool ready)
{
    if (session!=session_) { sequence_=0;drops_=0; }
    if (session!=session_ || revision!=revision_ || !ready) pending_=false;
    session_=session;revision_=revision;ready_=ready && session && revision;
}
bool ElrsCfRcPublisher::publish(bool available,bool modelMatch,bool inhibited,const uint32_t *channels,uint32_t nowUs)
{
    if (!ready_ || !modelMatch || inhibited) { pending_=false;return false; }
    if (!available) return false;
    if (!channels) { pending_=false;return false; }
    for (unsigned i=0;i<16;i++) if (channels[i]>2047 && channels[i]!=CHANNEL_UNSET) { pending_=false;return false; }
    if (pending_) ++drops_;
    frame_.session=session_;frame_.revision=revision_;frame_.sequence=++sequence_;
    frame_.flags=CF_RX_VALID|CF_RX_MODEL_MATCH;
    for (unsigned i=0;i<16;i++) frame_.channels[i]=channels[i]==CHANNEL_UNSET ? CHANNEL_EMPTY : channels[i];
    stagedUs_=nowUs;pending_=true;return true;
}
bool ElrsCfRcPublisher::snapshot(uint32_t nowUs,cfRxFrame_t *out)
{
    if (!pending_ || !out) return false;
    const uint32_t age=nowUs-stagedUs_;
    // An ISR may publish after a caller sampled its poll timestamp. Defer
    // that frame rather than interpreting a negative age as a long stall.
    if (age>=0x80000000U) return false;
    if (age>=MAX_AGE_US) { pending_=false;++drops_;return false; }
    *out=frame_;return true;
}
void ElrsCfRcPublisher::sent(uint32_t sequence)
{
    // A timer ISR may have published another frame while this copy was encoded.
    if (pending_ && frame_.sequence==sequence) pending_=false;
}
