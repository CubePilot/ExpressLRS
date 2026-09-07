#include "SX1280_hal.h"
#include "CubeRacerRadio.h"
#include "RFAMP_hal.h"
#include <cassert>
#include <cstdio>
#include <vector>
extern SX1280Hal hal;
extern RFAMP_hal RFAMP;
static bool ready=true,busy=false;
static bool txDone=false;
static unsigned transfers=0, failAt=0, resetLow=0, resetHigh=0;
void digitalWrite(int pin,int value) { assert(pin==83);if(value)resetHigh++;else resetLow++; }
void pinMode(int,int) { assert(false); }
static std::vector<std::vector<uint8_t>> commands;
bool cuberacerRadioGrant(uint32_t) { return true; }
bool cuberacerRadioInit(void (*)()) { return ready; }
void cuberacerRadioEnd() {}
int digitalRead(int pin) { assert(pin==67);return busy?1:0; }
bool cuberacerRadioTransfer(uint8_t *p,unsigned size) {
    if(!ready)return false;
    transfers++;commands.emplace_back(p,p+size);
    for(unsigned i=0;i<size;i++)p[i]=0;
    if(size==5 && commands.back()[0]==0x19)p[4]=commands.back()[2]==0x53?0xa9:0xb7;
    if(txDone && size==4 && commands.back()[0]==SX1280_RADIO_GET_IRQSTATUS)p[3]=1;
    if (failAt && transfers==failAt) ready=false;
    return ready;
}
CubeRacer::RadioFault cuberacerRadioFault() { return ready?CubeRacer::RadioFault::None:CubeRacer::RadioFault::SpiTimeout; }
int main() {
    uint8_t data[2]={0xa5,0xa5};busy=true;
    hal.WriteCommand(SX1280_RADIO_SET_STANDBY,0,SX12XX_Radio_1);
    hal.WriteRegister(0x891,data,2,SX12XX_Radio_1);
    hal.WriteBuffer(0,data,2,SX12XX_Radio_1);
    hal.ReadRegister(0x153,data,2,SX12XX_Radio_1);assert(data[0]==0 && data[1]==0);
    data[0]=data[1]=0xa5;hal.ReadBuffer(0,data,2,SX12XX_Radio_1);assert(data[0]==0 && data[1]==0);
    data[0]=0xa5;hal.ReadCommand(SX1280_RADIO_GET_STATUS,data,1,SX12XX_Radio_1);assert(data[0]==0);
    assert(transfers==6); // Every command proceeds after a BUSY timeout.
    transfers=0;commands.clear();ready=false;
    SX1280Driver driver;assert(!driver.Begin(2400000000U,2480000000U));assert(transfers==0);
    ready=true;assert(driver.Begin(2400000000U,2480000000U));assert(transfers>0);
    assert(commands[0]==std::vector<uint8_t>({0x80,0}));
    assert(commands[1]==std::vector<uint8_t>({0x19,0x01,0x53,0,0}));
    assert(commands[2]==std::vector<uint8_t>({0x19,0x01,0x54,0,0}));
    assert(resetLow==1 && resetHigh==1); // Shared HAL reset drives PF5.
    RFAMP.init();RFAMP.TXenable(SX12XX_Radio_1);RFAMP.RXenable();RFAMP.TXRXdisable();
    assert(resetLow==1 && resetHigh==1); // Undefined RFAMP pins produce no writes.
    const unsigned beginTransfers=transfers;
    for(unsigned failure=1;failure<=beginTransfers;failure++) {
        transfers=0;commands.clear();ready=true;failAt=failure;
        assert(!driver.Begin(2400000000U,2480000000U));
        assert(transfers==failure); // No next command after an SPI failure.
    }
    printf("CubeRacer SX1280 advisory BUSY and SPI-failure tests passed (%u initialization failure positions)\n",beginTransfers);
    ready=true;failAt=0;txDone=true;
    driver.TXdoneCallback=[](){};
    for(int power=-30;power<=30;power++) {
        driver.SetOutputPower(-18);hal.IsrCallback_1();commands.clear();
        driver.SetOutputPower(power);hal.IsrCallback_1();
        bool committed=false;
        for(const auto &cmd:commands)if(cmd[0]==SX1280_RADIO_SET_TXPARAMS)
            { committed=true;assert(cmd[1]<=28); } // -18 dBm encodes as 0; cap at +10.
        if(power>-18)assert(committed);
    }
    puts("CubeRacer driver output-power ceiling passed");
    const unsigned beforeEnd=transfers;
    driver.End();assert(transfers==beforeEnd);
    assert(!hal.IsrCallback_1 && !hal.IsrCallback_2);
    puts("CubeRacer driver shutdown requires no radio command");
}
