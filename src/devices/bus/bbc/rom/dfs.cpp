// license:BSD-3-Clause
// copyright-holders:Nigel Barnes
/***************************************************************************

    BBC Micro E00 DFS emulation

    Comprises of 8K ROM and 2K/4K? RAM on a carrier board, with flying lead
    to RW line to enable writing to RAM.

***************************************************************************/

#include "emu.h"
#include "dfs.h"


//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

DEFINE_DEVICE_TYPE(BBC_DFSE00, bbc_dfse00_device, "bbc_dfse00", "BBC Micro E00 DFS")

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  bbc_rom_device - constructor
//-------------------------------------------------

bbc_dfse00_device::bbc_dfse00_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, BBC_DFSE00, tag, owner, clock)
	, device_bbc_rom_interface(mconfig, *this)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void bbc_dfse00_device::device_start()
{
}

//-------------------------------------------------
//  read
//-------------------------------------------------

uint8_t bbc_dfse00_device::read(offs_t offset)
{
	if (offset < get_rom_size())
		return get_rom_base()[offset & (get_rom_size() - 1)];
	else
		return get_ram_base()[offset & (get_ram_size() - 1)];
}

//-------------------------------------------------
//  write
//-------------------------------------------------

void bbc_dfse00_device::write(offs_t offset, uint8_t data)
{
	get_ram_base()[offset & (get_ram_size() - 1)] = data;
}
