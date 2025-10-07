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

#include "MIDIUSB.h"
#include <USB.h>

MIDI_ MidiUSB;

bool MIDI_::setCableName(uint8_t cable_id, const char *str) {
    if (!_running || cable_id == 0 || cable_id > sizeof(_cableStrID)) {
        return false;
    }

    _cableStrID[cable_id - 1] = USB.registerString(str);
    return _cableStrID[cable_id - 1] > 0;
}

void MIDI_::interfaceCB(int itf, uint8_t *dst, int len) {
    uint8_t desc0[] = {
        TUD_MIDI_DESC_HEAD((uint8_t)itf, _strID, _cables),
    };
    memcpy(dst, desc0, sizeof(desc0));
    dst += sizeof(desc0);

    for (uint8_t i = 0; i < _cables; i++) {
        uint8_t desc1[] = {
            TUD_MIDI_DESC_JACK_DESC(i + 1, _cableStrID[i])
        };
        memcpy(dst, desc1, sizeof(desc1));
        dst += sizeof(desc1);
    }

    uint8_t desc2[] = {
        TUD_MIDI_DESC_EP(_epOut, 64, _cables)
    };
    memcpy(dst, desc2, sizeof(desc2));
    dst += sizeof(desc2);

    for (uint8_t i = 0; i < _cables; i++) {
        uint8_t desc3[] = {
            TUD_MIDI_JACKID_IN_EMB(i + 1)
        };
        memcpy(dst, desc3, sizeof(desc3));
        dst += sizeof(desc3);
    }

    uint8_t desc4[] = {
        TUD_MIDI_DESC_EP(_epIn, 64, _cables)
    };
    memcpy(dst, desc4, sizeof(desc4));
    dst += sizeof(desc4);

    for (uint8_t i = 0; i < _cables; i++) {
        uint8_t desc5[] = {
            TUD_MIDI_JACKID_OUT_EMB(i + 1)
        };
        memcpy(dst, desc5, sizeof(desc5));
        dst += sizeof(desc5);
    }
}

bool MIDI_::setName(const char *name) {
    if (!_running) {
        _strID = USB.registerString(name);
        return  true;
    }
    return false;
}

bool MIDI_::begin() {
    if (_running) {
        return false;
    }
    USB.disconnect();

    _epIn = USB.registerEndpointIn();
    _epOut = USB.registerEndpointOut();
    _strID = !_strID ? USB.registerString("PicoMIDI") : _strID;
    if (!_cableStrID[0]) {
        _cableStrID[0] = USB.registerString("virtual-cable");
    }

    _id = USB.registerInterface(2, _cb, (void *)this, TUD_MIDI_DESC_HEAD_LEN + TUD_MIDI_DESC_JACK_LEN * _cables + 2 * TUD_MIDI_DESC_EP_LEN(_cables), 3, 1 << 16);

    USB.connect();

    _running = true;

    return true;
}

void MIDI_::end() {
    if (_running) {
        USB.disconnect();
        USB.unregisterInterface(_id);
        USB.unregisterEndpointIn(_epIn);
        USB.unregisterEndpointOut(_epOut);
        _running = false;
        USB.connect();
    }
}

MIDI_::operator bool() {
    if (!_running) {
        begin();
    }
    return connected();
}

bool MIDI_::connected() {
    if (!_running) {
        begin();
    }
    return _running ? tud_midi_mounted() : false;
}

//int MIDI_::read(void) {
midi_read_t::operator int() {
    if (!midi->_running) {
        midi->begin();
    }
    uint8_t ch;
    if (!midi->_running || !midi->connected()) {
        return -1;
    }
    return tud_midi_stream_read(&ch, 1) ? (int)ch : (-1);
}

size_t MIDI_::write(uint8_t b) {
    if (!_running) {
        begin();
    }
    return _running && connected() ? tud_midi_stream_write(0, &b, 1) : false;
}

int MIDI_::available() {
    if (!_running) {
        begin();
    }
    return _running && connected() ? tud_midi_available() : 0;
}

int MIDI_::peek(void) {
    return -1;
}

void MIDI_::flush(void) {
    // NOOP
}

bool MIDI_::writePacket(const uint8_t packet[4]) {
    if (!_running) {
        begin();
    }
    return _running && connected() ? tud_midi_packet_write(packet) : false;
}

bool MIDI_::readPacket(uint8_t packet[4]) {
    if (!_running) {
        begin();
    }
    return _running && connected() ? tud_midi_packet_read(packet) : false;
}

//midiEventPacket_t MIDI_::read() {
midi_read_t::operator midiEventPacket_t() {
    if (!midi->_running) {
        midi->begin();
    }
    uint8_t packet[4];
    if (midi->_running && midi->connected() && midi->readPacket(packet)) {
        midiEventPacket_t out;
        out.header = packet[0];
        out.byte1 = packet[1];
        out.byte2 = packet[2];
        out.byte3 = packet[3];
        return out;
    }
    midiEventPacket_t bad = { 0, 0, 0, 0};
    return bad;
}

size_t MIDI_::write(const uint8_t *buffer, size_t size) {
    if (!_running) {
        begin();
    }
    return _running && connected() ? tud_midi_stream_write(1, buffer, size) : 0;
}

void MIDI_::sendMIDI(midiEventPacket_t event) {
    if (!_running) {
        begin();
    }
    uint8_t data[4];
    data[0] = event.header;
    data[1] = event.byte1;
    data[2] = event.byte2;
    data[3] = event.byte3;
    write(data, 4);
}
