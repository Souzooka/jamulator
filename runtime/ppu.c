#include "ppu.h"
#include "stdlib.h"
#include "string.h"
#include "rom.h"

uint32_t PPU_PALETTE_RGB[] = {
    0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E,
    0x6E0040, 0x6C0600, 0x561D00, 0x333500, 0x0B4800,
    0x005200, 0x004F08, 0x00404D, 0x000000, 0x000000,
    0x000000, 0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE,
    0xA01ACC, 0xB71E7B, 0xB53120, 0x994E00, 0x6B6D00,
    0x388700, 0x0C9300, 0x008F32, 0x007C8D, 0x000000,
    0x000000, 0x000000, 0xFFFEFF, 0x64B0FF, 0x9290FF,
    0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22,
    0xBCBE00, 0x88D800, 0x5CE430, 0x45E082, 0x48CDDE,
    0x4F4F4F, 0x000000, 0x000000, 0xFFFEFF, 0xC0DFFF,
    0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5,
    0xF7D8A5, 0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC,
    0xB5EBF2, 0xB8B8B8, 0x000000, 0x000000,
};

Ppu* Ppu_new() {
    Ppu* p = (Ppu*) malloc(sizeof(Ppu));
    memset(p, 0, sizeof(Ppu));

    p->registers.writeLatch = true;
    p->overscanEnabled = true;
    p->spriteLimitEnabled = true;
    p->scanline = 241;

    for (unsigned int i = 0; i < 0x400; ++i) {
        p->attributeShift[i] = ((i >> 4) & 0x04) | (i & 0x02);
        p->attributeLocation[i] = ((i >> 2) & 0x07) | (((i >> 4) & 0x38) | 0x3C0);
    }

    p->palettebufferSize = 0xf000;
    p->palettebuffer = malloc(sizeof(Pixel) * p->palettebufferSize);

    p->framebufferSize = 0xefe0;
    p->framebuffer = malloc(sizeof(uint32_t) * p->framebufferSize);

    return p;
}

void Ppu_dispose(Ppu* p) {
    free(p->palettebuffer);
    free(p->framebuffer);
    free(p);
}

// Writes to mirrored regions of VRAM
void Ppu_writeMirroredVram(Ppu* p, int a, uint8_t v) {
    if (a >= 0x3F00) {
        if ((a & 0xF) == 0) {
            a = 0;
        }
        p->paletteRam[a&0x1F] = v;
    } else {
        Nametable_writeNametableData(&p->nametables, a-0x1000, v);
    }
}

// $2000
void Ppu_writeControl(Ppu* p, uint8_t v) {
    p->registers.control = v;

    // Control flag
    // 7654 3210
    // |||| ||||
    // |||| ||++- Base nametable address
    // |||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
    // |||| |+--- VRAM address increment per CPU read/write of PPUDATA
    // |||| |     (0: increment by 1, going across; 1: increment by 32, going down)
    // |||| +---- Sprite pattern table address for 8x8 sprites
    // ||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
    // |||+------ Background pattern table address (0: $0000; 1: $1000)
    // ||+------- Sprite size (0: 8x8; 1: 8x16)
    // |+-------- PPU master/slave select (has no effect on the NES)
    // +--------- Generate an NMI at the start of the
    //            vertical blanking interval (0: off; 1: on)
    p->flags.baseNametableAddress = v & 0x03;
    p->flags.vramAddressInc = (v >> 2) & 0x01;
    p->flags.spritePatternAddress = (v >> 3) & 0x01;
    p->flags.backgroundPatternAddress = (v >> 4) & 0x01;
    p->flags.spriteSize = (v >> 5) & 0x01;
    p->flags.nmiOnVblank = (v >> 7) & 0x01;

    int intBaseNametableAddr = p->flags.baseNametableAddress;
    p->registers.vramLatch = (p->registers.vramLatch & 0xF3FF) | (intBaseNametableAddr << 10);
}

