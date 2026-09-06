#include "receiver_config.h"
#include <cstring>

namespace {
bool newer(uint32_t a,uint32_t b) { const uint32_t d=a-b;return d && d<0x80000000U; }
bool emptyUid(const uint8_t *uid) { const uint8_t empty[6]={};return !memcmp(uid,empty,6); }
}
void ElrsCfConfigClient::reset(uint32_t session)
{
    const uint8_t domain=domain_;
    *this=ElrsCfConfigClient();session_=session;domain_=domain;
}
void ElrsCfConfigClient::authorize(bool enabled,bool armed,uint32_t lastControlUs,bool linked)
{
    enabled_=enabled;armed_=armed;lastControlUs_=lastControlUs;linked_=linked;
}
cfRxResult_e ElrsCfConfigClient::authorization(uint32_t nowUs) const
{
    if (!enabled_) return CF_RX_UNSUPPORTED;
    if (armed_) return CF_RX_ARMED;
    if (!session_ || !linked_ || uint32_t(nowUs-lastControlUs_)>=250000U) return CF_RX_STALE;
    return CF_RX_OK;
}
cfRxResult_e ElrsCfConfigClient::canChange(uint32_t nowUs) const
{
    const auto auth=authorization(nowUs);
    if (auth!=CF_RX_OK) return auth;
    if (!configured_ || blocked_) return CF_RX_STALE;
    if (pending_) return CF_RX_BUSY;
    return CF_RX_OK;
}
bool ElrsCfConfigClient::ready(uint32_t nowUs) const
{
    // Arming only prevents configuration mutations; it must not stop RC.
    return configured_ && linked_ && enabled_ && uint32_t(nowUs-lastControlUs_)<250000U &&
        !pending_ && !blocked_ && !binding_;
}
cfRxResult_e ElrsCfConfigClient::validate(const cfRxSettings_t &s) const
{
    if (s.bindStorage>3 || s.antennaMode>2 || s.forceTelemetryOff>1 || s.lockOnFirstConnection>1 ||
        s.teamraceChannel<5 || s.teamraceChannel>15 || s.teamracePosition>7 || s.initialRate>31 ||
        s.telemetryPower>8 || s.domain>1) return CF_RX_INVALID;
    // SX1281 board: ten compiled rates, 10mW and MatchTX, switched diversity.
    if (s.initialRate>=10 || (s.telemetryPower!=0 && s.telemetryPower!=8) || s.domain!=domain_) return CF_RX_UNSUPPORTED;
    return CF_RX_OK;
}
cfRxResult_e ElrsCfConfigClient::apply(uint32_t transaction,uint32_t revision,uint32_t persistedRevision,
    const cfRxSettings_t &settings,uint32_t nowUs)
{
    if (!transaction || !revision || !persistedRevision) return CF_RX_STALE;
    if (configured_ && transaction==applyTransaction_) {
        return revision==revision_ && persistedRevision==persistedRevision_ && !memcmp(&settings,&settings_,sizeof(settings)) ? CF_RX_OK : CF_RX_INVALID;
    }
    const auto auth=authorization(nowUs);
    if (auth!=CF_RX_OK) return auth;
    if (blocked_) return CF_RX_STALE;
    const auto valid=validate(settings);
    if (valid!=CF_RX_OK) return valid;
    if (configured_ && (!newer(transaction,applyTransaction_) || !newer(revision,revision_))) return CF_RX_STALE;
    if (pending_) {
        if (!confirmed_) return CF_RX_PENDING;
        cfRxSettings_t expected=proposal_.settings;
        if (!proposal_.volatileBinding && expected.bindStorage==1 && settings_.bindStorage==1)
            memcpy(expected.boundUid,settings_.boundUid,6);
        if (revision!=confirmedRevision_ || persistedRevision!=confirmedPersisted_ || memcmp(&settings,&expected,sizeof(settings))) return CF_RX_STALE;
    }
    settings_=settings;revision_=revision;persistedRevision_=persistedRevision;applyTransaction_=transaction;
    configured_=true;pending_=confirmed_=binding_=administered_=false;lastResult_=CF_RX_OK;
    return CF_RX_OK;
}
cfRxResult_e ElrsCfConfigClient::request(const cfRxSettings_t &settings,bool volatileBinding,uint32_t nowUs)
{
    const auto auth=canChange(nowUs);
    if (auth!=CF_RX_OK) return auth;
    const auto valid=validate(settings);
    if (valid!=CF_RX_OK) return valid;
    if (volatileBinding) {
        auto check=settings;memcpy(check.boundUid,settings_.boundUid,6);
        if (settings_.bindStorage!=1 || memcmp(&check,&settings_,sizeof(check))) return CF_RX_INVALID;
    }
    if (!memcmp(&settings,&settings_,sizeof(settings))) return CF_RX_OK;
    if (nextRequest_==0x7fffffffU) return CF_RX_STALE; // New session required before the peer-command namespace.
    proposal_.settings=settings;
    if (!volatileBinding && settings.bindStorage==1) memset(proposal_.settings.boundUid,0,6);
    proposal_.volatileBinding=volatileBinding;proposal_.revision=revision_;
    proposal_.transaction=++nextRequest_;
    pending_=true;confirmed_=false;sends_=0;lastResult_=CF_RX_PENDING;
    return CF_RX_PENDING;
}
bool ElrsCfConfigClient::proposal(uint32_t nowUs,ElrsCfProposal *out)
{
    if (!out || !pending_) return false;
    if (confirmed_) {
        if (uint32_t(nowUs-lastSend_)>=1000000U) {
            pending_=false;blocked_=true;lastResult_=CF_RX_TIMEOUT;
        }
        return false;
    }
    if (sends_ && uint32_t(nowUs-lastSend_)<250000U) return false;
    if (sends_==4) {
        pending_=false;blocked_=true;lastResult_=CF_RX_TIMEOUT;return false;
    }
    ++sends_;lastSend_=nowUs;
    if (authorization(nowUs)!=CF_RX_OK) return false;
    *out=proposal_;return true;
}
void ElrsCfConfigClient::result(uint32_t transaction,cfRxResult_e result,uint32_t revision,uint32_t persistedRevision)
{
    if (!pending_ || transaction!=proposal_.transaction || result==CF_RX_PENDING) return;
    if (result==CF_RX_OK) {
        if (!newer(revision,proposal_.revision) ||
            (proposal_.volatileBinding ? persistedRevision!=persistedRevision_ : persistedRevision!=revision)) return;
        confirmed_=true;confirmedRevision_=revision;confirmedPersisted_=persistedRevision;
    } else {
        pending_=false;blocked_=result==CF_RX_SAVE_FAILED || result==CF_RX_TIMEOUT;lastResult_=result;
    }
}
cfRxResult_e ElrsCfConfigClient::bind(bool administered,uint32_t nowUs)
{
    const auto auth=canChange(nowUs);
    if (auth!=CF_RX_OK) return auth;
    if (settings_.bindStorage==3 && !administered) return CF_RX_UNSUPPORTED;
    binding_=true;administered_=administered;return CF_RX_OK;
}
cfRxResult_e ElrsCfConfigClient::bindUid(const uint8_t *uid,uint32_t nowUs)
{
    if (!uid || !binding_ || (settings_.bindStorage==3 && !administered_)) return CF_RX_INVALID;
    auto candidate=settings_;
    memcpy(candidate.boundUid,uid,6);
    if (settings_.bindStorage==0 || (settings_.bindStorage!=1 && emptyUid(settings_.homeUid))) memcpy(candidate.homeUid,uid,6);
    const auto result=request(candidate,settings_.bindStorage==1,nowUs);
    if (result==CF_RX_OK) binding_=administered_=false;
    return result;
}
cfRxResult_e ElrsCfConfigClient::cancel(uint32_t nowUs)
{
    const auto auth=canChange(nowUs);
    if (auth!=CF_RX_OK) return auth;
    binding_=administered_=false;return CF_RX_OK;
}
cfRxResult_e ElrsCfConfigClient::returnLoan(uint32_t nowUs)
{
    if (settings_.bindStorage!=2) return CF_RX_UNSUPPORTED;
    auto candidate=settings_;memcpy(candidate.boundUid,candidate.homeUid,6);
    return request(candidate,false,nowUs);
}
