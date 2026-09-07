#pragma once
#include <stdint.h>
void elrsCfInit(void);
void elrsCfPoll(uint32_t nowUs);

#include <CubePilotFW/ReceiverTypes.h>
#include <stddef.h>
cfRxResult_e elrsCfRequestSettings(const cfRxSettings_t *settings);
cfRxResult_e elrsCfEditByte(size_t offset, uint8_t value);
cfRxResult_e elrsCfBindUid(const uint8_t *uid);
cfRxResult_e elrsCfResetSettings(void);
cfRxResult_e elrsCfReturnLoan(void);
void elrsCfHintInitialRate(uint8_t rate);
bool elrsCfGetSettings(cfRxSettings_t *out);
bool elrsCfCanChangeSettings(void);
bool elrsCfConfigReady(void);
cfRxResult_e elrsCfSettingsResult(void);
// Called in the main loop only, after M7's configuration is accepted.
void elrsCfApplyRuntime(const cfRxSettings_t *settings);

cfRxResult_e elrsCfBeginBinding(bool administered);
cfRxResult_e elrsCfCancelBinding(void);
bool elrsCfOperationPending(void);
cfRxResult_e elrsCfRuntimeCommand(cfRxCommand_e command);

cfRxResult_e elrsCfRequestCommand(cfRxCommand_e command);
cfRxResult_e elrsCfMutationAuthorization(void);

void elrsCfPublishChannels(bool available,bool modelMatch,bool inhibited,const uint32_t *channels);
void elrsCfPublishStats(const cfRxStatus_t *status);

// Framed management/telemetry calls are main-loop only.
bool elrsCfSendCrsf(const uint8_t *frame,uint8_t length);
void elrsCfStartCrsfRouter();

void elrsCfResetCrsfRouter();

// Radio lifecycle hooks run in the main loop outside transport critical sections.
uint8_t elrsCfRadioStart(uint32_t session);
uint8_t elrsCfRadioReconfigure();
uint8_t elrsCfRadioFault();
bool elrsCfRadioStop();
bool elrsCfRadioReady();
bool elrsCfRadioRunning();
