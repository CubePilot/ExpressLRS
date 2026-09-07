#pragma once
#include <CubePilotFW/ReceiverTypes.h>

namespace CubeRacer
{
// Shared by the embedded hooks and native failure fixtures. Lifecycle owns the
// shutdown obligation on any nonzero result; no RF-ready escapes a partial setup.
template <class Hooks>
class RadioRuntime
{
public:
    explicit RadioRuntime(Hooks &hooks) : hooks_(hooks) {}

    uint8_t start()
    {
        if (!prepared_)
        {
            hooks_.prepare();
            prepared_ = true;
        }
        if (!hooks_.begin())
            return error();
        return reconfigure();
    }

    uint8_t reconfigure()
    {
        if (!hooks_.configure() || !hooks_.timer() || !hooks_.receive())
            return error();
        return 0;
    }

    bool stop() { return hooks_.stop(); }

private:
    Hooks &hooks_;
    bool prepared_ = false;

    uint8_t error() const
    {
        const uint8_t fault = hooks_.fault();
        return fault ? fault : static_cast<uint8_t>(CF_RX_RADIO_INIT_FAILED);
    }
};
}
