/*
 * kaleidoscope::plugin::FirmwareVersion -- Tell the firmware version via Focus
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

#include "Raise2FirmwareVersion.h"
#include "Communications.h"
#include "Kaleidoscope-FocusSerial.h"
#include "Kaleidoscope.h"
#include "nrf_log.h"
#include <cstdio>
#include "Ble_manager.h"


#ifndef RAISE2_FIRMWARE_VERSION
    #define RAISE2_FIRMWARE_VERSION "v0.0.0"
#endif

#define RP2040_ID_END_PACKAGE 28
#define RP2040_ID_START_PACKAGE 12

# define base16char(i) ("0123456789ABCDEF"[i])


namespace kaleidoscope
{
namespace plugin
{
uint16_t FirmwareVersion::settings_base_ = 0;
FirmwareVersion::Specifications specifications_right_side;
FirmwareVersion::Specifications specifications_left_side;
char FirmwareVersion::device_name[16] = {0};
bool FirmwareVersion::hardware_info_requested = false;
bool conf_set = false;

struct request_t {
    bool layout{false};
    bool wireless{false};
    bool chip_id_left{false};
    bool chip_id_right{false};
    bool device_name{false};
    bool chip_id_left_rf{false};
    bool chip_id_right_rf{false};

    void reset()
    {
        layout = false;
        wireless = false;
        chip_id_left = false;
        chip_id_right = false;
        device_name = false;
        chip_id_left_rf = false;
        chip_id_right_rf = false;
    }
} request;


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

EventHandlerResult FirmwareVersion::onSetup()
{

    settings_base_ = kaleidoscope::plugin::EEPROMSettings::requestSlice((sizeof(specifications_left_side)*2)); //multiply by 2
                                                                                                                  // because we have two specification structures.
    Communications.callbacks.bind(CONFIGURATION, (
                                                     [this](Packet const &packet)
                                                     {
                                                         NRF_LOG_DEBUG("Configuration command receive");
                                                         if (filterHand(packet.header.device, false))
                                                         {
                                                             if (are_specifications_diferences(packet ,true)){
                                                                 NRF_LOG_DEBUG("saving specifications LEFT side due to differences");
                                                                 NRF_LOG_DEBUG("configuration right_side: %i", specifications_right_side.configuration);
                                                                 configuration_receive.left = false;
                                                             }

                                                             specifications_left_side.configuration = packet.data[0];
                                                             NRF_LOG_DEBUG("configuration left_side: %i", specifications_left_side.configuration);

                                                             specifications_left_side.device_name = packet.data[1];
                                                             NRF_LOG_DEBUG("device_name left_side: %i",specifications_left_side.device_name);

                                                             specifications_left_side.connection = packet.data[2];
                                                             NRF_LOG_DEBUG("connection left_side: %i", specifications_left_side.connection);

                                                             specifications_left_side.rf_gateway_chip_id = rebuild_64Bit_rf_gateway_id(packet);
                                                             //NRF_LOG_DEBUG("rf_gateway_chip_id left_side: %lu",  specifications_left_side.rf_gateway_chip_id);
                                                             for (uint8_t i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE ; ++i) {
                                                                 specifications_left_side.chip_id_rp2040[i - RP2040_ID_START_PACKAGE] = static_cast<char>(packet.data[i]);
                                                                 //NRF_LOG_DEBUG("chip_id left_side: %c", specifications_left_side.chip_id_rp2040[(i - RP2040_ID_START_PACKAGE)]);
                                                             }

                                                             //Save the configuration in memory just one time.
                                                             if (!configuration_receive.left  ){
                                                                 NRF_LOG_DEBUG("saving specifications left side");
                                                                 Runtime.storage().put(settings_base_, specifications_left_side);
                                                                 Runtime.storage().commit();
                                                                 configuration_receive.left = true;
                                                                 _BleManager.set_bt_name_from_specifications(get_specification(&specifications_left_side));
                                                                 Runtime.device().side.reset_sides();
                                                             }
                                                         }
                                                         if (filterHand(packet.header.device, true))
                                                         {
                                                             if (are_specifications_diferences(packet ,false)){
                                                                 NRF_LOG_DEBUG("saving specifications right side due to differences");
                                                                 NRF_LOG_DEBUG("configuration right_side: %i", specifications_right_side.configuration);
                                                                 configuration_receive.right = false;
                                                             }

                                                             specifications_right_side.configuration = packet.data[0];
                                                             NRF_LOG_DEBUG("configuration right_side: %i", specifications_right_side.configuration);

                                                             specifications_right_side.device_name = packet.data[1];
                                                             NRF_LOG_DEBUG("device_name right_side: %i",specifications_right_side.device_name);

                                                             specifications_right_side.connection = packet.data[2];
                                                             NRF_LOG_DEBUG("conection right_side: %i", specifications_right_side.connection);

                                                             specifications_right_side.rf_gateway_chip_id = rebuild_64Bit_rf_gateway_id(packet);
                                                            // NRF_LOG_DEBUG("rf_gateway_chip_id right_side: %lu",  specifications_right_side.rf_gateway_chip_id);

                                                             for (uint8_t i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE ; ++i) {
                                                                 specifications_right_side.chip_id_rp2040[i - RP2040_ID_START_PACKAGE] = static_cast<char>(packet.data[i]);
                                                                // NRF_LOG_DEBUG("chip_id right_side: %c", specifications_right_side.chip_id_rp2040[(i - RP2040_ID_START_PACKAGE)]);
                                                             }

                                                             //Save the configuration in memory just once.
                                                             if (!configuration_receive.right ){
                                                                 NRF_LOG_DEBUG("saving specifications right side");
                                                                 Runtime.storage().put(settings_base_ + sizeof(specifications_left_side), specifications_right_side);
                                                                 Runtime.storage().commit();
                                                                 configuration_receive.right = true;
                                                                 _BleManager.set_bt_name_from_specifications(get_specification(&specifications_right_side));
                                                                 Runtime.device().side.reset_sides();
                                                                 //reset_mcu();

                                                             }
                                                         }

                                                     }));

    Runtime.storage().get(settings_base_, specifications_left_side);

    Runtime.storage().get(settings_base_ + sizeof (specifications_left_side), specifications_right_side);

    /*Left side*/
    if (specifications_left_side.configuration == 0xFF || specifications_left_side.configuration == 0 )
    {
        configuration_receive.left = false;
/*        specifications_left_side.reset();
        Runtime.storage().put(settings_base_, specifications_left_side);
        Runtime.storage().commit();*/
    }
    else if (specifications_left_side.configuration != 0)
    {
        configuration_receive.left = true;
    }
    Runtime.storage().get(settings_base_, specifications_left_side);

    /*Right side*/
    if (specifications_right_side.configuration == 0xFF || specifications_right_side.configuration == 0 )
    {
        configuration_receive.right = false;
        //Wait to the configuration to be set afterward.
/*        specifications_right_side.reset();
        Runtime.storage().put(settings_base_ + sizeof(specifications_left_side), specifications_right_side);
        Runtime.storage().commit();*/
    }
    else if (specifications_right_side.configuration != 0)
    {
        configuration_receive.right = true;
    }
    Runtime.storage().get(settings_base_ + sizeof (specifications_left_side), specifications_right_side);

    NRF_LOG_DEBUG("Getting configurations right %i", specifications_right_side.configuration);
    NRF_LOG_DEBUG("Getting configurations left %i", specifications_left_side.configuration);

    /*Depending on which specification side we receive, we set the BT name.
     * It's not necessary to get the two sides to set the BT with one side is sufficient. */
    if (configuration_receive.left && !conf_set){
        _BleManager.set_bt_name_from_specifications(get_specification(&specifications_left_side));
        conf_set = true;
    }
    else if (configuration_receive.right && !conf_set){
        _BleManager.set_bt_name_from_specifications(get_specification(&specifications_right_side));
    }
    else {
        const char *device_name = "Dygma";
        _BleManager.set_bt_name_from_specifications(device_name);
    }

    return EventHandlerResult::OK;
}

