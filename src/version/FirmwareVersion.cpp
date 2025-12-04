/*
 * FirmwareVersion -- Provide the version and system hardware configuration
 *
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

#include "FirmwareVersion.h"
#include "Config_manager.h"
#include "Communications.h"
#include "Kaleidoscope-FocusSerial.h"
#include "Kaleidoscope.h"
//#include "nrf_log.h"
#include "Ble_manager.h"

#include "kbd_if_manager.h"

#ifndef RAISE2_FW_VERSION
#error "Firmware package version is not specified."
    #define RAISE2_FW_VERSION "N/A"
#endif

#define RP2040_ID_END_PACKAGE 28
#define RP2040_ID_START_PACKAGE 12

# define base16char(i) ("0123456789ABCDEF"[i])

    const FirmwareVersion::device_spec_t * FirmwareVersion::p_device_spec = nullptr;

//At the end of the function we need to know if the specifications are different from the ones stored in memory.
//Also we need to act differently depending if the configuration stored in memory is empty or not.
// If it's empty we store the data and don't reset the neuron. If it's not empty we store the data and we DON'T reset the neuron.
    bool left_side_spec_changes = false;
    bool right_side_spec_changes = false;

    char FirmwareVersion::device_name[16] = {0};
    bool conf_set = false;

    struct Configuration{
        bool configuration_receive_left;
        bool configuration_receive_right;
        bool configuration_left_empty;
        bool configuration_right_empty;
    };
    Configuration configuration;

bool inline filterHand(Communications_protocol::Devices incomingDevice, bool right_or_left)
{
    if (right_or_left == 1)
    {
        return incomingDevice == Communications_protocol::KEYSCANNER_DEFY_RIGHT || incomingDevice == Communications_protocol::BLE_DEFY_RIGHT ||
               incomingDevice == Communications_protocol::RF_DEFY_RIGHT;
    }
    else
    {
        return incomingDevice == Communications_protocol::KEYSCANNER_DEFY_LEFT || incomingDevice == Communications_protocol::BLE_DEFY_LEFT ||
               incomingDevice == Communications_protocol::RF_DEFY_LEFT;
    }
}


result_t FirmwareVersion::init()
{
    result_t result = RESULT_ERR;

    result = kbdif_initialize();
    EXIT_IF_ERR( result, "kbdif_initialize failed" );

    result = ConfigManager.config_item_request( ConfigManager::CFG_ITEM_TYPE_DEVICE_SPEC, (const void **)&p_device_spec );
    EXIT_IF_ERR( result, "ConfigManager.config_item_request failed" );

    // because we have two specification structures.
    Communications.callbacks.bind(CONFIGURATION, (
            [this](Packet const &packet)
            {
                //NRF_LOG_DEBUG("Configuration command receive");
                if (filterHand(packet.header.device, false))
                {
                    keyscanner_spec_t keyscanner_spec = p_device_spec->ks_left;

                    if (are_specifications_diferences(packet ,true))
                    {
                        //NRF_LOG_DEBUG("saving specifications LEFT side due to differences");
                        configuration.configuration_receive_left = false;
                    }

                    keyscanner_spec.configuration = packet.data[0];
                    //NRF_LOG_DEBUG("configuration left_side: %i", keyscanner_spec.configuration);

                    keyscanner_spec.device_name = packet.data[1];
                    //NRF_LOG_DEBUG("device_name left_side: %i",keyscanner_spec.device_name);

                    keyscanner_spec.connection = packet.data[2];
                    //NRF_LOG_DEBUG("connection left_side: %i", keyscanner_spec.connection);

                    keyscanner_spec.rf_gateway_chip_id = rebuild_64Bit_rf_gateway_id(packet);
                    //NRF_LOG_DEBUG("rf_gateway_chip_id left_side: %lu",  keyscanner_spec.rf_gateway_chip_id);

                    for (uint8_t i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE ; ++i)
                    {
                        keyscanner_spec.chip_id_rp2040[i - RP2040_ID_START_PACKAGE] = static_cast<char>(packet.data[i]);
                        //NRF_LOG_DEBUG("chip_id left_side: %c", keyscanner_spec.chip_id_rp2040[(i - RP2040_ID_START_PACKAGE)]);
                    }

                    //Save the configuration in memory just one time.
                    if ( !configuration.configuration_receive_left )
                    {
                        //NRF_LOG_DEBUG("saving specifications left side");
                        cfgmem_keyscanner_spec_left_save( &keyscanner_spec );
                        configuration.configuration_receive_left = true;
                        BleManager.set_bt_name_from_specifications(get_specification(&keyscanner_spec));
                        left_side_spec_changes = true;
                    }
                }
                if (filterHand(packet.header.device, true))
                {
                    keyscanner_spec_t keyscanner_spec = p_device_spec->ks_right;

                    if (are_specifications_diferences(packet ,false))
                    {
                        //NRF_LOG_DEBUG("saving specifications right side due to differences");
                        //NRF_LOG_DEBUG("configuration right_side: %i", keyscanner_spec.configuration);
                        configuration.configuration_receive_right = false;
                    }

                    keyscanner_spec.configuration = packet.data[0];
                    //NRF_LOG_DEBUG("configuration right_side: %i", keyscanner_spec.configuration);

                    keyscanner_spec.device_name = packet.data[1];
                    //NRF_LOG_DEBUG("device_name right_side: %i",keyscanner_spec.device_name);

                    keyscanner_spec.connection = packet.data[2];
                    //NRF_LOG_DEBUG("conection right_side: %i", keyscanner_spec.connection);

                    keyscanner_spec.rf_gateway_chip_id = rebuild_64Bit_rf_gateway_id(packet);
                    // NRF_LOG_DEBUG("rf_gateway_chip_id right_side: %lu",  keyscanner_spec.rf_gateway_chip_id);

                    for (uint8_t i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE ; ++i)
                    {
                        keyscanner_spec.chip_id_rp2040[i - RP2040_ID_START_PACKAGE] = static_cast<char>(packet.data[i]);
                        // NRF_LOG_DEBUG("chip_id right_side: %c", keyscanner_spec.chip_id_rp2040[(i - RP2040_ID_START_PACKAGE)]);
                    }

                    //Save the configuration in memory just once.
                    if (!configuration.configuration_receive_right )
                    {
                        //NRF_LOG_DEBUG("saving specifications right side");
                        cfgmem_keyscanner_spec_right_save( &keyscanner_spec );
                        configuration.configuration_receive_right = true;
                        BleManager.set_bt_name_from_specifications(get_specification(&keyscanner_spec));
                        right_side_spec_changes = true;
                    }
                }
            }));

    /*Left side*/
    if (p_device_spec->ks_left.configuration == 0xFF || p_device_spec->ks_left.configuration == 0 )
    {
        configuration.configuration_receive_left = false;
        configuration.configuration_left_empty = true;
    }
    else if (p_device_spec->ks_left.configuration != 0)
    {
        configuration.configuration_receive_left = true;
        configuration.configuration_left_empty  = false;
    }

    /*Right side*/
    if (p_device_spec->ks_right.configuration == 0xFF || p_device_spec->ks_right.configuration == 0 )
    {
        configuration.configuration_receive_right = false;
        configuration.configuration_right_empty = true;
    }
    else if (p_device_spec->ks_right.configuration != 0)
    {
        configuration.configuration_receive_right = true;
        configuration.configuration_right_empty = false;
    }

    //NRF_LOG_DEBUG("Getting configurations right %i", p_device_spec->ks_right.configuration);
    //NRF_LOG_DEBUG("Getting configurations left %i", p_device_spec->ks_left.configuration);

    /*Depending on which specification side we receive, we set the BT name.
     * It's not necessary to get the two sides to set the BT with one side is sufficient. */
    if (configuration.configuration_receive_left&& !conf_set){
        BleManager.set_bt_name_from_specifications(get_specification(&p_device_spec->ks_left));
        conf_set = true;
    }
    else if (configuration.configuration_receive_right && !conf_set){
        BleManager.set_bt_name_from_specifications(get_specification(&p_device_spec->ks_right));
    }
    else {
        const char *device_name = "Dygma";
        BleManager.set_bt_name_from_specifications(device_name);
    }

