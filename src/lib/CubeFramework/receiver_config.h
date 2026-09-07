#pragma once
#include <CubePilotFW/ReceiverTypes.h>
#include <stdint.h>

struct ElrsCfProposal
{
    uint32_t transaction, revision;
    cfRxSettings_t settings;
    bool volatileBinding;
};

// Protocol/state only: no EEPROM, radio, interrupt or shared-memory operations.
// The M4 loop owns this object; ISR-originated edits are staged by the adapter.
class ElrsCfConfigClient
{
public:
    void reset(uint32_t session);
    void authorize(bool enabled, bool armed, uint32_t lastControlUs, bool linked);

    void compiledDomain(uint8_t domain) { domain_ = domain; }

    cfRxResult_e apply(uint32_t transaction, uint32_t revision, uint32_t persistedRevision,
                       const cfRxSettings_t &settings, uint32_t nowUs);
    cfRxResult_e request(const cfRxSettings_t &settings, bool volatileBinding, uint32_t nowUs);
    bool proposal(uint32_t nowUs, ElrsCfProposal *out);
    void result(uint32_t transaction, cfRxResult_e result, uint32_t revision, uint32_t persistedRevision);
    cfRxResult_e bind(bool administered, uint32_t nowUs);
    cfRxResult_e bindUid(const uint8_t *uid, uint32_t nowUs);
    cfRxResult_e cancel(uint32_t nowUs);
    cfRxResult_e returnLoan(uint32_t nowUs);
    bool ready(uint32_t nowUs) const;
    cfRxResult_e canChange(uint32_t nowUs) const;

    bool binding() const { return binding_; }

    bool pending() const { return pending_; }

    uint32_t session() const { return session_; }

    uint32_t revision() const { return revision_; }

    uint32_t persistedRevision() const { return persistedRevision_; }

    const cfRxSettings_t &settings() const { return settings_; }

    cfRxResult_e lastResult() const { return lastResult_; }

private:
    cfRxResult_e authorization(uint32_t nowUs) const;
    cfRxResult_e validate(const cfRxSettings_t &settings) const;
    cfRxSettings_t settings_{};
    ElrsCfProposal proposal_{};
    uint32_t session_ = 0, revision_ = 0, persistedRevision_ = 0, applyTransaction_ = 0;
    uint32_t nextRequest_ = 0, confirmedRevision_ = 0, confirmedPersisted_ = 0, lastControlUs_ = 0, lastSend_ = 0;
    bool linked_ = false, enabled_ = false, armed_ = true, configured_ = false, pending_ = false;
    bool confirmed_ = false, blocked_ = false, binding_ = false, administered_ = false;
    uint8_t sends_ = 0, domain_ = 0;
    cfRxResult_e lastResult_ = CF_RX_STALE;
};