EventHandlerResult FirmwareVersion::onFocusEvent(const char *command)
{
    const char *cmd = "version"
                      "\nhardware.layout"
                      "\nhardware.wireless"
                      "\nhardware.device_name"
                      "\nhardware.chip_id.left"
                      "\nhardware.chip_id.left_rf"
                      "\nhardware.chip_id.right"
                      "\nhardware.chip_id.right_rf";

    if (::Focus.handleHelp(command, cmd)) return EventHandlerResult::OK;

    if (strcmp(command, "version") != 0 && strncmp(command, "hardware.", 9) != 0 )   return EventHandlerResult::OK;

    /*********************** FW VERSION ***********************/
    if (strcmp(command, "version") == 0)
    {
        NRF_LOG_DEBUG("read request: version");

        char cstr[70];
        strcpy(cstr, RAISE2_FIRMWARE_VERSION);
        ::Focus.sendRaw<char *>(cstr);
        return EventHandlerResult::OK;
    }

    /*********************** COMMON SPECS ***********************/
    if (strcmp(command + 9, "layout") == 0)
    {
        if (::Focus.isEOL())
        {
            request.layout = true;
            hardware_info_requested = true;
            NRF_LOG_DEBUG("read request: sides.layout");
        }
        return EventHandlerResult::EVENT_CONSUMED;
    }

    if (strcmp(command + 9, "wireless") == 0)
    {
        if (::Focus.isEOL())
        {
            request.wireless = true;
            hardware_info_requested = true;
            NRF_LOG_DEBUG("read request: sides.info.wireless");
        }
        return EventHandlerResult::EVENT_CONSUMED;
    }

    if (strcmp(command + 9, "device_name") == 0)
    {
        if (::Focus.isEOL())
        {
            request.device_name = true;
            hardware_info_requested = true;
            NRF_LOG_DEBUG("read request: sides.info.device_name.left");
        }
        return EventHandlerResult::EVENT_CONSUMED;
    }

    /*********************** LEFT SIDE ***********************/
    if (strcmp(command + 9, "chip_id.left") == 0)
    {
        if (::Focus.isEOL())
        {
            request.chip_id_left = true;
            hardware_info_requested = true;
            NRF_LOG_DEBUG("read request: sides.info.chip_id.left");
        }
        return EventHandlerResult::EVENT_CONSUMED;
    }

    if (strcmp(command + 9, "chip_id.left_rf") == 0)
    {
        if (::Focus.isEOL())
        {
            request.chip_id_left_rf = true;
            hardware_info_requested = true;
            NRF_LOG_DEBUG("read request: hardware.chip_id.right_rf");
        }
        return EventHandlerResult::EVENT_CONSUMED;
    }

    /*********************** RIGHT SIDE ***********************/
    if (strcmp(command + 9, "chip_id.right") == 0)
    {
        if (::Focus.isEOL())
        {
            request.chip_id_right = true;
            hardware_info_requested = true;
            NRF_LOG_DEBUG("read request: sides.info.chip_id.right");
            return EventHandlerResult::EVENT_CONSUMED;
        }
    }

    if (strcmp(command + 9, "chip_id.right_rf") == 0)
    {
        if (::Focus.isEOL())
        {
            request.chip_id_right_rf = true;
            hardware_info_requested = true;
            NRF_LOG_DEBUG("read request: hardware.chip_id.right_rf");
        }
        return EventHandlerResult::EVENT_CONSUMED;
    }

    return EventHandlerResult::OK;
}