// $2001
void Ppu_writeMask(Ppu* p, uint8_t v) {
    p->registers.mask = v;

    // 76543210
    // ||||||||
    // |||||||+- Grayscale (0: normal color; 1: produce a monochrome display)
    // ||||||+-- 1: Show background in leftmost 8 pixels of screen; 0: Hide
    // |||||+--- 1: Show sprites in leftmost 8 pixels of screen; 0: Hide
    // ||||+---- 1: Show background
    // |||+----- 1: Show sprites
    // ||+------ Intensify reds (and darken other colors)
    // |+------- Intensify greens (and darken other colors)
    // +-------- Intensify blues (and darken other colors)
    p->masks.grayscale = ((v&0x01) == 0x01);
    p->masks.showBackgroundOnLeft = (((v >> 1) & 0x01) == 0x01);
    p->masks.showSpritesOnLeft = (((v >> 2) & 0x01) == 0x01);
    p->masks.showBackground = (((v >> 3) & 0x01) == 0x01);
    p->masks.showSprites = (((v >> 4) & 0x01) == 0x01);
    p->masks.intensifyReds = (((v >> 5) & 0x01) == 0x01);
    p->masks.intensifyGreens = (((v >> 6) & 0x01) == 0x01);
    p->masks.intensifyBlues = (((v >> 7) & 0x01) == 0x01);
}

// $4014
void Ppu_writeDma(Ppu* p, uint8_t v) {
    // Fill sprite RAM
    int intV = v;
    int addr = intV * 0x100;
    for (int i = 0; i < 0x100; ++i) {
        uint8_t d = p->readRam(addr + i);
        p->spriteRam[i] = d;
        Ppu_updateBufferedSpriteMem(p, i, d);
    }
}

void Ppu_raster(Ppu* p) {
    int length = p->palettebufferSize;
    for (int i = length - 1; i >= 0; --i) {
        int y = i / 256;
        int x = i - (y * 256);

        uint32_t color = p->palettebuffer[i].color;

        int width = 256;

        if (p->overscanEnabled) {
            if (y < 8 || y > 231 || x < 8 || x > 247) {
                continue;
            } else {
                y -= 8;
                x -= 8;
            }

            width = 240;

            if (p->framebufferSize == 0xf000) {
                free(p->framebuffer);
                p->framebufferSize = 0xefe0;
                p->framebuffer = malloc(sizeof(uint32_t) * p->framebufferSize);
            }
        } else {
            if (p->framebufferSize == 0xefe0) {
                free(p->framebuffer);
                p->framebufferSize = 0xf000;
                p->framebuffer = malloc(sizeof(uint32_t) * p->framebufferSize);
            }
        }

        p->framebuffer[(y*width)+x] = color;
        p->palettebuffer[i].value = 0;
        p->palettebuffer[i].pindex = -1;
    }

    if (p->render != NULL) {
        p->render();
    }
}


void Ppu_step(Ppu* p) {
    if (p->scanline == 240) {
        if (p->cycle == 1) {
            if (!p->suppressVbl) {
                // We're in VBlank
                Ppu_setStatus(p, STATUS_VBLANK_STARTED);
                p->cycleCount = 0;
            }
            if (p->flags.nmiOnVblank == 0x1 && !p->suppressNmi) {
                // Request NMI
                p->vblankInterrupt();
            }
            Ppu_raster(p);
        }
    } else if (p->scanline == 260) {
        // End of vblank
        if (p->cycle == 1) {
            // Clear VBlank flag
            Ppu_clearStatus(p, STATUS_VBLANK_STARTED);
            p->cycleCount = 0;
        } else if(p->cycle == 341) {
            p->scanline = -1;
            p->cycle = 1;
            p->frameCount++;
            return;
        }
    } else if (p->scanline < 240 && p->scanline > -1) {
        if (p->cycle == 254) {
            if (p->masks.showBackground) {
                Ppu_renderTileRow(p);
            }

            if (p->masks.showSprites) {
                Ppu_evaluateScanlineSprites(p, p->scanline);
            }
        } else if (p->cycle == 256) {
            if (p->masks.showBackground) {
                Ppu_updateEndScanlineRegisters(p);
            }
        }
    } else if (p->scanline == -1) {
        if (p->cycle == 1) {
            Ppu_clearStatus(p, STATUS_SPRITE0HIT);
            Ppu_clearStatus(p, STATUS_SPRITE_OVERFLOW);
        } else if (p->cycle == 304) {
            // Copy scroll latch into VRAMADDR register
            if (p->masks.showBackground || p->masks.showSprites) {
                p->registers.vramAddress = p->registers.vramLatch;
            }
        }
    }

    if (p->cycle == 341) {
        p->cycle = 0;
        p->scanline++;
    }

    p->cycle++;
    p->cycleCount++;
}


