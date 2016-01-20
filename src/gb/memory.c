/* Copyright (c) 2013-2016 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "memory.h"

#include "gb/gb.h"
#include "gb/io.h"

#include "util/memory.h"

static void _GBMBCNone(struct GBMemory* memory, uint16_t address, uint8_t value) {
	// TODO: Log game error
	UNUSED(memory);
	UNUSED(address);
	UNUSED(value);
}

static void _GBMBC1(struct GBMemory*, uint16_t address, uint8_t value);
static void _GBMBC2(struct GBMemory*, uint16_t address, uint8_t value);
static void _GBMBC3(struct GBMemory*, uint16_t address, uint8_t value);
static void _GBMBC4(struct GBMemory*, uint16_t address, uint8_t value);
static void _GBMBC5(struct GBMemory*, uint16_t address, uint8_t value);

static void GBSetActiveRegion(struct LR35902Core* cpu, uint16_t address) {
	// TODO
}

void GBMemoryInit(struct GB* gb) {
	struct LR35902Core* cpu = gb->cpu;
	cpu->memory.load16 = GBLoad16;
	cpu->memory.load8 = GBLoad8;
	cpu->memory.store16 = GBStore16;
	cpu->memory.store8 = GBStore8;
	cpu->memory.setActiveRegion = GBSetActiveRegion;

	gb->memory.wram = 0;
	gb->memory.wramBank = 0;
	gb->memory.rom = 0;
	gb->memory.romBank = 0;
	gb->memory.romSize = 0;
	gb->memory.mbcType = GB_MBC_NONE;
	gb->memory.mbc = 0;

	memset(gb->memory.hram, 0, sizeof(gb->memory.hram));

	GBIOInit(gb);
}

void GBMemoryDeinit(struct GB* gb) {
	mappedMemoryFree(gb->memory.wram, GB_SIZE_WORKING_RAM);
	if (gb->memory.rom) {
		mappedMemoryFree(gb->memory.rom, gb->memory.romSize);
	}
}

void GBMemoryReset(struct GB* gb) {
	if (gb->memory.wram) {
		mappedMemoryFree(gb->memory.wram, GB_SIZE_WORKING_RAM);
	}
	gb->memory.wram = anonymousMemoryMap(GB_SIZE_WORKING_RAM);
	gb->memory.wramBank = &gb->memory.wram[GB_SIZE_WORKING_RAM_BANK0];
	gb->memory.romBank = &gb->memory.rom[GB_SIZE_CART_BANK0];

	const struct GBCartridge* cart = &gb->memory.rom[0x100];
	switch (cart->type) {
	case 0:
	case 8:
	case 9:
		gb->memory.mbc = _GBMBCNone;
		gb->memory.mbcType = GB_MBC_NONE;
		break;
	case 1:
	case 2:
	case 3:
		gb->memory.mbc = _GBMBC1;
		gb->memory.mbcType = GB_MBC1;
		break;
	case 5:
	case 6:
		gb->memory.mbc = _GBMBC2;
		gb->memory.mbcType = GB_MBC2;
		break;
	case 0x0F:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		gb->memory.mbc = _GBMBC3;
		gb->memory.mbcType = GB_MBC3;
		break;
	case 0x15:
	case 0x16:
	case 0x17:
		gb->memory.mbc = _GBMBC4;
		gb->memory.mbcType = GB_MBC4;
		break;
	default:
		// TODO: Log
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
		gb->memory.mbc = _GBMBC5;
		gb->memory.mbcType = GB_MBC5;
		break;
	}

	if (!gb->memory.wram) {
		GBMemoryDeinit(gb);
	}
}

uint16_t GBLoad16(struct LR35902Core* cpu, uint16_t address) {
	// TODO
}

uint8_t GBLoad8(struct LR35902Core* cpu, uint16_t address) {
	struct GB* gb = (struct GB*) cpu->master;
	struct GBMemory* memory = &gb->memory;
	switch (address >> 12) {
	case GB_REGION_CART_BANK0:
	case GB_REGION_CART_BANK0 + 1:
	case GB_REGION_CART_BANK0 + 2:
	case GB_REGION_CART_BANK0 + 3:
		return memory->rom[address & (GB_SIZE_CART_BANK0 - 1)];
	case GB_REGION_CART_BANK1:
	case GB_REGION_CART_BANK1 + 1:
	case GB_REGION_CART_BANK1 + 2:
	case GB_REGION_CART_BANK1 + 3:
		return memory->romBank[address & (GB_SIZE_CART_BANK0 - 1)];
	case GB_REGION_VRAM:
	case GB_REGION_VRAM + 1:
		return gb->video.vram[address & (GB_SIZE_VRAM - 1)];
	case GB_REGION_EXTERNAL_RAM:
	case GB_REGION_EXTERNAL_RAM + 1:
		// TODO
		return 0;
	case GB_REGION_WORKING_RAM_BANK0:
	case GB_REGION_WORKING_RAM_BANK0 + 2:
		return memory->wram[address & (GB_SIZE_WORKING_RAM_BANK0 - 1)];
	case GB_REGION_WORKING_RAM_BANK1:
		return memory->wramBank[address & (GB_SIZE_WORKING_RAM_BANK0 - 1)];
	default:
		if (address < GB_BASE_OAM) {
			return memory->wramBank[address & (GB_SIZE_WORKING_RAM_BANK0 - 1)];
		}
		if (address < GB_BASE_IO) {
			// TODO
			return 0;
		}
		if (address < GB_BASE_HRAM) {
			return GBIORead(gb, address & (GB_SIZE_IO - 1));
		}
		if (address < GB_BASE_IE) {
			return memory->hram[address & GB_SIZE_HRAM];
		}
		return GBIORead(gb, REG_IE);
	}
}

void GBStore16(struct LR35902Core* cpu, uint16_t address, int16_t value) {
	// TODO
}

void GBStore8(struct LR35902Core* cpu, uint16_t address, int8_t value) {
	struct GB* gb = (struct GB*) cpu->master;
	struct GBMemory* memory = &gb->memory;
	switch (address >> 12) {
	case GB_REGION_CART_BANK0:
	case GB_REGION_CART_BANK0 + 1:
	case GB_REGION_CART_BANK0 + 2:
	case GB_REGION_CART_BANK0 + 3:
	case GB_REGION_CART_BANK1:
	case GB_REGION_CART_BANK1 + 1:
	case GB_REGION_CART_BANK1 + 2:
	case GB_REGION_CART_BANK1 + 3:
		memory->mbc(memory, address, value);
		return;
	case GB_REGION_VRAM:
	case GB_REGION_VRAM + 1:
		// TODO: Block access in wrong modes
		gb->video.vram[address & (GB_SIZE_VRAM - 1)] = value;
		gb->video.renderer->writeVRAM(gb->video.renderer, address & (GB_SIZE_VRAM - 1));
		return;
	case GB_REGION_EXTERNAL_RAM:
	case GB_REGION_EXTERNAL_RAM + 1:
		// TODO
		return;
	case GB_REGION_WORKING_RAM_BANK0:
	case GB_REGION_WORKING_RAM_BANK0 + 2:
		memory->wram[address & (GB_SIZE_WORKING_RAM_BANK0 - 1)] = value;
		return;
	case GB_REGION_WORKING_RAM_BANK1:
		memory->wramBank[address & (GB_SIZE_WORKING_RAM_BANK0 - 1)] = value;
		return;
	default:
		if (address < GB_BASE_OAM) {
			memory->wramBank[address & (GB_SIZE_WORKING_RAM_BANK0 - 1)] = value;
		} else if (address < GB_BASE_IO) {
			// TODO
		} else if (address < GB_BASE_HRAM) {
			GBIOWrite(gb, address & (GB_SIZE_IO - 1), value);
		} else if (address < GB_BASE_IE) {
			memory->hram[address & GB_SIZE_HRAM] = value;
		} else {
			GBIOWrite(gb, REG_IE, value);
		}
	}
}

uint16_t GBView16(struct LR35902Core* cpu, uint16_t address);
uint8_t GBView8(struct LR35902Core* cpu, uint16_t address);

void GBPatch16(struct LR35902Core* cpu, uint16_t address, int16_t value, int16_t* old);
void GBPatch8(struct LR35902Core* cpu, uint16_t address, int8_t value, int8_t* old);

static void _switchBank(struct GBMemory* memory, int bank) {
	size_t bankStart = bank * GB_SIZE_CART_BANK0;
	if (bankStart + GB_SIZE_CART_BANK0 > memory->romSize) {
		// TODO: Log
		return;
	}
	memory->romBank = &memory->rom[bankStart];
	memory->currentBank = bank;
}

void _GBMBC1(struct GBMemory* memory, uint16_t address, uint8_t value) {
	int bank = value & 0x1F;
	switch (address >> 13) {
	case 0x0:
		// TODO
		break;
	case 0x1:
		if (!bank) {
			++bank;
		}
		_switchBank(memory, bank | (memory->currentBank & 0x60));
		break;
	}
}

void _GBMBC2(struct GBMemory* memory, uint16_t address, uint8_t value) {
	// TODO
}

void _GBMBC3(struct GBMemory* memory, uint16_t address, uint8_t value) {
	int bank = value & 0x7F;
	switch (address >> 13) {
	case 0x0:
		// TODO
		break;
	case 0x1:
		if (!bank) {
			++bank;
		}
		_switchBank(memory, bank);
		break;
	}
}

void _GBMBC4(struct GBMemory* memory, uint16_t address, uint8_t value) {
	// TODO
}

void _GBMBC5(struct GBMemory* memory, uint16_t address, uint8_t value) {
	int bank = value & 0x7F;
	switch (address >> 13) {
	case 0x0:
		// TODO
		break;
	case 0x1:
		_switchBank(memory, bank);
		break;
	}
}