_EXIT:
    return result;
}

bool FirmwareVersion::keyboard_is_wireless()
{
    bool resp = false;

    if(!configuration.configuration_receive_right || !configuration.configuration_receive_left)
    {
        //NRF_LOG_DEBUG("Configuration not received");
        return false;
    }

    if (static_cast<Device>(p_device_spec->ks_left.connection) == Device::Wireless
        && static_cast<Device>(p_device_spec->ks_right.connection) == Device::Wireless)
    {
        resp = true;
    }
    else
    {
        resp = false;
    }
    return resp;
}

uint64_t FirmwareVersion::rebuild_64Bit_rf_gateway_id(const Packet &packet)
{
    uint8_t bytes[8];
    for (uint8_t i = 0; i < 8; ++i) {
        bytes[i] = packet.data[i + 3]; //Start with index 3 to avoid the first three packages.
        //NRF_LOG_DEBUG("Bytes %i", bytes[i]);
    }
    uint64_t rf_gateway_chip_id_received = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        rf_gateway_chip_id_received |= static_cast<uint64_t>(bytes[i]) << (8 * i);
    }
    return rf_gateway_chip_id_received;
}

const char *FirmwareVersion::get_specification(const keyscanner_spec_t * p_keyscanner_spec)
{

    const char *config_prefix = (static_cast<Device>(p_keyscanner_spec->configuration) == Device::ANSI) ? "-A" :
                                (static_cast<Device>(p_keyscanner_spec->configuration) == Device::ISO) ? "-I" : "-A";

    const char *connection_type = (static_cast<Device>(p_keyscanner_spec->connection) == Device::Wired) ? "Wired" : "Wless";

    snprintf(FirmwareVersion::device_name, sizeof(FirmwareVersion::device_name), "Raise2-%s%s", connection_type, config_prefix);

    return device_name;
}

