#include "radio_runtime.h"
#include <cassert>
#include <cstdio>
#include <vector>
struct Hooks {
    unsigned failure=0,prepared=0;
    std::vector<unsigned> calls;
    bool call(unsigned n) { calls.push_back(n);return failure!=n; }
    bool grant(uint32_t session) { assert(session==7);return call(1); }
    void prepare() { ++prepared;calls.push_back(2); }
    bool begin() { return call(3); }
    bool configure() { return call(4); }
    bool timer() { return call(5); }
    bool receive() { return call(6); }
    uint8_t fault() const { return failure ? CF_RX_RADIO_BUSY_TIMEOUT : 0; }
    bool stop() { return call(7); }
};
int main() {
    for(unsigned failure: {1U,3U,4U,5U,6U}) {
        Hooks h;h.failure=failure;CubeRacer::RadioRuntime<Hooks> r(h);
        assert(r.start(7)!=0);assert(h.calls.back()==failure);
    }
    Hooks h;CubeRacer::RadioRuntime<Hooks> r(h);
    assert(!r.start(7));assert((h.calls==std::vector<unsigned>{1,2,3,4,5,6}));
    h.calls.clear();assert(!r.reconfigure());assert((h.calls==std::vector<unsigned>{4,5,6}));
    assert(h.prepared==1);assert(r.stop());h.calls.clear();assert(!r.start(7));
    assert((h.calls==std::vector<unsigned>{1,3,4,5,6}) && h.prepared==1);
    puts("Radio runtime initialization ordering, failure and single registration tests passed");
}
