/*
 * Copyright (C) 2020  Dygma Lab S.L.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * The purpose of this module is for defining the HID descriptors and functions for
 * getting the USB and BLE HID descriptor versions in hidDefy module
 */

#include "hidDefy.h"
#include "FirmwareVersion.h"
#include <string.h>

const uint8_t hid_report_descriptor_ansi[] = HID_DEFY_REPORT_DESCRIPTOR( RAW_USAGE_ANSI );
const uint8_t hid_report_descriptor_iso[] = HID_DEFY_REPORT_DESCRIPTOR( RAW_USAGE_ISO );

void hid_report_descriptor_get( const uint8_t ** pp_desc, uint32_t * p_desc_len )
{
    kaleidoscope::plugin::FirmwareVersion::Device device = kaleidoscope::plugin::FirmwareVersion::get_layout();

    if( device == kaleidoscope::plugin::FirmwareVersion::Device::ISO )
    {
        *pp_desc = &hid_report_descriptor_iso[0];
        *p_desc_len = sizeof( hid_report_descriptor_iso );
    }
    else
    {
        *pp_desc = &hid_report_descriptor_ansi[0];
        *p_desc_len = sizeof( hid_report_descriptor_ansi );
    }
}

/* ── USB descriptor: same reports as BLE but Raw HID uses 63-byte reports ────
 *
 * USB Full Speed interrupt endpoints are limited to 64 bytes per packet. With
 * the 1-byte report ID prepended, the maximum data payload is 63 bytes. The
 * Raw HID section (200 bytes) is always the last item appended by
 * HID_DEFY_REPORT_DESCRIPTOR, so we swap it for a 63-byte version instead of
 * removing it entirely. Removing it made report ID 5 unavailable over USB,
 * so any Raw HID report sent while on USB was dropped (e.g. the OverlayKey
 * plugin that feeds Dygma Lens).
 *
 * TUD_HID_REPORT_DESC_GENERIC_INOUT_DYGMA(200,...) and (63,...) produce the
 * same number of descriptor bytes (the REPORT_COUNT value fits in 1 byte for
 * both 63 and 200), which is what makes the in-place swap possible. It is
 * verified at compile time by the static_asserts below.
 */

#define USB_RAW_HID_REPORT_SIZE 63  /* 63 data bytes + 1 report ID = 64-byte USB packet */

/* Probe: measures the exact byte-size of the Raw HID tail in the descriptor. */
static const uint8_t rawhid_tail_probe[] = {
    TUD_HID_REPORT_DESC_GENERIC_INOUT_DYGMA( OUTPUT_REPORT_LEN_RAW, RAW_USAGE_ANSI,
                                             HID_REPORT_ID( REPORT_ID_RAW ) )
};

/* Replacements: 63-byte Raw HID section, one per layout, so the vendor usage
 * keeps matching the descriptor it is spliced into. */
static const uint8_t usb_rawhid_tail_ansi[] = {
    TUD_HID_REPORT_DESC_GENERIC_INOUT_DYGMA( USB_RAW_HID_REPORT_SIZE, RAW_USAGE_ANSI,
                                             HID_REPORT_ID( REPORT_ID_RAW ) )
};

static const uint8_t usb_rawhid_tail_iso[] = {
    TUD_HID_REPORT_DESC_GENERIC_INOUT_DYGMA( USB_RAW_HID_REPORT_SIZE, RAW_USAGE_ISO,
                                             HID_REPORT_ID( REPORT_ID_RAW ) )
};

/* Compile-time guards: the swap requires every section to have equal byte count. */
static_assert( sizeof( rawhid_tail_probe ) == sizeof( usb_rawhid_tail_ansi ),
    "BLE and USB Raw HID descriptor sections must have equal byte count for the tail-swap" );
static_assert( sizeof( usb_rawhid_tail_ansi ) == sizeof( usb_rawhid_tail_iso ),
    "ANSI and ISO Raw HID descriptor sections must have equal byte count" );
static_assert( sizeof( hid_report_descriptor_ansi ) == sizeof( hid_report_descriptor_iso ),
    "ANSI and ISO report descriptors must have equal byte count" );

static uint8_t usb_desc_buf[sizeof( hid_report_descriptor_ansi )];
static bool usb_desc_ready = false;

static void build_usb_descriptor( void )
{
    if( usb_desc_ready )
    {
        return;
    }

    kaleidoscope::plugin::FirmwareVersion::Device device = kaleidoscope::plugin::FirmwareVersion::get_layout();
    const uint8_t * p_usb_tail = ( device == kaleidoscope::plugin::FirmwareVersion::Device::ISO )
                                     ? &usb_rawhid_tail_iso[0]
                                     : &usb_rawhid_tail_ansi[0];

    const uint8_t * p_base_desc;
    uint32_t base_desc_len;
    hid_report_descriptor_get( &p_base_desc, &base_desc_len );

    const size_t base_len = base_desc_len - sizeof( rawhid_tail_probe );
    memcpy( usb_desc_buf, p_base_desc, base_len );
    memcpy( usb_desc_buf + base_len, p_usb_tail, sizeof( usb_rawhid_tail_ansi ) );

    usb_desc_ready = true;
}

void hid_report_descriptor_usb_get( const uint8_t ** pp_desc, uint32_t * p_desc_len )
{
    build_usb_descriptor();

    *pp_desc = &usb_desc_buf[0];
    *p_desc_len = sizeof( usb_desc_buf );
}

void hid_report_descriptor_ble_get( const uint8_t ** pp_desc, uint32_t * p_desc_len )
{
    /* Get the valid descriptor */
    hid_report_descriptor_get( pp_desc, p_desc_len );
}