void Ppu_updateEndScanlineRegisters(Ppu* p) {
    // *******************************************************
    //  TODO: Some documentation implies that the X increment
    //  should occur 34 times per scanline. These may not be
    //  necessary.
    // *******************************************************

    // Flip bit 10 on wraparound
    if ((p->registers.vramAddress&0x1F) == 0x1F) {
        // If rendering is enabled, at the end of a scanline
        // copy bits 10 and 4-0 from VRAM latch into VRAMADDR
        p->registers.vramAddress ^= 0x41F;
    } else {
        p->registers.vramAddress++;
    }

    // Flip bit 10 on wraparound
    if ((p->registers.vramAddress&0x1F) == 0x1F) {
        // If rendering is enabled, at the end of a scanline
        // copy bits 10 and 4-0 from VRAM latch into VRAMADDR
        p->registers.vramAddress ^= 0x41F;
    } else {
        p->registers.vramAddress++;
    }

    if (p->masks.showBackground || p->masks.showSprites) {
        // Scanline has ended
        if ((p->registers.vramAddress&0x7000) == 0x7000) {
            int tmp = p->registers.vramAddress & 0x3E0;
            p->registers.vramAddress &= 0xFFF;

            if (tmp == 0x3A0) {
                p->registers.vramAddress ^= 0xBA0;
            } else if (tmp == 0x3e0) {
                p->registers.vramAddress ^= 0x3E0;
            } else {
                p->registers.vramAddress += 0x20;
            }
        } else {
            // Increment the fine-Y
            p->registers.vramAddress += 0x1000;
        }

        p->registers.vramAddress = (p->registers.vramAddress & 0x7BE0) | (p->registers.vramLatch & 0x41F);
    }
}

void Ppu_clearStatus(Ppu* p, uint8_t s) {
    uint8_t current = p->registers.status;

    switch (s) {
    case STATUS_SPRITE_OVERFLOW:
        current = current & 0xDF;
        break;
    case STATUS_SPRITE0HIT:
        current = current & 0xBF;
        break;
    case STATUS_VBLANK_STARTED:
        current = current & 0x7F;
        break;
    }

    p->registers.status = current;
}

void Ppu_setStatus(Ppu* p, uint8_t s) {
    int current = p->registers.status;

    switch (s) {
    case STATUS_SPRITE_OVERFLOW:
        current = current | 0x20;
        break;
    case STATUS_SPRITE0HIT:
        current = current | 0x40;
        break;
    case STATUS_VBLANK_STARTED:
        current = current | 0x80;
        break;
    }

    p->registers.status = current;
}

// $2002
uint8_t Ppu_readStatus(Ppu* p) {
    p->registers.writeLatch = true;
    uint8_t s = p->registers.status;

    if (p->cycle == 1 && p->scanline == 240) {
        s &= 0x7F;
        p->suppressNmi = true;
        p->suppressVbl = true;
    } else {
        p->suppressNmi = false;
        p->suppressVbl = false;
        // Clear VBlank flag
        Ppu_clearStatus(p, STATUS_VBLANK_STARTED);
    }

    return s;
}

