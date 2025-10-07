/*
    MIDIUSB-compatible library for the Arduino-Pico core
    Implements the APIs of the Arduino.cc MIDIUSB library
    Extends to some of the Adafruit TinyUSB library functions

    Copyright (c) 2025 Earle F. Philhower, III <earlephilhower@yahoo.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
    Original API Copyright (c) 2015, Gary Grewal
    Permission to use, copy, modify, and/or distribute this software for
    any purpose with or without fee is hereby granted, provided that the
    above copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
    BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
    OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
    WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
    ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
    SOFTWARE.
*/

#pragma once

#include <stdint.h>
#include <Arduino.h>

// MIDIUSB-compat
typedef struct {
    uint8_t header;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
} midiEventPacket_t;

class MIDI_;

// Some C++ magic to make 2 return types for read()  Thanks(?) to https://artificial-mind.net/blog/2020/10/10/return-type-overloading
// Please forgive me, this is for compatibility and not by choice!
struct midi_read_t {
    MIDI_ *midi;
    operator int(); // Adafruit
    operator midiEventPacket_t(); // MIDIUSB
};

class MIDI_ {
public:
    MIDI_() {
    }

    // Adafruit-compat
    MIDI_(int cables) {
        _cables = cables;
    }

    void setCables(uint8_t cables) {
        if (!_running) {
            _cables = cables;
        }
    }
    bool setCableName(uint8_t cable_id, const char *str);

    // Set the MIDI device name, note that this string must be long-lived and is not copied out
    bool setName(const char *name);

    // USB interface callback and shim
    void interfaceCB(int itf, uint8_t *dst, int len);
    static void _cb(int itf, uint8_t *dst, int len, void *param) {
        ((MIDI_*)param)->interfaceCB(itf, dst, len);
    }

    bool begin();
    void end();
    operator bool();
    bool connected();
    // int read(); // read raw 1 byte from cable
    size_t write(uint8_t b); // write 1 byte to cable
    int available();
    int peek();
    void flush();

    midi_read_t read() {
        midi_read_t o;
        o.midi = this;
        return o;
    }

    // Adafruit-compat
    bool writePacket(const uint8_t packet[4]);
    bool readPacket(uint8_t packet[4]);

    // MIDIUSB-compat
    // midiEventPacket_t read();
    size_t write(const uint8_t *buffer, size_t size);
    void sendMIDI(midiEventPacket_t event);

protected:
    friend struct midi_read_t;
    bool _running = false;
    uint8_t _epIn;
    uint8_t _epOut;
    uint8_t _strID = 0;
    uint8_t _cables = 1;
    uint8_t _cableStrID[6] = {}; // init all to 0
    uint8_t _id;
};

extern MIDI_ MidiUSB;