EventHandlerResult FirmwareVersion::beforeEachCycle()
{

    //Check if the configuration is set to send the specifications.
    if (configuration_receive.left && configuration_receive.right)
    {
        hardware_info_requested = false;
        check_and_send_specifications();
    }
    else
    {
       // NRF_LOG_DEBUG("Waiting for the configuration to be set");

        //if the configuration is not received in 3 seconds, we send an error message.
        if (!hardware_info_requested)
        {
            configuration_timeout_ = Runtime.millisAtCycleStart();
        }
        else if (Runtime.hasTimeExpired(configuration_timeout_, 3000))
        {
            NRF_LOG_DEBUG("Configuration not received");

            ::Focus.sendRaw("NOT_RECEIVED");

            request.reset();

            hardware_info_requested = false;
        }
    }

    return EventHandlerResult::OK;
}

uint64_t FirmwareVersion::rebuild_64Bit_rf_gateway_id(const Packet &packet)
{
    uint8_t bytes[8];
    for (uint8_t i = 0; i < 8; ++i) {
        bytes[i] = packet.data[i + 3]; //Start with index 3 to avoid the first three packages.
        NRF_LOG_DEBUG("Bytes %i", bytes[i]);
    }
    uint64_t rf_gateway_chip_id_received = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        rf_gateway_chip_id_received |= static_cast<uint64_t>(bytes[i]) << (8 * i);
    }
    return rf_gateway_chip_id_received;
}

const char *FirmwareVersion::get_specification(const Specifications* specifications)
{

    const char *config_prefix = (static_cast<Device>(specifications->configuration) == Device::ANSI) ? "-A" :
                                (static_cast<Device>(specifications->configuration) == Device::ISO) ? "-I" : "-A";
                                //(static_cast<Device>(specifications_left_side.configuration) == Device::NONE) ? "" : "";

    const char *connection_type = (static_cast<Device>(specifications->connection) == Device::Wired) ? "Wired" : "Wless";

    snprintf(FirmwareVersion::device_name, sizeof(FirmwareVersion::device_name), "Raise2-%s%s", connection_type, config_prefix);
//TODO: uncomment this block of code when new HW arrives.
/*    if (static_cast<Device>(specifications->configuration) == Device::NONE){
        snprintf(FirmwareVersion::device_name, sizeof(FirmwareVersion::device_name), "Raise-%s", connection_type);
    }*/

    return device_name;
}

