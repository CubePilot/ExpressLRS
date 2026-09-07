#pragma once
#include <CubePilotFW/ReceiverTypes.h>
struct ElrsCfRadioContext { uint32_t session,revision;bool linked,selected,configured,armed,available; };
// Main-loop lifecycle. Runtime is the peripheral boundary; start/reconfigure and
// stop run only in step(), outside transport/HSEM and local critical sections.
template<class Runtime> class ElrsCfRadio {
public:
    explicit ElrsCfRadio(Runtime &runtime):runtime_(runtime) {}
    void update(const ElrsCfRadioContext &context) {
        context_=context;
        if(state_==Starting || state_==Running || state_==Held || state_==Changing) {
            if(!ownedContext() || (state_==Starting && context_.armed))fail(CF_RX_RADIO_CONTROL_LOST);
            else if(state_!=Starting && runtime_.fault())fail(runtime_.fault());
            else if(!context_.configured || context_.revision!=lease_.revision) {
                if(state_==Starting)fail(CF_RX_RADIO_CONTROL_LOST);
                else state_=Held;
            }
        }
    }
    bool receive(const cfRxRadioControl_t &message) {
        if(!context_.linked || !message.session || message.session!=context_.session ||
            !message.revision || !message.grant || message.fault)return false;
        if(message.phase==CF_RX_RADIO_REVOKE) {
            // A new challenged session may revoke the previous session's lease.
            // If a local lease is active, another grant cannot revoke it.
            if(state_!=Idle && lease_.grant && message.grant!=lease_.grant)return false;
            if(state_==Starting || running() || state_==Changing)failedSession_=lease_.session;
            lease_=message;revoked_=true;state_=Stopping;return true;
        }
        if(message.phase!=CF_RX_RADIO_GRANT || !authorized() || message.revision!=context_.revision)return false;
        if(state_==Idle) {
            if(context_.armed || context_.session==failedSession_ ||
                (lastSession_==message.session && message.grant<=lastGrant_))return false;
            lease_=message;fault_=0;revoked_=false;state_=Starting;return true;
        }
        if(!sameGrant(message,lease_))return false;
        if(state_==Running && message.revision==lease_.revision)return true;
        if(state_==Held) { lease_=message;state_=Changing;return true; }
        return state_==Starting || state_==Changing;
    }
    void step() {
        if(state_==Starting || state_==Changing) {
            const uint8_t result=state_==Starting ? runtime_.start(lease_.session) : runtime_.reconfigure();
            if(result)fail(result);
            else state_=Running;
        }
        if(state_==Stopping && runtime_.stop())state_=Quiet;
    }
    bool ready() const { return state_==Running && authorized() && context_.revision==lease_.revision && !runtime_.fault(); }
    // Binding/configuration may keep using the radio while RC remains inhibited.
    bool running() const { return (state_==Running || state_==Held) && ownedContext() && !runtime_.fault(); }
    uint8_t fault() const { return fault_; }
    bool message(cfRxRadioControl_t *out) const {
        if(!out || !context_.linked)return false;
        if(state_==Idle && authorized() && !context_.armed && context_.session!=failedSession_) {
            *out={context_.session,context_.revision,0,CF_RX_RADIO_REQUEST,0};return true;
        }
        if(lease_.session!=context_.session)return false;
        *out=lease_;out->fault=0;
        if(ready())out->phase=CF_RX_RADIO_READY;
        else if(state_==Quiet && revoked_)out->phase=CF_RX_RADIO_QUIESCENT;
        else if(state_==Quiet && fault_) { out->phase=CF_RX_RADIO_FAULT;out->fault=fault_; }
        else return false;
        return true;
    }
    void sent(const cfRxRadioControl_t &message) {
        if(state_==Quiet && revoked_ && message.phase==CF_RX_RADIO_QUIESCENT && sameGrant(message,lease_) &&
            message.revision==lease_.revision) {
            lastSession_=lease_.session;lastGrant_=lease_.grant;state_=Idle;
        }
    }
private:
    enum State { Idle,Starting,Running,Held,Changing,Stopping,Quiet } state_=Idle;
    Runtime &runtime_;
    ElrsCfRadioContext context_{};
    cfRxRadioControl_t lease_{};
    uint32_t failedSession_=0,lastSession_=0,lastGrant_=0;
    uint8_t fault_=0;
    bool revoked_=false;
    bool authorized() const {
        return context_.session && context_.revision && context_.linked && context_.selected &&
            context_.configured && context_.available;
    }
    bool ownedContext() const {
        return context_.linked && context_.selected && context_.available && context_.session==lease_.session;
    }
    static bool sameGrant(const cfRxRadioControl_t &a,const cfRxRadioControl_t &b) {
        return a.session==b.session && a.grant==b.grant;
    }
    void fail(uint8_t fault) { failedSession_=lease_.session;fault_=fault;state_=Stopping; }
};
