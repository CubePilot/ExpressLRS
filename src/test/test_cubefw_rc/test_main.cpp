#include <unity.h>
#include "receiver_rc.h"
static ElrsCfRcPublisher publisher;
static uint32_t channels[16];
void setUp() { publisher=ElrsCfRcPublisher();publisher.configure(7,2,true);for(auto &ch:channels) ch=992; }
void tearDown() {}
void test_only_eligible_frames_advance_sequence() {
    cfRxFrame_t frame{};
    TEST_ASSERT_FALSE(publisher.publish(false,true,false,channels,100));
    TEST_ASSERT_FALSE(publisher.snapshot(100,&frame));
    TEST_ASSERT_TRUE(publisher.publish(true,true,false,channels,100));
    TEST_ASSERT_TRUE(publisher.snapshot(100,&frame));TEST_ASSERT_EQUAL(1,frame.sequence);
    publisher.sent(frame.sequence);TEST_ASSERT_FALSE(publisher.snapshot(101,&frame));
    TEST_ASSERT_FALSE(publisher.publish(true,false,false,channels,102));
    TEST_ASSERT_FALSE(publisher.publish(true,true,true,channels,102));
    TEST_ASSERT_TRUE(publisher.publish(true,true,false,channels,103));
    TEST_ASSERT_TRUE(publisher.snapshot(103,&frame));TEST_ASSERT_EQUAL(2,frame.sequence);
    TEST_ASSERT_EQUAL(CF_RX_VALID|CF_RX_MODEL_MATCH,frame.flags);
}
void test_latest_snapshot_survives_completion_of_older_copy() {
    cfRxFrame_t old{},latest{};publisher.publish(true,true,false,channels,100);
    TEST_ASSERT_TRUE(publisher.snapshot(100,&old));channels[0]=1000;
    publisher.publish(true,true,false,channels,101);publisher.sent(old.sequence);
    TEST_ASSERT_TRUE(publisher.snapshot(101,&latest));TEST_ASSERT_EQUAL(2,latest.sequence);
    TEST_ASSERT_EQUAL(992,old.channels[0]);TEST_ASSERT_EQUAL(1000,latest.channels[0]);
    TEST_ASSERT_EQUAL(1,publisher.drops());
    publisher.publish(false,true,false,channels,102); // Polling cannot erase an unsent valid frame.
    TEST_ASSERT_TRUE(publisher.snapshot(102,&latest));
}
void test_inhibition_and_session_change_discard_old_frames() {
    cfRxFrame_t frame{};publisher.publish(true,true,false,channels,100);
    publisher.publish(false,true,true,channels,101);TEST_ASSERT_FALSE(publisher.snapshot(101,&frame));
    publisher.publish(true,true,false,channels,102);publisher.configure(7,3,true);
    TEST_ASSERT_FALSE(publisher.snapshot(102,&frame));publisher.publish(true,true,false,channels,103);
    publisher.configure(8,1,false);TEST_ASSERT_FALSE(publisher.snapshot(103,&frame));
    TEST_ASSERT_FALSE(publisher.publish(true,true,false,channels,104));publisher.configure(8,1,true);
    publisher.publish(true,true,false,channels,105);TEST_ASSERT_TRUE(publisher.snapshot(105,&frame));
    TEST_ASSERT_EQUAL(8,frame.session);TEST_ASSERT_EQUAL(1,frame.revision);TEST_ASSERT_EQUAL(1,frame.sequence);
}
void test_stale_snapshot_and_time_wrap() {
    cfRxFrame_t frame{};publisher.publish(true,true,false,channels,0xfffffff0U);
    TEST_ASSERT_TRUE(publisher.snapshot(10,&frame));
    TEST_ASSERT_FALSE(publisher.snapshot(0xfffffff0U+ElrsCfRcPublisher::MAX_AGE_US,&frame));
}
void test_original_units_and_invalid_channel_bounds() {
    cfRxFrame_t frame{};channels[0]=0;channels[15]=2047;
    publisher.publish(true,true,false,channels,100);TEST_ASSERT_TRUE(publisher.snapshot(100,&frame));
    TEST_ASSERT_EQUAL(0,frame.channels[0]);TEST_ASSERT_EQUAL(992,frame.channels[8]);TEST_ASSERT_EQUAL(2047,frame.channels[15]);
    channels[0]=0xffff;TEST_ASSERT_TRUE(publisher.publish(true,true,false,channels,101));
    TEST_ASSERT_TRUE(publisher.snapshot(101,&frame));TEST_ASSERT_EQUAL(0,frame.channels[0]);
    channels[0]=2048;TEST_ASSERT_FALSE(publisher.publish(true,true,false,channels,101));
    TEST_ASSERT_FALSE(publisher.snapshot(101,&frame));
}
void test_interrupt_after_poll_timestamp_does_not_expire_new_frame() {
    cfRxFrame_t frame{};publisher.publish(true,true,false,channels,101);
    TEST_ASSERT_FALSE(publisher.snapshot(100,&frame));
    TEST_ASSERT_TRUE(publisher.snapshot(102,&frame));TEST_ASSERT_EQUAL(1,frame.sequence);
}
int main() { UNITY_BEGIN();RUN_TEST(test_only_eligible_frames_advance_sequence);
    RUN_TEST(test_latest_snapshot_survives_completion_of_older_copy);RUN_TEST(test_inhibition_and_session_change_discard_old_frames);
    RUN_TEST(test_interrupt_after_poll_timestamp_does_not_expire_new_frame);RUN_TEST(test_stale_snapshot_and_time_wrap);RUN_TEST(test_original_units_and_invalid_channel_bounds);return UNITY_END(); }
