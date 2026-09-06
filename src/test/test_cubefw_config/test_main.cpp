#include <unity.h>
#include <cstring>
#include "receiver_config.h"
static ElrsCfConfigClient client;
static cfRxSettings_t settings;
void setUp() {
    client=ElrsCfConfigClient();client.reset(7);client.authorize(true,false,1000,true);
    settings={};settings.modelId=255;settings.antennaMode=2;settings.teamraceChannel=10;
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(1,1,1,settings,1000));
}
void tearDown() {}
static void bindMode(unsigned mode) {
    client.reset(7);client.authorize(true,false,1000,true);settings.bindStorage=mode;
    settings.homeUid[0]=10;settings.boundUid[0]=10;
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(1,1,1,settings,1000));
}
void test_binding_modes_and_home_loan() {
    for(unsigned mode=0;mode<4;mode++) {
        bindMode(mode);
        TEST_ASSERT_EQUAL(mode==3?CF_RX_UNSUPPORTED:CF_RX_OK,client.bind(false,1000));
        if(mode==3) TEST_ASSERT_EQUAL(CF_RX_OK,client.bind(true,1000));
        uint8_t uid[6]={20};TEST_ASSERT_EQUAL(CF_RX_PENDING,client.bindUid(uid,1000));
        ElrsCfProposal proposal{};TEST_ASSERT_TRUE(client.proposal(1000,&proposal));
        TEST_ASSERT_EQUAL(mode==1,proposal.volatileBinding);
        TEST_ASSERT_EQUAL(mode==0?20:10,proposal.settings.homeUid[0]);
        TEST_ASSERT_EQUAL(20,proposal.settings.boundUid[0]);
        TEST_ASSERT_FALSE(client.ready(1000));
    }
}
void test_persistence_ack_must_precede_binding_completion() {
    bindMode(2);TEST_ASSERT_EQUAL(CF_RX_OK,client.bind(false,1000));
    uint8_t uid[6]={20};TEST_ASSERT_EQUAL(CF_RX_PENDING,client.bindUid(uid,1000));
    ElrsCfProposal proposal{};TEST_ASSERT_TRUE(client.proposal(1000,&proposal));
    TEST_ASSERT_EQUAL(CF_RX_PENDING,client.apply(2,2,2,proposal.settings,1000));
    TEST_ASSERT_EQUAL(10,client.settings().boundUid[0]);
    client.result(proposal.transaction,CF_RX_OK,2,2);
    TEST_ASSERT_FALSE(client.ready(1000));
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(2,2,2,proposal.settings,1000));
    TEST_ASSERT_EQUAL(20,client.settings().boundUid[0]);TEST_ASSERT_TRUE(client.ready(1000));
    TEST_ASSERT_EQUAL(CF_RX_PENDING,client.returnLoan(1000));
    TEST_ASSERT_TRUE(client.proposal(1000,&proposal));TEST_ASSERT_EQUAL(10,proposal.settings.boundUid[0]);
}
void test_fresh_disarmed_authorization_and_runtime_readiness() {
    client.authorize(true,true,1000,true);TEST_ASSERT_TRUE(client.ready(1000));
    TEST_ASSERT_EQUAL(CF_RX_ARMED,client.bind(false,1000));
    TEST_ASSERT_EQUAL(CF_RX_ARMED,client.request(settings,false,1000));
    client.authorize(true,false,1000,true);TEST_ASSERT_EQUAL(CF_RX_STALE,client.bind(false,251000));
    client.authorize(false,false,1000,true);TEST_ASSERT_FALSE(client.ready(1000));
    TEST_ASSERT_EQUAL(CF_RX_UNSUPPORTED,client.bind(false,1000));
}
void test_retry_failure_and_restart_do_not_assume_saved() {
    settings.modelId=1;TEST_ASSERT_EQUAL(CF_RX_PENDING,client.request(settings,false,1000));
    ElrsCfProposal proposal{};
    for(unsigned i=0;i<4;i++) { client.authorize(true,false,1000+i*250000,true);TEST_ASSERT_TRUE(client.proposal(1000+i*250000,&proposal)); }
    TEST_ASSERT_FALSE(client.proposal(1001000,&proposal));TEST_ASSERT_EQUAL(CF_RX_TIMEOUT,client.lastResult());
    client.result(proposal.transaction,CF_RX_OK,2,2);TEST_ASSERT_FALSE(client.ready(1001000));
    client.reset(8);client.authorize(true,false,1001000,true);
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(1,1,1,settings,1001000));TEST_ASSERT_TRUE(client.ready(1001000));
}
void test_rejects_changed_duplicate_and_stale_apply() {
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(1,1,1,settings,1000));
    settings.modelId=1;TEST_ASSERT_EQUAL(CF_RX_INVALID,client.apply(1,1,1,settings,1000));
    TEST_ASSERT_EQUAL(CF_RX_STALE,client.apply(2,0,1,settings,1000));
    settings.telemetryPower=1;TEST_ASSERT_EQUAL(CF_RX_UNSUPPORTED,client.request(settings,false,1000));
}
void test_save_failure_and_cancel_pending() {
    TEST_ASSERT_EQUAL(CF_RX_OK,client.bind(false,1000));uint8_t uid[6]={20};
    TEST_ASSERT_EQUAL(CF_RX_PENDING,client.bindUid(uid,1000));TEST_ASSERT_EQUAL(CF_RX_BUSY,client.cancel(1000));
    ElrsCfProposal proposal{};TEST_ASSERT_TRUE(client.proposal(1000,&proposal));
    client.result(proposal.transaction,CF_RX_SAVE_FAILED,1,1);
    TEST_ASSERT_FALSE(client.ready(1000));TEST_ASSERT_EQUAL(0,client.settings().boundUid[0]);
}
void test_same_uid_finishes_without_another_save() {
    settings.homeUid[0]=settings.boundUid[0]=20;
    client.reset(7);client.authorize(true,false,1000,true);
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(1,1,1,settings,1000));
    TEST_ASSERT_EQUAL(CF_RX_OK,client.bind(false,1000));
    TEST_ASSERT_EQUAL(CF_RX_OK,client.bindUid(settings.boundUid,1000));TEST_ASSERT_TRUE(client.ready(1000));
}
void test_confirmed_save_without_apply_times_out() {
    settings.modelId=1;TEST_ASSERT_EQUAL(CF_RX_PENDING,client.request(settings,false,1000));
    ElrsCfProposal proposal{};TEST_ASSERT_TRUE(client.proposal(1000,&proposal));
    client.result(proposal.transaction,CF_RX_OK,2,2);
    TEST_ASSERT_FALSE(client.proposal(1001000,&proposal));TEST_ASSERT_EQUAL(CF_RX_TIMEOUT,client.lastResult());
}
void test_volatile_uid_is_removed_from_other_persistent_proposals() {
    bindMode(1);settings=client.settings();settings.modelId=1;
    TEST_ASSERT_EQUAL(CF_RX_PENDING,client.request(settings,false,1000));
    ElrsCfProposal proposal{};TEST_ASSERT_TRUE(client.proposal(1000,&proposal));
    TEST_ASSERT_EQUAL(0,proposal.settings.boundUid[0]);TEST_ASSERT_FALSE(proposal.volatileBinding);
}
void test_restart_before_ack_requires_authoritative_settings() {
    settings.modelId=1;TEST_ASSERT_EQUAL(CF_RX_PENDING,client.request(settings,false,1000));
    ElrsCfProposal proposal{};TEST_ASSERT_TRUE(client.proposal(1000,&proposal));
    client.reset(8);client.authorize(true,false,1000,true);
    client.result(proposal.transaction,CF_RX_OK,2,2);
    TEST_ASSERT_FALSE(client.ready(1000));
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(1,1,1,settings,1000));
    TEST_ASSERT_TRUE(client.ready(1000));TEST_ASSERT_EQUAL(1,client.settings().modelId);
}
void test_volatile_binding_then_durable_edit_keeps_session_uid() {
    bindMode(1);TEST_ASSERT_EQUAL(CF_RX_OK,client.bind(false,1000));
    uint8_t uid[6]={20};TEST_ASSERT_EQUAL(CF_RX_PENDING,client.bindUid(uid,1000));
    ElrsCfProposal proposal{};TEST_ASSERT_TRUE(client.proposal(1000,&proposal));
    client.result(proposal.transaction,CF_RX_OK,2,1);
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(2,2,1,proposal.settings,1000));
    settings=client.settings();settings.modelId=2;
    TEST_ASSERT_EQUAL(CF_RX_PENDING,client.request(settings,false,1000));
    TEST_ASSERT_TRUE(client.proposal(1000,&proposal));
    TEST_ASSERT_EQUAL(0,proposal.settings.boundUid[0]);
    client.result(proposal.transaction,CF_RX_OK,3,3);
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(3,3,3,settings,1000));
    TEST_ASSERT_EQUAL(20,client.settings().boundUid[0]);TEST_ASSERT_TRUE(client.ready(1000));
}
void test_arming_between_save_confirmation_and_apply_defers_adoption() {
    settings.modelId=17;TEST_ASSERT_EQUAL(CF_RX_PENDING,client.request(settings,false,1000));
    ElrsCfProposal proposal{};TEST_ASSERT_TRUE(client.proposal(1000,&proposal));
    client.result(proposal.transaction,CF_RX_OK,2,2);
    client.authorize(true,true,2000,true);
    TEST_ASSERT_EQUAL(CF_RX_ARMED,client.apply(2,2,2,settings,2000));
    TEST_ASSERT_EQUAL(255,client.settings().modelId);TEST_ASSERT_FALSE(client.ready(2000));
    client.authorize(true,false,3000,true);
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(2,2,2,settings,3000));
    TEST_ASSERT_EQUAL(17,client.settings().modelId);TEST_ASSERT_TRUE(client.ready(3000));
}
void test_lost_save_ack_retries_identical_proposal_without_adopting_early_apply() {
    settings.modelId=17;TEST_ASSERT_EQUAL(CF_RX_PENDING,client.request(settings,false,1000));
    ElrsCfProposal first{},retry{};TEST_ASSERT_TRUE(client.proposal(1000,&first));
    TEST_ASSERT_EQUAL(CF_RX_PENDING,client.apply(2,2,2,settings,2000));
    TEST_ASSERT_EQUAL(255,client.settings().modelId);TEST_ASSERT_FALSE(client.ready(2000));
    client.authorize(true,false,251000,true);TEST_ASSERT_TRUE(client.proposal(251000,&retry));
    TEST_ASSERT_EQUAL(first.transaction,retry.transaction);TEST_ASSERT_EQUAL(first.revision,retry.revision);
    TEST_ASSERT_EQUAL_MEMORY(&first.settings,&retry.settings,sizeof first.settings);
    client.result(retry.transaction,CF_RX_OK,2,2);
    TEST_ASSERT_EQUAL(CF_RX_OK,client.apply(2,2,2,settings,251000));
    TEST_ASSERT_TRUE(client.ready(251000));
}
int main() {
    UNITY_BEGIN();RUN_TEST(test_binding_modes_and_home_loan);RUN_TEST(test_persistence_ack_must_precede_binding_completion);
    RUN_TEST(test_fresh_disarmed_authorization_and_runtime_readiness);RUN_TEST(test_retry_failure_and_restart_do_not_assume_saved);
    RUN_TEST(test_rejects_changed_duplicate_and_stale_apply);RUN_TEST(test_save_failure_and_cancel_pending);
    RUN_TEST(test_same_uid_finishes_without_another_save);RUN_TEST(test_confirmed_save_without_apply_times_out);
    RUN_TEST(test_volatile_uid_is_removed_from_other_persistent_proposals);
    RUN_TEST(test_restart_before_ack_requires_authoritative_settings);
    RUN_TEST(test_volatile_binding_then_durable_edit_keeps_session_uid);
    RUN_TEST(test_arming_between_save_confirmation_and_apply_defers_adoption);
    RUN_TEST(test_lost_save_ack_retries_identical_proposal_without_adopting_early_apply);
    return UNITY_END();
}
