#ifdef CUBERACER_M4
#include "receiver_link.h"
#include "receiver_config.h"
#include <Arduino.h>
#include <CubePilotFW/ReceiverEndpoint.h>
#include <CubePilotFW/ReceiverLayout.h>
#include <CubePilotFW/ReceiverCodec.h>
#include <CubePilotFW/PAL.h>
#include <cstring>
#include <CubePilotFW/pal/HSEM.h>
using namespace CubePilotFW;
namespace {
struct BootRecord { uint32_t magic,value,inverse; };
static_assert(sizeof(BootRecord)==12);
volatile BootRecord record __attribute__((section(".noinit.cuberacer_boot")));
constexpr uint32_t BOOT_MAGIC=0x43525831;
ReceiverEndpoint endpoint;
ElrsCfConfigClient client;
struct Guard {
    Guard() { cubefw_pal_criticalEnter(); }
    ~Guard() { cubefw_pal_criticalExit(); }
};
uint32_t lastCapabilities=0;
uint8_t rateHint=255;
bool ackPending=false,runtimeRefresh=false,lastCommandPending=false;
uint32_t lastCommandTransaction=0,lastCommandRevision=0;
uint8_t lastCommand=0;
cfRxResult_e lastCommandResult=CF_RX_STALE;
CubePilotFW_ReceiverAck pendingAck{};
cfRxResult_e userResult=CF_RX_STALE;
bool peerRequestPending=false,awaitingCommand=false;
uint32_t peerRequestTransaction=0,peerRequestLastSend=0;
uint8_t peerRequestSends=0;
cfRxCommand_e peerRequestCommand=CF_RX_BIND;
void acknowledge(uint32_t transaction,uint32_t revision,uint32_t persistedRevision,cfRxResult_e result,bool operationPending=false)
{
    pendingAck=CubePilotFW_ReceiverAck_init_zero;
    pendingAck.session=endpoint.session().session();pendingAck.transaction=transaction;
    pendingAck.revision=revision;pendingAck.persistedRevision=persistedRevision;pendingAck.result=result;
    pendingAck.operationPending=operationPending;ackPending=true;
}
void sendAck()
{
    if (!ackPending) return;
    CubePilotFW_ReceiverControlEnvelope envelope=CubePilotFW_ReceiverControlEnvelope_init_zero;
    envelope.which_payload=CubePilotFW_ReceiverControlEnvelope_ack_tag;envelope.payload.ack=pendingAck;
    if (!endpoint.sendControl(envelope)) ackPending=false;
}
uint32_t lastHello=0;
bool active=false;
}
void elrsCfInit(void)
{
    const uint32_t boot=record.magic==BOOT_MAGIC ? cfRxNextBoot(record.value,record.inverse) : 1;
    record.magic=0;record.value=boot;record.inverse=~boot;__DMB();record.magic=BOOT_MAGIC;
    HwSemaphore::init(); // Local MPU/IRQ only: shared HSEM clock is M7-owned.
    const uint32_t now=micros();
    active=endpoint.begin(false,boot,reinterpret_cast<uint8_t *>(ReceiverLayout::BASE),now);
    lastHello=now-ReceiverSession::HEARTBEAT_US;
    client.reset(0);
#ifdef Regulatory_Domain_EU_CE_2400
    client.compiledDomain(1);
#endif
    lastCapabilities=now-1000000U;
}
cfRxResult_e elrsCfRequestSettings(const cfRxSettings_t *settings)
{
    if (!settings) return CF_RX_INVALID;
    Guard guard;return userResult=client.request(*settings,false,micros());
}
cfRxResult_e elrsCfEditByte(size_t offset,uint8_t value)
{
    if (offset<12 || offset>=sizeof(cfRxSettings_t)) return CF_RX_INVALID;
    Guard guard;auto settings=client.settings();reinterpret_cast<uint8_t *>(&settings)[offset]=value;
    return userResult=client.request(settings,false,micros());
}
cfRxResult_e elrsCfBindUid(const uint8_t *uid)
{
    Guard guard;userResult=client.bindUid(uid,micros());
    if (userResult==CF_RX_OK) runtimeRefresh=true;
    return userResult;
}
cfRxResult_e elrsCfReturnLoan(void)
{
    Guard guard;return userResult=client.returnLoan(micros());
}
cfRxResult_e elrsCfResetSettings(void)
{
    Guard guard;cfRxSettings_t defaults{};
    defaults.modelId=255;defaults.antennaMode=2;defaults.teamraceChannel=10;defaults.domain=client.settings().domain;
    return userResult=client.request(defaults,false,micros());
}
void elrsCfHintInitialRate(uint8_t rate) { if (rate<10) { Guard guard;rateHint=rate; } }
bool elrsCfGetSettings(cfRxSettings_t *out)
{
    if (!out) return false;
    Guard guard;*out=client.settings();return client.revision()!=0;
}
cfRxResult_e elrsCfBeginBinding(bool administered) { Guard guard;return userResult=client.bind(administered,micros()); }
cfRxResult_e elrsCfCancelBinding(void) { Guard guard;return userResult=client.cancel(micros()); }
bool elrsCfOperationPending(void) { Guard guard;return client.pending() || client.binding(); }
cfRxResult_e elrsCfMutationAuthorization(void) { Guard guard;return client.canChange(micros()); }
cfRxResult_e elrsCfRequestCommand(cfRxCommand_e command)
{
    Guard guard;
    const auto auth=client.canChange(micros());
    if (auth!=CF_RX_OK) return userResult=auth;
    if (client.settings().bindStorage==3 && command!=CF_RX_CANCEL_BIND) return userResult=CF_RX_UNSUPPORTED;
    if (peerRequestPending || awaitingCommand) return userResult=CF_RX_BUSY;
    if (static_cast<unsigned>(command)>CF_RX_RESET_SETTINGS) return userResult=CF_RX_INVALID;
    if (peerRequestTransaction==UINT32_MAX) return userResult=CF_RX_STALE;
    peerRequestTransaction=(peerRequestTransaction+1U)|0x80000000U;
    peerRequestCommand=command;peerRequestPending=true;peerRequestSends=0;
    return userResult=CF_RX_PENDING;
}
bool elrsCfCanChangeSettings(void) { Guard guard;return client.canChange(micros())==CF_RX_OK; }
bool elrsCfConfigReady(void) { Guard guard;return client.ready(micros()) && !peerRequestPending && !awaitingCommand; }
cfRxResult_e elrsCfSettingsResult(void)
{
    Guard guard;
    if (peerRequestPending || awaitingCommand || client.pending()) return CF_RX_PENDING;
    if (client.lastResult()==CF_RX_SAVE_FAILED || client.lastResult()==CF_RX_TIMEOUT) return client.lastResult();
    if (client.binding()) return CF_RX_PENDING;
    return userResult==CF_RX_PENDING ? client.lastResult() : userResult;
}
void elrsCfPoll(uint32_t nowUs)
{
    if (!active) return;
    const uint32_t previous=endpoint.session().session();
    const bool wasLinked=endpoint.session().linked(nowUs);
    const bool received=endpoint.pollControl(nowUs);
    {
        Guard guard;
        if (previous!=endpoint.session().session()) {
            client.reset(endpoint.session().session());ackPending=false;rateHint=255;runtimeRefresh=false;
            lastCommandTransaction=0;lastCommandPending=false;
            peerRequestPending=awaitingCommand=false;peerRequestTransaction=0;
            lastCapabilities=nowUs-1000000U;
        }
        client.authorize(endpoint.session().receiverEnabled(),endpoint.session().peerArmed(),
            endpoint.session().lastControlUs(),endpoint.session().linked(nowUs));
    }
    sendAck();
    CubePilotFW_ReceiverControlEnvelope event;
    if (!ackPending && endpoint.takeControlEvent(&event,nowUs)) {
        if (event.which_payload==CubePilotFW_ReceiverControlEnvelope_ack_tag) {
            const auto &ack=event.payload.ack;
            Guard guard;
            if (peerRequestPending && ack.transaction==peerRequestTransaction) {
                peerRequestPending=false;awaitingCommand=ack.result==CF_RX_OK;
                peerRequestLastSend=nowUs;userResult=awaitingCommand ? CF_RX_PENDING : static_cast<cfRxResult_e>(ack.result);
            } else {
                client.result(ack.transaction,static_cast<cfRxResult_e>(ack.result),ack.revision,ack.persistedRevision);
            }
        } else if (event.which_payload==CubePilotFW_ReceiverControlEnvelope_command_tag) {
            const auto &command=event.payload.command;
            cfRxResult_e result;
            if (command.transaction==lastCommandTransaction) {
                result=command.command==lastCommand && command.revision==lastCommandRevision ? lastCommandResult : CF_RX_INVALID;
            } else if (!command.transaction || (lastCommandTransaction && uint32_t(command.transaction-lastCommandTransaction)>=0x80000000U) || command.revision!=client.revision()) {
                result=CF_RX_STALE;
            } else {
                {
                    Guard guard;
                    // The authoritative command can arrive before its request ACK
                    // if that ACK encountered an occupied mailbox.
                    if ((peerRequestPending || awaitingCommand) && command.command==peerRequestCommand)
                        peerRequestPending=awaitingCommand=false;
                }
                result=elrsCfRuntimeCommand(static_cast<cfRxCommand_e>(command.command));
                lastCommandTransaction=command.transaction;lastCommandRevision=command.revision;lastCommand=command.command;
                lastCommandResult=result==CF_RX_PENDING ? CF_RX_OK : result;
                lastCommandPending=elrsCfOperationPending();
                result=lastCommandResult;
            }
            acknowledge(command.transaction,client.revision(),client.persistedRevision(),result,elrsCfOperationPending());
        }
    }
    CubePilotFW_ReceiverConfigTransaction config;
    if (!ackPending && endpoint.pollConfig(&config,nowUs)) {
        cfRxSettings_t settings;
        cfRxResult_e result=CF_RX_INVALID;
        bool changed=false;
        if (config.result==CF_RX_OK && !config.volatileBinding && !cfRxSettingsFromWire(config.settings,&settings)) {
            Guard guard;
            const uint32_t previousRevision=client.revision();
            result=client.apply(config.transaction,config.revision,config.persistedRevision,settings,nowUs);
            changed=result==CF_RX_OK && client.revision()!=previousRevision;
            if (changed) { elrsCfApplyRuntime(&settings);userResult=CF_RX_OK; }
        }
        if (result==CF_RX_OK) endpoint.session().configured(endpoint.session().session(),config.revision);
        acknowledge(config.transaction,config.revision,config.persistedRevision,result);
    }
    ElrsCfProposal proposal;
    bool haveProposal;
    {
        Guard guard;
        if (rateHint!=255 && client.ready(nowUs) && client.canChange(nowUs)==CF_RX_OK) {
            auto settings=client.settings();settings.initialRate=rateHint;
            const auto result=client.request(settings,false,nowUs);
            if (result==CF_RX_OK || result==CF_RX_PENDING) { rateHint=255;userResult=result; }
        }
        haveProposal=client.proposal(nowUs,&proposal);
        if (runtimeRefresh) { const auto settings=client.settings();elrsCfApplyRuntime(&settings);runtimeRefresh=false; }
    }
    if (haveProposal) {
        CubePilotFW_ReceiverConfigTransaction wire=CubePilotFW_ReceiverConfigTransaction_init_zero;
        wire.session=endpoint.session().session();wire.transaction=proposal.transaction;wire.revision=proposal.revision;
        wire.result=CF_RX_PENDING;wire.has_settings=true;wire.volatileBinding=proposal.volatileBinding;
        cfRxSettingsToWire(proposal.settings,&wire.settings);endpoint.sendConfig(wire);
    }
    CubePilotFW_ReceiverControlEnvelope request=CubePilotFW_ReceiverControlEnvelope_init_zero;
    bool sendRequest=false;
    {
        Guard guard;
        if (peerRequestPending && (!peerRequestSends || uint32_t(nowUs-peerRequestLastSend)>=250000U)) {
            if (peerRequestSends==4) { peerRequestPending=false;userResult=CF_RX_TIMEOUT; }
            else if (!ackPending) {
                request.which_payload=CubePilotFW_ReceiverControlEnvelope_command_tag;
                request.payload.command.session=endpoint.session().session();request.payload.command.transaction=peerRequestTransaction;
                request.payload.command.revision=client.revision();request.payload.command.command=peerRequestCommand;
                peerRequestSends++;peerRequestLastSend=nowUs;sendRequest=true;
            }
        }
        if (awaitingCommand && uint32_t(nowUs-peerRequestLastSend)>=1500000U) { awaitingCommand=false;userResult=CF_RX_TIMEOUT; }
    }
    if (sendRequest) endpoint.sendControl(request);
    sendAck();
    if (!ackPending && lastCommandPending && !elrsCfOperationPending()) {
        lastCommandPending=false;lastCommandResult=client.lastResult();
        acknowledge(lastCommandTransaction,client.revision(),client.persistedRevision(),lastCommandResult);
        sendAck();
    }
    if ((received && !wasLinked) || uint32_t(nowUs-lastHello)>=ReceiverSession::HEARTBEAT_US) {
        if (!endpoint.sendHello(false,false)) lastHello=nowUs;
    }
    if (!ackPending && endpoint.session().linked(nowUs) && uint32_t(nowUs-lastCapabilities)>=1000000U) {
        CubePilotFW_ReceiverControlEnvelope envelope=CubePilotFW_ReceiverControlEnvelope_init_zero;
        envelope.which_payload=CubePilotFW_ReceiverControlEnvelope_capabilities_tag;
        auto &caps=envelope.payload.capabilities;
        caps.session=endpoint.session().session();caps.schemaVersion=1;caps.layoutVersion=1;
        caps.supportedRates=0x3ff;caps.supportedPowers=0x101;caps.antennaModes=7;
#ifdef Regulatory_Domain_EU_CE_2400
        caps.domain=1;
#endif
        strcpy(caps.firmware,"ExpressLRS CubeRacer M4");
        if (!endpoint.sendControl(envelope)) lastCapabilities=nowUs;
    }
}
#endif
