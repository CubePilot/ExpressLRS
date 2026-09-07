"""Exercise the real periodic stats path while connected and disconnected."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class StatsPublishTest(unittest.TestCase):
    def test_connected_stats_reach_m7(self):
        source = (Path(__file__).parents[3] / 'src/rx_main.cpp').read_text()
        start = source.index('static void checkSendLinkStatsToFc(')
        function = source[start:source.index('\n}', start) + 2]
        with tempfile.TemporaryDirectory() as tmp:
            cpp = Path(tmp) / 'test.cpp'
            cpp.write_text('''
#include <cstdint>
#include <cassert>
#include <cstring>
#define CUBERACER_M4
#define SEND_LINK_STATS_TO_FC_INTERVAL 100
#define CRSF_FRAME_NOT_COUNTED_BYTES 2
#define CRSF_FRAME_SIZE(n) ((n)+2)
struct cfRxStatus_t {
    unsigned packetRateHz=0;
    int rssiDbm=0, snr=0, linkQuality=0, antenna=0, power=0;
} published;
struct crsfLinkStatistics_t {
    int active_antenna=1, uplink_RSSI_1=65, uplink_RSSI_2=72;
    int uplink_SNR=7, uplink_Link_quality=98;
} linkStats;
struct AirRate { unsigned interval=3000; } rate;
auto ExpressLRS_currAirRate_Modparams=&rate;
namespace POWERMGNT { int currPower() { return 0; } }
enum { disconnected, connected };
int connectionState=connected;
bool connectionHasModelMatch=true, teamraceHasModelMatch=true;
unsigned SendLinkStatstoFCintervalLastSent=0, SendLinkStatstoFCForcedSends=0;
unsigned publications=0;
void getRFlinkInfo() {}
void elrsCfPublishStats(const cfRxStatus_t *value) { published=*value;publications++; }
struct crsf_header_t {};
struct Router {
    void makeLinkStatisticsPacket(uint8_t *) {}
    void deliverMessage(void *,crsf_header_t *) {}
} crsfRouter;
int otaConnector;
''' + function + '''
int main() {
    checkSendLinkStatsToFc(101);
    assert(publications==1 && published.packetRateHz==333);
    assert(published.rssiDbm==-72 && published.snr==7 && published.linkQuality==98);
    linkStats.uplink_RSSI_2=80;checkSendLinkStatsToFc(202);
    assert(publications==2 && published.rssiDbm==-80);
    connectionState=disconnected;checkSendLinkStatsToFc(303);
    assert(publications==3);
}
''')
            binary = str(Path(tmp) / 'test')
            subprocess.run(['c++', '-std=c++11', '-Wno-vla-cxx-extension', str(cpp), '-o', binary], check=True)
            subprocess.run([binary], check=True)


if __name__ == '__main__':
    unittest.main()
