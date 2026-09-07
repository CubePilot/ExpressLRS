#pragma once
#include <CubePilotFW/ReceiverTypes.h>

struct ElrsCfRadioContext
{
    uint32_t session, revision;
    bool linked, configured, armed, available;
};

// M4 owns the hardware until reset. Link/configuration freshness gates READY,
// while an already running receiver keeps its last configuration during gaps.
template <class Runtime>
class ElrsCfRadio
{
public:
    explicit ElrsCfRadio(Runtime &runtime) : runtime_(runtime) {}

    void update(const ElrsCfRadioContext &context) { context_ = context; }

    void step()
    {
        if (!fault_ && started_)
            fault_ = runtime_.fault();
        if (!fault_ && configured() && !context_.armed &&
            (!started_ || session_ != context_.session || revision_ != context_.revision))
        {
            fault_ = started_ ? runtime_.reconfigure() : runtime_.start();
            if (!fault_)
            {
                started_ = true;
                session_ = context_.session;
                revision_ = context_.revision;
            }
        }
        if (fault_ && !stopped_)
            stopped_ = runtime_.stop();
    }

    bool ready() const { return running() && configured() && session_ == context_.session && revision_ == context_.revision; }

    bool running() const { return started_ && !fault_ && !runtime_.fault(); }

    uint8_t fault() const { return fault_ ? fault_ : runtime_.fault(); }

private:
    bool configured() const { return context_.available && context_.linked && context_.configured && context_.session && context_.revision; }

    Runtime &runtime_;
    ElrsCfRadioContext context_{};
    uint32_t session_ = 0, revision_ = 0;
    uint8_t fault_ = 0;
    bool started_ = false, stopped_ = false;
};
