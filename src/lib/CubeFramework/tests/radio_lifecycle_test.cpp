#include "receiver_radio.h"
#include <cassert>
#include <cstdio>

struct Runtime
{
    unsigned starts = 0, changes = 0, stops = 0;
    uint8_t error = 0;

    uint8_t start()
    {
        ++starts;
        return error;
    }

    uint8_t reconfigure()
    {
        ++changes;
        return error;
    }

    uint8_t fault() const { return error; }

    bool stop()
    {
        ++stops;
        return true;
    }
};

int main()
{
    Runtime hw;
    ElrsCfRadio<Runtime> radio(hw);
    ElrsCfRadioContext c{7, 3, true, true, false, true};
    c.configured = false;
    radio.update(c);
    radio.step();
    assert(hw.starts == 0);
    c.configured = true;
    radio.update(c);
    radio.step();
    assert(hw.starts == 1 && radio.ready());
    radio.step();
    assert(hw.starts == 1 && hw.changes == 0);
    c.linked = false;
    radio.update(c);
    radio.step();
    assert(radio.running() && !radio.ready() && hw.stops == 0);
    c.linked = true;
    c.configured = false;
    radio.update(c);
    radio.step();
    assert(radio.running() && !radio.ready());
    c.configured = true;
    c.revision = 4;
    c.armed = true;
    radio.update(c);
    radio.step();
    assert(!radio.ready() && hw.changes == 0);
    c.armed = false;
    radio.update(c);
    radio.step();
    assert(radio.ready() && hw.changes == 1 && hw.starts == 1);
    c.session = 8;
    c.revision = 1;
    radio.update(c);
    radio.step();
    assert(radio.ready() && hw.changes == 2 && hw.starts == 1);
    hw.error = CF_RX_RADIO_SPI_TIMEOUT;
    radio.update(c);
    radio.step();
    assert(!radio.running() && !radio.ready());
    assert(radio.fault() == CF_RX_RADIO_SPI_TIMEOUT && hw.stops == 1);
    radio.step();
    assert(hw.starts == 1 && hw.stops == 1);
    Runtime broken;
    broken.error = CF_RX_RADIO_INIT_FAILED;
    ElrsCfRadio<Runtime> failed(broken);
    failed.update(c);
    failed.step();
    failed.step();
    assert(broken.starts == 1 && broken.stops == 1 && !failed.ready());
    Runtime disabled;
    ElrsCfRadio<Runtime> off(disabled);
    c.available = false;
    off.update(c);
    off.step();
    assert(!disabled.starts);
    puts("M4 static radio lifecycle startup/configuration/freshness/fault tests passed");
}
