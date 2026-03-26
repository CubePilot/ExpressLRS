"""
STM32 DFU flasher using pyusb.
Handles STM32H7 bootloader quirk where ERASE_PAGE returns dfuERROR
state that requires clear_status to recover (dfu-util ticket #88).
"""
import usb.core
import usb.util
import struct
import time
import sys

# DFU constants
DFU_DETACH = 0
DFU_DNLOAD = 1
DFU_UPLOAD = 2
DFU_GETSTATUS = 3
DFU_CLRSTATUS = 4
DFU_GETSTATE = 5
DFU_ABORT = 6

# DFU states
DFU_STATE_IDLE = 2
DFU_STATE_DNBUSY = 4
DFU_STATE_DNLOAD_IDLE = 5
DFU_STATE_ERROR = 10

# DfuSe commands
DFUSE_CMD_SET_ADDRESS = 0x21
DFUSE_CMD_ERASE = 0x41

STM32_VID = 0x0483
STM32_DFU_PID = 0xDF11
H7_SECTOR_SIZE = 0x20000  # 128KB


def find_device(retries=10):
    for i in range(retries):
        dev = usb.core.find(idVendor=STM32_VID, idProduct=STM32_DFU_PID)
        if dev is not None:
            try:
                dev.set_configuration()
            except usb.core.USBError:
                pass  # Already configured
            usb.util.claim_interface(dev, 0)
            return dev
        time.sleep(1)
    raise RuntimeError("No STM32 DFU device found")


def get_status(dev):
    data = dev.ctrl_transfer(0xA1, DFU_GETSTATUS, 0, 0, 6)
    status = data[0]
    poll_timeout = data[1] | (data[2] << 8) | (data[3] << 16)
    state = data[4]
    return status, poll_timeout, state


def clear_status(dev):
    dev.ctrl_transfer(0x21, DFU_CLRSTATUS, 0, 0, None)


def wait_idle(dev, timeout=10):
    """Wait for dfuIDLE or dfuDNLOAD-IDLE, clearing errors as needed."""
    state = -1
    start = time.time()
    while time.time() - start < timeout:
        try:
            status, poll_ms, state = get_status(dev)
        except usb.core.USBError:
            time.sleep(0.5)
            continue
        if state in (DFU_STATE_IDLE, DFU_STATE_DNLOAD_IDLE):
            return state
        if state == DFU_STATE_ERROR:
            clear_status(dev)
        elif state == DFU_STATE_DNBUSY:
            time.sleep(max(poll_ms / 1000.0, 0.1))
        else:
            time.sleep(0.1)
    raise RuntimeError("Timeout waiting for DFU idle (state=%d)" % state)


def dfuse_mass_erase(dev):
    """Mass erase with H7 STALL recovery (dfu-util ticket #88)."""
    sys.stdout.write("Mass erasing...")
    sys.stdout.flush()

    # DfuSe mass erase: DNLOAD with just command byte 0x41 (no address)
    dev.ctrl_transfer(0x21, DFU_DNLOAD, 0, 0, struct.pack('<B', DFUSE_CMD_ERASE))

    # get_status triggers erase — H7 STALLs (pipe error)
    # After STALL, keep trying clear+get cycles until bootloader recovers
    for attempt in range(60):
        try:
            status, poll_ms, state = get_status(dev)
            if state == DFU_STATE_IDLE or state == DFU_STATE_DNLOAD_IDLE:
                sys.stdout.write(" done\n")
                sys.stdout.flush()
                return
            if state == DFU_STATE_DNBUSY:
                sys.stdout.write(".")
                sys.stdout.flush()
                time.sleep(max(poll_ms / 1000.0, 2.0))
                continue
            if state == DFU_STATE_ERROR:
                clear_status(dev)
                time.sleep(0.5)
                continue
        except usb.core.USBError:
            try:
                clear_status(dev)
            except usb.core.USBError:
                pass
            sys.stdout.write(".")
            sys.stdout.flush()
            time.sleep(1)

    raise RuntimeError("Mass erase timeout")


def dfuse_set_address(dev, address):
    cmd = struct.pack('<BI', DFUSE_CMD_SET_ADDRESS, address)
    dev.ctrl_transfer(0x21, DFU_DNLOAD, 0, 0, cmd)
    wait_idle(dev)


def dfuse_download_block(dev, block_num, data):
    dev.ctrl_transfer(0x21, DFU_DNLOAD, block_num, 0, data)
    wait_idle(dev)


def flash(filename, base_address=0x08000000):
    with open(filename, 'rb') as f:
        firmware = f.read()

    print("Connecting to DFU device...")
    dev = find_device()
    wait_idle(dev)

    fw_size = len(firmware)
    num_sectors = (fw_size + H7_SECTOR_SIZE - 1) // H7_SECTOR_SIZE
    print("Firmware: %d bytes (%d sectors)" % (fw_size, num_sectors))

    # Mass erase instead of per-sector (H7 sector erase has STALL issues)
    dfuse_mass_erase(dev)

    # Write firmware in 1024-byte blocks (DFU transfer size)
    xfer_size = 1024
    dfuse_set_address(dev, base_address)

    total_blocks = (fw_size + xfer_size - 1) // xfer_size
    for i in range(total_blocks):
        offset = i * xfer_size
        block = firmware[offset:offset + xfer_size]
        # DfuSe block numbering starts at 2
        dfuse_download_block(dev, i + 2, block)
        pct = (i + 1) * 100 // total_blocks
        sys.stdout.write("\rWriting: [%-25s] %3d%%" % ('#' * (pct // 4), pct))
        sys.stdout.flush()
    print(" done")

    # Leave DFU mode — send empty download then reset
    print("Starting firmware...")
    dfuse_set_address(dev, base_address)
    dev.ctrl_transfer(0x21, DFU_DNLOAD, 0, 0, None)
    try:
        get_status(dev)
    except:
        pass  # Device may have already reset

    print("Flash complete!")


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='STM32 DFU Flasher')
    parser.add_argument('firmware', help='Firmware .bin file')
    parser.add_argument('--address', type=lambda x: int(x, 0), default=0x08000000, help='Base address')
    args = parser.parse_args()
    flash(args.firmware, args.address)