// $2003
void Ppu_writeOamAddress(Ppu* p, uint8_t v) {
    p->registers.spriteRamAddress = v;
}

// $2004
void Ppu_writeOamData(Ppu* p, uint8_t v) {
    p->spriteRam[p->registers.spriteRamAddress] = v;

    Ppu_updateBufferedSpriteMem(p, p->registers.spriteRamAddress, v);

    p->registers.spriteRamAddress++;
    p->registers.spriteRamAddress %= 0x100;
}

void Ppu_updateBufferedSpriteMem(Ppu* p, int a, uint8_t v) {
    int i = a / 4;

    switch (a % 4) {
    case 0x0:
        p->spriteData.yCoordinates[i] = v;
        break;
    case 0x1:
        p->spriteData.tiles[i] = v;
        break;
    case 0x2:
        // Attribute
        p->spriteData.attributes[i] = v;
        break;
    case 0x3:
        p->spriteData.xCoordinates[i] = v;
        break;
    }
}


// $2004
uint8_t Ppu_readOamData(Ppu* p) {
    return p->spriteRam[p->registers.spriteRamAddress];
}


// $2005
void Ppu_writeScroll(Ppu* p, uint8_t v) {
    int intValue = v;
    if (p->registers.writeLatch) {
        p->registers.vramLatch = p->registers.vramLatch & 0x7FE0;
        p->registers.vramLatch = p->registers.vramLatch | ((intValue & 0xF8) >> 3);
        p->registers.fineX = v & 0x07;
    } else {
        p->registers.vramLatch = p->registers.vramLatch & 0xC1F;
        p->registers.vramLatch = p->registers.vramLatch | (((intValue & 0xF8) << 2) | ((intValue & 0x07) << 12));
    }

    p->registers.writeLatch = !p->registers.writeLatch;
}


// $2006
void Ppu_writeAddress(Ppu* p, uint8_t v) {
    int intValue = v;
    if (p->registers.writeLatch) {
        p->registers.vramLatch = p->registers.vramLatch & 0xFF;
        p->registers.vramLatch = p->registers.vramLatch | ((intValue & 0x3F) << 8);
    } else {
        p->registers.vramLatch = p->registers.vramLatch & 0x7F00;
        p->registers.vramLatch = p->registers.vramLatch | intValue;
        p->registers.vramAddress = p->registers.vramLatch;
    }

    p->registers.writeLatch = !p->registers.writeLatch;
}


// $2007
void Ppu_writeData(Ppu* p, uint8_t v) {
    if (p->registers.vramAddress > 0x3000) {
        Ppu_writeMirroredVram(p, p->registers.vramAddress, v);
    } else if (p->registers.vramAddress >= 0x2000 && p->registers.vramAddress < 0x3000) {
        // Nametable mirroring
        Nametable_writeNametableData(&p->nametables, p->registers.vramAddress, v);
    } else {
        p->vram[p->registers.vramAddress&0x3FFF] = v;
    }

    Ppu_incrementVramAddress(p);
}

// $2007
uint8_t Ppu_readData(Ppu* p) {
    uint8_t r;
    // Reads from $2007 are buffered with a
    // 1-byte delay
    if (p->registers.vramAddress >= 0x2000 && p->registers.vramAddress < 0x3000) {
        r = p->registers.vramDataBuffer;
        p->registers.vramDataBuffer = Nametable_readNametableData(&p->nametables, p->registers.vramAddress);
    } else if (p->registers.vramAddress < 0x3F00) {
        r = p->registers.vramDataBuffer;
        p->registers.vramDataBuffer = p->vram[p->registers.vramAddress];
    } else {
        int bufferAddress = p->registers.vramAddress - 0x1000;
        if (bufferAddress >= 0x2000 && bufferAddress < 0x3000) {
            p->registers.vramDataBuffer = Nametable_readNametableData(&p->nametables, bufferAddress);
        } else {
            p->registers.vramDataBuffer = p->vram[bufferAddress];
        }

        int a = p->registers.vramAddress;
        if ((a&0xF) == 0) {
            a = 0;
        }

        r = p->paletteRam[a&0x1F];
    }

    Ppu_incrementVramAddress(p);

    return r;
}