bool FirmwareVersion::are_specifications_diferences( Communications_protocol::Packet  const &packet_check , bool side)
{
    uint8_t configuration = packet_check.data[0];
    uint8_t connection = packet_check.data[2];
    bool chip_id_diferences = false;

    const keyscanner_spec_t * p_keyscanner_spec = nullptr;

    if (side)
    {
        p_keyscanner_spec = &p_device_spec->ks_left;
    } else
    {
        p_keyscanner_spec = &p_device_spec->ks_right;
    }

    for (uint8_t i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE ; ++i) {
        if (p_keyscanner_spec->chip_id_rp2040[i - RP2040_ID_START_PACKAGE] == static_cast<char>(packet_check.data[i]))
        {
            continue;
        }
        else
        {
            chip_id_diferences = true;
        }
    }
    if (chip_id_diferences)
    {
        NRF_LOG_DEBUG("Chip id is not the same" );
    }

    NRF_LOG_DEBUG(" connection Memory: %i , Receive: %i ", p_keyscanner_spec->connection, connection );
    NRF_LOG_DEBUG(" configuration Memory: %i , Receive: %i ", p_keyscanner_spec->configuration, configuration);

    if (   connection != p_keyscanner_spec->connection
           || configuration != p_keyscanner_spec->configuration || chip_id_diferences){
        NRF_LOG_DEBUG("Specifications are different from stored in memory");
        return true;
    }
    NRF_LOG_DEBUG("Same specifications");
    return false;
}

FirmwareVersion::Device FirmwareVersion::get_layout()
{
    FirmwareVersion::Device layout;

    if (configuration.configuration_receive_left&& static_cast<Device> (p_device_spec->ks_left.configuration) == Device::ISO){
        layout = Device::ISO;
    }
    else if (configuration.configuration_receive_right && static_cast<Device> (p_device_spec->ks_right.configuration) == Device::ISO)
    {
        layout = Device::ISO;
    }
    else
    {
        layout = Device::ANSI;
    }

    return layout;
}

void FirmwareVersion::check_and_send_specifications(request_t request)
{
    if (!configuration.configuration_receive_left || !configuration.configuration_receive_right)
    {
        ::Focus.sendRaw("undefined");
        return;
    }
    switch (request)
    {
        case LAYOUT:
            send_layout();
            break;

        case HARDWARE_NAME:
            send_device_name();
            break;

        case CHIP_ID_LEFT:
            send_chip_id_left();
            break;

        case CHIP_ID_RIGHT:
            send_chip_id_right();
            break;

        case CHIP_ID_LEFT_RF:
            send_chip_id_left_rf();
            break;

        case CHIP_ID_RIGHT_RF:
            send_chip_id_right_rf();
            break;

        case CONNECTION_TYPE:
            send_connection_type();
            break;
    }
}

void FirmwareVersion::send_layout()
{
    Device layout;
    String layout_str;
    NRF_LOG_DEBUG("read request: sides.layout");

    layout = get_layout();

    if ( layout == Device::ISO){
        layout_str = "ISO";
    }
    else /* ( layout == Device::ANSI) */
    {
        layout_str = "ANSI";
    }
    ::Focus.sendRaw(layout_str);
}

