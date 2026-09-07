#include "receiver_radio.h"
#include <cassert>
#include <cstdio>
struct Runtime {
    unsigned starts=0,changes=0,stops=0;
    uint8_t error=0;
    bool quiet=true;
    uint8_t start(uint32_t session) { assert(session==7 || session==8);++starts;return error; }
    uint8_t reconfigure() { ++changes;return error; }
    uint8_t fault() const { return error; }
    bool stop() { ++stops;return quiet; }
};
using Context=ElrsCfRadioContext;
static Context context{7,3,true,true,true,false,true};
static cfRxRadioControl_t grant{7,3,12,CF_RX_RADIO_GRANT,0};
int main() {
    Runtime hw;ElrsCfRadio<Runtime> radio(hw);cfRxRadioControl_t message{};
    radio.update(context);assert(radio.message(&message) && message.phase==CF_RX_RADIO_REQUEST);
    auto bad=grant;bad.session=6;assert(!radio.receive(bad));
    bad=grant;bad.revision=2;assert(!radio.receive(bad));
    auto c=context;c.armed=true;radio.update(c);assert(!radio.receive(grant));
    c=context;c.available=false;radio.update(c);assert(!radio.receive(grant));
    radio.update(context);assert(radio.receive(grant));
    assert(!radio.ready() && hw.starts==0);radio.step();
    assert(radio.ready() && hw.starts==1);
    assert(radio.message(&message) && message.phase==CF_RX_RADIO_READY);
    assert(radio.receive(grant));radio.step();assert(hw.starts==1 && hw.changes==0);
    c=context;c.armed=true;radio.update(c);assert(radio.ready());
    c=context;c.configured=false;radio.update(c);
    assert(!radio.ready() && radio.running());radio.step();assert(hw.stops==0);
    c.configured=true;c.revision=4;radio.update(c);
    assert(!radio.ready() && !radio.receive(grant));
    grant.revision=4;assert(radio.receive(grant));assert(!radio.ready());radio.step();
    assert(radio.ready() && hw.changes==1 && hw.starts==1 && hw.stops==0);
    // Same-revision binding completion also needs a fresh grant/confirmation.
    c.configured=false;radio.update(c);c.configured=true;radio.update(c);
    assert(!radio.ready());assert(radio.receive(grant));radio.step();assert(hw.changes==2);
    // A local bus fault removes readiness before any quiescence acknowledgement.
    hw.error=CF_RX_RADIO_BUSY_TIMEOUT;radio.update(c);assert(!radio.ready());
    hw.quiet=false;radio.step();assert(!radio.running());
    auto revoke=grant;revoke.phase=CF_RX_RADIO_REVOKE;assert(radio.receive(revoke));
    radio.step();assert(!radio.message(&message));
    hw.quiet=true;radio.step();assert(radio.message(&message) && message.phase==CF_RX_RADIO_QUIESCENT);
    radio.sent(message);assert(!radio.receive(grant));assert(!radio.message(&message));
    // Delayed revoke retransmissions must still receive a quiescent reply.
    assert(radio.receive(revoke));radio.step();assert(radio.message(&message));radio.sent(message);
    // New-session revocation of an old lease is safe even after local shutdown.
    c.session=8;c.revision=1;radio.update(c);revoke.session=8;revoke.revision=1;
    assert(radio.receive(revoke));radio.step();assert(radio.message(&message));
    assert(message.session==8 && message.revision==1 && message.grant==12);radio.sent(message);
    hw.error=0;assert(radio.message(&message) && message.phase==CF_RX_RADIO_REQUEST);
    grant={8,1,13,CF_RX_RADIO_GRANT,0};assert(radio.receive(grant));radio.step();assert(radio.ready());
    c.linked=false;radio.update(c);assert(!radio.ready() && !radio.running());radio.step();
    assert(!radio.message(&message));c.linked=true;radio.update(c);assert(!radio.receive(grant));
    // Init failure must never produce READY, including repeated grants.
    Runtime broken;broken.error=CF_RX_RADIO_INIT_FAILED;ElrsCfRadio<Runtime> failed(broken);
    failed.update(context);grant={7,3,1,CF_RX_RADIO_GRANT,0};assert(failed.receive(grant));failed.step();
    assert(!failed.ready());failed.step();assert(failed.message(&message) && message.phase==CF_RX_RADIO_FAULT);
    assert(!failed.receive(grant) && broken.starts==1);
    puts("M4 radio lifecycle authorization, reconfiguration, fault and quiescence tests passed");
}