void Ppu_incrementVramAddress(Ppu* p) {
    if (p->flags.vramAddressInc == 0x01) {
        p->registers.vramAddress = p->registers.vramAddress + 0x20;
    } else {
        p->registers.vramAddress = p->registers.vramAddress + 0x01;
    }
}

int Ppu_sprPatternTableAddress(Ppu* p, int i) {
    if ((p->flags.spriteSize&0x01) != 0x0) {
        // 8x16 Sprites
        if ((i & 0x01) != 0) {
            return 0x1000 | ((i >> 1) * 0x20);
        } else {
            return ((i >> 1) * 0x20);
        }

    }

    // 8x8 Sprites
    int a = p->flags.spritePatternAddress == 0x01 ? 0x1000 : 0x0;

    return i*0x10 + a;
}

int Ppu_bgPatternTableAddress(Ppu* p, uint8_t i) {
    int a = p->flags.backgroundPatternAddress == 0x01 ? 0x1000 : 0x0;
    return (i << 4) | (p->registers.vramAddress >> 12) | a;
}

void Ppu_fetchTileAttributes(Ppu* p, PpuTileAttributes* attrs) {
    // Load first two tiles into shift registers at start, then load
    // one per loop and shift the other back out

    int intAttrLoc = p->attributeLocation[p->registers.vramAddress&0x3FF];
    int attrAddr = 0x23C0 | (p->registers.vramAddress & 0xC00) | intAttrLoc;
    unsigned int shift = p->attributeShift[p->registers.vramAddress&0x3FF];
    uint8_t attr = ((Nametable_readNametableData(&p->nametables, attrAddr) >> shift) & 0x03) << 2;

    uint8_t index = Nametable_readNametableData(&p->nametables, p->registers.vramAddress);
    int t = Ppu_bgPatternTableAddress(p, index);

    // Flip bit 10 on wraparound
    if ((p->registers.vramAddress&0x1F) == 0x1F) {
        // If rendering is enabled, at the end of a scanline
        // copy bits 10 and 4-0 from VRAM latch into VRAMADDR
        p->registers.vramAddress ^= 0x41F;
    } else {
        p->registers.vramAddress++;
    }

    attrs->low = p->vram[t];
    attrs->high = p->vram[t+8];
    attrs->attr = attr;
}

void Ppu_renderTileRow(Ppu* p) {
    // Generates each tile, one scanline at a time
    // and applies the palette

    // Move first tile into shift registers
    PpuTileAttributes tileAttrs;
    Ppu_fetchTileAttributes(p, &tileAttrs);
    p->registers.lowBitShift = tileAttrs.low;
    p->registers.highBitShift = tileAttrs.high;
    uint8_t attr = tileAttrs.attr;

    Ppu_fetchTileAttributes(p, &tileAttrs);
    // Get second tile, move the pixels into the right side of
    // shift registers
    p->registers.lowBitShift = (p->registers.lowBitShift << 8) | tileAttrs.low;
    p->registers.highBitShift = (p->registers.highBitShift << 8) | tileAttrs.high;
    // Current tile to render is attrBuf
    uint8_t attrBuf = tileAttrs.attr;

    for (int x = 0; x < 32; x++) {
        int palette = 0;

        for (unsigned int b = 0; b < 8; b++) {
            int intB = b;
            int fbRow = p->scanline*256 + ((x * 8) + intB);

            unsigned int uintFineX = p->registers.fineX;
            uint16_t pixel = (p->registers.lowBitShift >> (15 - b - uintFineX)) & 0x01;
            pixel += ((p->registers.highBitShift >> (15 - b - uintFineX) & 0x01) << 1);

            // If we're grabbing the pixel from the high
            // part of the shift register, use the buffered
            // palette, not the current one
            if ((15 - b - uintFineX) < 8) {
                palette = Ppu_bgPaletteEntry(p, attrBuf, pixel);
            } else {
                palette = Ppu_bgPaletteEntry(p, attr, pixel);
            }

            if (p->palettebuffer[fbRow].value != 0) {
                // Pixel is already rendered and priority
                // 1 means show behind background
                continue;
            }

            p->palettebuffer[fbRow].color = PPU_PALETTE_RGB[palette%64];
            p->palettebuffer[fbRow].value = pixel;
            p->palettebuffer[fbRow].pindex = -1;
        }

        // xcoord = p->registers.vramAddress & 0x1F
        attr = attrBuf;

        // Shift the first tile out, bring the new tile in
        Ppu_fetchTileAttributes(p, &tileAttrs);
        p->registers.lowBitShift = (p->registers.lowBitShift << 8) | tileAttrs.low;
        p->registers.highBitShift = (p->registers.highBitShift << 8) | tileAttrs.high;
        attrBuf = tileAttrs.attr;
    }
}