void FirmwareVersion::send_device_name()
{
    String hardware_name = "";
    if (configuration.configuration_receive_left)
    {
        if (static_cast<Device>(p_device_spec->ks_left.device_name) == Device::Raise2)
        {
            hardware_name = "Raise2";
        }
        else if (static_cast<Device>(p_device_spec->ks_left.device_name) == Device::Defy)
        {
            hardware_name = "Defy";
        }
    }
    else if (configuration.configuration_receive_right)
    {
        if (static_cast<Device>(p_device_spec->ks_right.device_name) == Device::Raise2)
        {
            hardware_name = "Raise2";
        }
        else if (static_cast<Device>(p_device_spec->ks_right.device_name) == Device::Defy)
        {
            hardware_name = "Defy";
        }
    }
    ::Focus.sendRaw(hardware_name);
}

void FirmwareVersion::send_chip_id_left()
{
    String cstr = "";
    for (int i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE; ++i) {
        if (isprint(p_device_spec->ks_left.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ])) {
            cstr += p_device_spec->ks_left.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ];
        }
    }
    ::Focus.sendRaw(cstr);
}

void FirmwareVersion::send_chip_id_right()
{
    String cstrs = "";
    for (int i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE; ++i) {
        if (isprint(p_device_spec->ks_right.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ])) {
            cstrs += p_device_spec->ks_right.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ];
        }
    }
    ::Focus.sendRaw(cstrs);
}

void FirmwareVersion::send_chip_id_left_rf()
{
    char buffer[21] = {'0'};
    uint64_t chip_id = p_device_spec->ks_left.rf_gateway_chip_id;
    snprintf(buffer, sizeof(buffer), "%8lx%8lx", static_cast<uint32_t>(chip_id >> 32), static_cast<uint32_t>(chip_id & 0xFFFFFFFF));
    ::Focus.sendRaw(buffer);
}

void FirmwareVersion::send_chip_id_right_rf()
{
    char buffer[21] = {'0'};
    uint64_t chip_id = p_device_spec->ks_right.rf_gateway_chip_id;
    snprintf(buffer, sizeof(buffer), "%8lx%8lx", static_cast<uint32_t>(chip_id >> 32), static_cast<uint32_t>(chip_id & 0xFFFFFFFF));
    ::Focus.sendRaw(buffer);
}

void FirmwareVersion::send_connection_type()
{
    bool resp;
    if (static_cast<Device>(p_device_spec->ks_left.connection) == Device::Wireless
        && static_cast<Device>(p_device_spec->ks_right.connection) == Device::Wireless)
    {
        resp = true;
    }
    else
    {
        resp = false;
    }
    ::Focus.send(resp);
}

bool FirmwareVersion::check_specifications_in_memory()
{
    bool result = false;

    /*Left side*/
    if (p_device_spec->ks_left.configuration == 0xFF || p_device_spec->ks_left.configuration == 0 )
    {
        result =   true;
    }
    else if (p_device_spec->ks_left.configuration != 0)
    {
        result =  false;
    }

    /*Right side*/
    if (p_device_spec->ks_right.configuration == 0xFF || p_device_spec->ks_right.configuration == 0 )
    {
        result = true;
    }
    else if (p_device_spec->ks_right.configuration != 0)
    {
        result = false;
    }
    return result;
}

result_t FirmwareVersion::kbdif_initialize()
{
    result_t result = RESULT_ERR;
    kbdif_conf_t config;

    /* Prepare the kbdif configuration */
    config.p_instance = this;
    config.handlers = &kbdif_handlers;

    /* Initialize the kbdif */
    result = kbdif_init( &p_kbdif, &config );
    EXIT_IF_ERR( result, "kbdif_init failed" );

    /* Add the kbdif into the kbdif manager */
    result = kbdifmgr_add( p_kbdif );
    EXIT_IF_ERR( result, "kbdifmgr_add failed" );

_EXIT:
    return result;
}