bool FirmwareVersion::are_specifications_diferences( Communications_protocol::Packet  const &packet_check , bool side)
{
    uint8_t configuration = packet_check.data[0];
    uint8_t connection = packet_check.data[2];
    bool chip_id_diferences = false;

    FirmwareVersion::Specifications mem_spec{};

    if (side){
        mem_spec = specifications_left_side;
    } else{
        mem_spec =  specifications_right_side;
    }

    for (uint8_t i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE ; ++i) {
        if (mem_spec.chip_id_rp2040[i - RP2040_ID_START_PACKAGE] == static_cast<char>(packet_check.data[i])){
            continue;
        } else{
            chip_id_diferences = true;
        }
    }
    if (chip_id_diferences){
        NRF_LOG_DEBUG("Chip id is not the same" );
    }

    NRF_LOG_DEBUG(" connection Memory: %i , Receive: %i ", mem_spec.connection, connection );
    NRF_LOG_DEBUG(" configuration Memory: %i , Receive: %i ", mem_spec.configuration, configuration);

    if (   connection != mem_spec.connection
        || configuration != mem_spec.configuration || chip_id_diferences){
        NRF_LOG_DEBUG("Specifications are different from stored in memory");
        return true;
    }
    NRF_LOG_DEBUG("Same specifications");
    return false;
}

FirmwareVersion::Device FirmwareVersion::get_layout()
{
    return static_cast<FirmwareVersion::Device> (specifications_left_side.configuration);
}

void FirmwareVersion::check_and_send_specifications() const
{
    if (request.layout)
    {
        request.layout = false;
        String layout;
        NRF_LOG_DEBUG("read request: sides.layout");
        if (configuration_receive.left && static_cast<Device> (specifications_left_side.configuration) == Device::ISO){
            layout = "ISO";
        } else if (configuration_receive.right && static_cast<Device> (specifications_left_side.configuration) == Device::ISO) {
            layout = "ISO";
        } else {
            layout = "ANSI";
        }
        ::Focus.sendRaw(layout);
    }

    if (request.device_name)
    {
        request.device_name = false;
        String hardware_name = "";
        if (configuration_receive.left){
            if (static_cast<Device>(specifications_left_side.device_name) == Device::Raise2){
                hardware_name = "Raise2";
            } else {
                hardware_name = "Unknown";
            }
        } else if (configuration_receive.right){
            if (static_cast<Device>(specifications_right_side.device_name) == Device::Raise2){
                hardware_name = "Raise2";
            } else {
                hardware_name = "Unknown";
            }
        }
        ::Focus.sendRaw(hardware_name);
    }

    if (request.chip_id_left)
    {
        request.chip_id_left = false;
        String cstr = "";
        for (int i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE; ++i) {
            if (isprint(specifications_left_side.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ])) {
                cstr += specifications_left_side.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ];
            }
        }
        ::Focus.sendRaw(cstr);
    }

    if (request.chip_id_right)
    {
        request.chip_id_right = false;
        String cstrs = "";
        for (int i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE; ++i) {
            if (isprint(specifications_right_side.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ])) {
                cstrs += specifications_right_side.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ];
            }
        }
        ::Focus.sendRaw(cstrs);
    }

    if (request.chip_id_left_rf)
    {
        request.chip_id_left_rf = false;
        char buffer[21] = {'0'};
        uint64_t chip_id = specifications_left_side.rf_gateway_chip_id;
        snprintf(buffer, sizeof(buffer), "%8lx%8lx", static_cast<uint32_t>(chip_id >> 32), static_cast<uint32_t>(chip_id & 0xFFFFFFFF));
        ::Focus.sendRaw(buffer);
    }

    if (request.chip_id_right_rf)
    {
        request.chip_id_right_rf = false;
        char buffer[21] = {'0'};
        uint64_t chip_id = specifications_right_side.rf_gateway_chip_id;
        snprintf(buffer, sizeof(buffer), "%8lx%8lx", static_cast<uint32_t>(chip_id >> 32), static_cast<uint32_t>(chip_id & 0xFFFFFFFF));
        ::Focus.sendRaw(buffer);
    }

    if (request.wireless)
    {
        request.wireless = false;
        bool resp;
        if (static_cast<Device>(specifications_left_side.connection) == Device::Wireless
            && static_cast<Device>(specifications_right_side.connection) == Device::Wireless)
        {
            resp = true;
        }
        else
        {
            resp = false;
        }
        ::Focus.send(resp);
    }

}

} // namespace plugin
} // namespace kaleidoscope

kaleidoscope::plugin::FirmwareVersion FirmwareVersion;