void Ppu_evaluateScanlineSprites(Ppu* p, int line) {
    int spriteCount = 0;

    for (int i = 0; i < 256; ++i) {
        uint8_t y = p->spriteData.yCoordinates[i];
        int spriteHeight = 8;
        if ((p->flags.spriteSize&0x1) == 0x1) {
            spriteHeight = 16;
        }

        int intY = y;
        if (intY > (line-1)-spriteHeight && intY+(spriteHeight-1) < (line-1)+spriteHeight) {
            uint8_t attrValue = p->spriteData.attributes[i] & 0x3;
            uint8_t t = p->spriteData.tiles[i];

            int c = (line - 1) - intY;

            int ycoord;
            bool yflip = ((p->spriteData.attributes[i]>>7)&0x1) == 0x1;
            int intYCoord = p->spriteData.yCoordinates[i];
            if (yflip) {
                ycoord = intYCoord + ((spriteHeight - 1) - c);
            } else {
                ycoord = intYCoord + c + 1;
            }

            uint8_t entry[4];
            Ppu_sprPaletteEntry(p, attrValue, entry);
            int s = Ppu_sprPatternTableAddress(p, t);
            bool sprite0 = i == 0;
            int t0Index;
            int t1Index;
            if ((p->flags.spriteSize&0x01) != 0x0) {
                // 8x16 Sprite
                int topStartIndex = s;
                int bottomStartIndex = s + 16;
                int tileStartIndex;
                if (c > 7 && yflip) {
                    tileStartIndex = topStartIndex;
                    ycoord += 8;
                } else if (c < 8 && yflip) {
                    tileStartIndex = bottomStartIndex;
                    ycoord -= 8;
                } else if (c > 7) {
                    tileStartIndex = bottomStartIndex;
                } else {
                    tileStartIndex = topStartIndex;
                }

                t0Index = tileStartIndex + c % 8;
                t1Index = tileStartIndex + (c % 8) + 8;
            } else {
                // 8x8 Sprite
                t0Index = s + c;
                t1Index = s + c + 8;
            }
            Ppu_decodePatternTile(p, p->vram[t0Index], p->vram[t1Index],
                p->spriteData.xCoordinates[i],
                ycoord,
                entry,
                &p->spriteData.attributes[i], sprite0, i);

            spriteCount++;

            if (spriteCount == 9) {
                if (p->spriteLimitEnabled) {
                    Ppu_setStatus(p, STATUS_SPRITE_OVERFLOW);
                    break;
                }
            }
        }
    }
}

