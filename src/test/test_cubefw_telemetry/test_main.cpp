#include <unity.h>
#include <cstring>
#include "crsf_connector.h"
#include "CRSFRouter.h"
#include "RXOTAConnector.h"
#include <CubePilotFW/ReceiverCrsf.h>
CRSFRouter crsfRouter;
static bool allowed=true;static unsigned sentCount;static cfRxCrsfFrame_t sent{};
bool elrsCfForwardToFcAllowed() { return allowed; }
bool elrsCfConfigReady() { return allowed; }
bool elrsCfSendCrsf(const uint8_t *p,uint8_t n) { ++sentCount;sent.length=n;memcpy(sent.bytes,p,n);return true; }
void elrsCfTelemetrySensor(uint8_t) {}
class RadioSink : public RXOTAConnector {
public: cfRxCrsfFrame_t received{};unsigned count=0;
 void forwardMessage(const crsf_header_t *p) override { RXOTAConnector::forwardMessage(p);++count;received.length=p->frame_size+2;memcpy(received.bytes,p,received.length); }
};
class ReceiverSink : public CRSFEndpoint {
public: unsigned writes=0; ReceiverSink():CRSFEndpoint(CRSF_ADDRESS_CRSF_RECEIVER) {}
 void handleMessage(const crsf_header_t *p) override { if(p->type==CRSF_FRAMETYPE_PARAMETER_WRITE) ++writes; }
};
static CubeFrameworkCrsfConnector connector;static RadioSink radio;static ReceiverSink receiver;
void setUp() { crsfRouter=CRSFRouter();crsfRouter.addConnector(&connector);crsfRouter.addConnector(&radio);crsfRouter.addEndpoint(&receiver);allowed=true;sentCount=radio.count=receiver.writes=0;radio.Reset(); }
void tearDown() {}
static void finish(uint8_t *p,unsigned n) { p[n-1]=crsfRouter.crsf_crc.calc(p+2,n-3); }
void test_flight_frames_reach_radio_unchanged() {
 uint8_t battery[]={0xc8,10,8,0,0,0,0,0,0,0,67,0};uint8_t attitude[]={0xc8,8,0x1e,0,0,0,0,0,0,0};
 uint8_t gps[]={0xc8,17,2,0,0,0,0,0,0,0,0,0,0,0,3,0xe8,0,0};uint8_t mode[]={0xc8,8,0x21,'A','C','R','O','*',0,0};
 for(auto p:{battery,attitude,gps,mode}) { unsigned n=p[1]+2;finish(p,n);TEST_ASSERT_TRUE(connector.receive(p,n));TEST_ASSERT_EQUAL(n,radio.received.length);TEST_ASSERT_EQUAL_MEMORY(p,radio.received.bytes,n);uint8_t actual[64],size=0;TEST_ASSERT_TRUE(radio.GetNextPayload(&size,actual));TEST_ASSERT_EQUAL(n,size);TEST_ASSERT_EQUAL_MEMORY(p,actual,n); }
 TEST_ASSERT_EQUAL(4,radio.count);TEST_ASSERT_EQUAL(0,sentCount);
}
void test_addresses_and_no_echo() {
 uint8_t fc[]={0xc8,7,0x7a,0xc8,0xea,0x30,0,1,0};finish(fc,sizeof fc);
 crsfRouter.processMessage(&radio,(crsf_header_t*)fc);TEST_ASSERT_EQUAL(1,sentCount);TEST_ASSERT_EQUAL_MEMORY(fc,sent.bytes,sizeof fc);
 uint8_t parameter[]={0xc8,6,0x2d,0xec,0xea,1,2,0};finish(parameter,sizeof parameter);
 crsfRouter.processMessage(&radio,(crsf_header_t*)parameter);TEST_ASSERT_EQUAL(1,receiver.writes);TEST_ASSERT_EQUAL(1,sentCount);
 TEST_ASSERT_TRUE(connector.receive(parameter,sizeof parameter));TEST_ASSERT_EQUAL(2,receiver.writes);TEST_ASSERT_EQUAL(0,radio.count);
}
void test_malformed_frames_and_inhibition() {
 uint8_t f[]={0xc8,4,0x28,0xec,0xc8,0};finish(f,sizeof f);
 TEST_ASSERT_FALSE(connector.receive(nullptr,6));TEST_ASSERT_FALSE(connector.receive(f,5));f[5]^=1;TEST_ASSERT_FALSE(connector.receive(f,6));
 finish(f,sizeof f);allowed=false;TEST_ASSERT_FALSE(connector.receive(f,6));connector.forwardMessage((crsf_header_t*)f);TEST_ASSERT_EQUAL(0,sentCount);
}
void test_reset_discards_radio_queue() {
 uint8_t f[]={0xc8,4,0x28,0xec,0xc8,0};finish(f,sizeof f);
 radio.forwardMessage((crsf_header_t*)f);radio.Reset();
 uint8_t bytes[64],size=0;TEST_ASSERT_FALSE(radio.GetNextPayload(&size,bytes));
}
int main() { UNITY_BEGIN();RUN_TEST(test_flight_frames_reach_radio_unchanged);RUN_TEST(test_addresses_and_no_echo);RUN_TEST(test_malformed_frames_and_inhibition);RUN_TEST(test_reset_discards_radio_queue);return UNITY_END(); }