kbdapi_event_result_t FirmwareVersion::kbdif_command_event_cb( void * p_instance, const char * p_command )
{
    const char *cmd = "version"
                      "\nhardware.layout"
                      "\nhardware.wireless"
                      "\nhardware.device_name"
                      "\nhardware.chip_id.left"
                      "\nhardware.chip_id.left_rf"
                      "\nhardware.chip_id.right"
                      "\nhardware.chip_id.right_rf";

    if (::Focus.handleHelp(p_command, cmd)) return KBDAPI_EVENT_RESULT_IGNORED;

    if (strcmp(p_command, "version") != 0 && strncmp(p_command, "hardware.", 9) != 0 )   return KBDAPI_EVENT_RESULT_IGNORED;

    /*********************** FW VERSION ***********************/
    if (strcmp(p_command, "version") == 0)
    {
        NRF_LOG_DEBUG("read request: version");

        char cstr[70];
        strcpy(cstr, RAISE2_FW_VERSION);
        ::Focus.sendRaw<char *>(cstr);
        return KBDAPI_EVENT_RESULT_IGNORED;
    }

    /*********************** COMMON SPECS ***********************/
    if (strcmp(p_command + 9, "layout") == 0)
    {
        if (::Focus.isEOL())
        {
            NRF_LOG_DEBUG("read request: sides.layout");
            check_and_send_specifications(LAYOUT);
        }
        return KBDAPI_EVENT_RESULT_CONSUMED;
    }

    if (strcmp(p_command + 9, "wireless") == 0)
    {
        if (::Focus.isEOL())
        {
            NRF_LOG_DEBUG("read request: sides.info.wireless");
            check_and_send_specifications(CONNECTION_TYPE);
        }
        return KBDAPI_EVENT_RESULT_CONSUMED;
    }

    if (strcmp(p_command + 9, "device_name") == 0)
    {
        if (::Focus.isEOL())
        {
            NRF_LOG_DEBUG("read request: sides.info.device_name.left");
            check_and_send_specifications(HARDWARE_NAME);
        }
        return KBDAPI_EVENT_RESULT_CONSUMED;
    }

    /*********************** LEFT SIDE ***********************/
    if (strcmp(p_command + 9, "chip_id.left") == 0)
    {
        if (::Focus.isEOL())
        {
            NRF_LOG_DEBUG("read request: sides.info.chip_id.left");
            check_and_send_specifications(CHIP_ID_LEFT);
        }
        return KBDAPI_EVENT_RESULT_CONSUMED;
    }

    if (strcmp(p_command + 9, "chip_id.left_rf") == 0)
    {
        if (::Focus.isEOL())
        {
            NRF_LOG_DEBUG("read request: hardware.chip_id.right_rf");
            check_and_send_specifications(CHIP_ID_LEFT_RF);
        }
        return KBDAPI_EVENT_RESULT_CONSUMED;
    }

    /*********************** RIGHT SIDE ***********************/
    if (strcmp(p_command + 9, "chip_id.right") == 0)
    {
        if (::Focus.isEOL())
        {
            NRF_LOG_DEBUG("read request: sides.info.chip_id.right");
            check_and_send_specifications(CHIP_ID_RIGHT);
            return KBDAPI_EVENT_RESULT_CONSUMED;
        }
    }

    if (strcmp(p_command + 9, "chip_id.right_rf") == 0)
    {
        if (::Focus.isEOL())
        {
            NRF_LOG_DEBUG("read request: hardware.chip_id.right_rf");
            check_and_send_specifications(CHIP_ID_RIGHT_RF);
        }
        return KBDAPI_EVENT_RESULT_CONSUMED;
    }

    return KBDAPI_EVENT_RESULT_IGNORED;
}

const kbdif_handlers_t FirmwareVersion::kbdif_handlers =
{
    .key_event_cb = NULL,
    .command_event_cb = kbdif_command_event_cb,
};

void FirmwareVersion::cfgmem_keyscanner_spec_left_save( const keyscanner_spec_t * p_spec )
{
    result_t result = RESULT_ERR;

    result = ConfigManager.config_item_update( &p_device_spec->ks_left, p_spec, sizeof( p_device_spec->ks_left) );
    ASSERT_DYGMA( result == RESULT_OK, "ConfigManager.config_item_update failed" );

    UNUSED( result );
}

void FirmwareVersion::cfgmem_keyscanner_spec_right_save( const keyscanner_spec_t * p_spec )
{
    result_t result = RESULT_ERR;

    result = ConfigManager.config_item_update( &p_device_spec->ks_right, p_spec, sizeof( p_device_spec->ks_right) );
    ASSERT_DYGMA( result == RESULT_OK, "ConfigManager.config_item_update failed" );

    UNUSED( result );
}

class FirmwareVersion FirmwareVersion;