void Ppu_decodePatternTile(Ppu* p, uint8_t t0, uint8_t t1, int x, int y,
        uint8_t* pal, uint8_t* attr, bool spZero, int index)
{
    for (unsigned int b = 0; b < 8; ++b) {
        int xcoord = 0;
        int intB = b;
        if (((*attr>>6)&0x1) != 0) {
            xcoord = x + intB;
        } else {
            int reversedB = 7 - b;
            xcoord = x + reversedB;
        }

        // Don't wrap around if we're past the edge of the
        // screen
        if (xcoord > 255) {
            continue;
        }

        int fbRow = y*256 + xcoord;

        // Store the bit 0/1
        uint8_t pixel = (t0 >> b) & 0x01;
        pixel += ((t1 >> b & 0x01) << 1);

        bool trans = false;
        if (attr != NULL && pixel == 0) {
            trans = true;
        }

        // Set the color of the pixel in the buffer
        if (fbRow < 0xF000 && !trans) {
            uint8_t priority = (*attr >> 5) & 0x1;

            bool hit = ((p->registers.status&0x40) == 0x40);
            if (p->palettebuffer[fbRow].value != 0 && spZero && !hit) {
                // Since we render background first, if we're placing an opaque
                // pixel here and the existing pixel is opaque, we've hit
                // Sprite 0 
                Ppu_setStatus(p, STATUS_SPRITE0HIT);
            }

            if (p->palettebuffer[fbRow].pindex > -1 && p->palettebuffer[fbRow].pindex < index) {
                // Pixel with a higher sprite priority (lower index)
                // is already here, so don't render this pixel
                continue;
            } else if (p->palettebuffer[fbRow].value != 0 && priority == 1) {
                // Pixel is already rendered and priority
                // 1 means show behind background
                // unless background pixel is not transparent
                continue;
            }

            int intPalPixel = pal[pixel];
            p->palettebuffer[fbRow].color = PPU_PALETTE_RGB[intPalPixel%64];
            p->palettebuffer[fbRow].value = pixel;
            p->palettebuffer[fbRow].pindex = index;
        }
    }
}

int Ppu_bgPaletteEntry(Ppu* p, uint8_t a, uint16_t pix) {
    if (pix == 0x0) {
        return p->paletteRam[0x00];
    }

    switch (a) {
    case 0x0:
        return p->paletteRam[0x00+pix];
    case 0x4:
        return p->paletteRam[0x04+pix];
    case 0x8:
        return p->paletteRam[0x08+pix];
    case 0xC:
        return p->paletteRam[0x0C+pix];
    }

    return 0;
}


void Ppu_sprPaletteEntry(Ppu* p, unsigned int a, uint8_t* dest) {
    switch (a) {
    case 0x0:
        dest[0] = p->paletteRam[0x10];
        dest[1] = p->paletteRam[0x11];
        dest[2] = p->paletteRam[0x12];
        dest[3] = p->paletteRam[0x13];
        break;
    case 0x1:
        dest[0] = p->paletteRam[0x10];
        dest[1] = p->paletteRam[0x15];
        dest[2] = p->paletteRam[0x16];
        dest[3] = p->paletteRam[0x17];
        break;
    case 0x2:
        dest[0] = p->paletteRam[0x10];
        dest[1] = p->paletteRam[0x19];
        dest[2] = p->paletteRam[0x1A];
        dest[3] = p->paletteRam[0x1B];
        break;
    case 0x3:
        dest[0] = p->paletteRam[0x10];
        dest[1] = p->paletteRam[0x1D];
        dest[2] = p->paletteRam[0x1E];
        dest[3] = p->paletteRam[0x1F];
        break;
    }
}



// // write the chr rom into dest
// void rom_read_chr(uint8_t* dest){};

// // starts executing the PRG ROM.
// // this function returns when the RTI instruction is executed,
// // or the program exits.
// // when an interrupt occurs, call rom_start with the interrupt
// // index.
// void rom_start(uint8_t interrupt){};
// // controller
// void rom_set_button_state(uint8_t padIndex, uint8_t buttonIndex, uint8_t value){};

// // RAM
// uint8_t rom_ram_read(uint16_t addr){};
