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

const uint8_t hid_report_descriptor_ansi[] = HID_DEFY_REPORT_DESCRIPTOR( RAW_USAGE_ANSI );
const uint8_t hid_report_descriptor_iso[] = HID_DEFY_REPORT_DESCRIPTOR( RAW_USAGE_ISO );

void hid_report_descriptor_get( const uint8_t ** pp_desc, uint32_t * p_desc_len )
{
    FirmwareVersion::Device device = FirmwareVersion::get_layout();

    if( device == FirmwareVersion::Device::ISO )
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

void hid_report_descriptor_usb_get( const uint8_t ** pp_desc, uint32_t * p_desc_len )
{
    /* Dummy generic inout part of the descriptor for determining its size */
    const uint8_t sizeRawHID[] = {TUD_HID_REPORT_DESC_GENERIC_INOUT(OUTPUT_REPORT_LEN_RAW, HID_REPORT_ID(RAW_USAGE_UNKNOWN))};

    /* Get the valid descriptor */
    hid_report_descriptor_get( pp_desc, p_desc_len );

    /* Modify the descriptor length to omit the BLE part */
    *p_desc_len -= sizeof( sizeRawHID );
}

void hid_report_descriptor_ble_get( const uint8_t ** pp_desc, uint32_t * p_desc_len )
{
    /* Get the valid descriptor */
    hid_report_descriptor_get( pp_desc, p_desc_len );
}
