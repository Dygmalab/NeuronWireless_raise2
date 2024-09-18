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


#ifndef RAISE2_FW_VERSION
#error "Firmware package version is not specified."
    #define RAISE2_FW_VERSION "N/A"
#endif

#define RP2040_ID_END_PACKAGE 28
#define RP2040_ID_START_PACKAGE 12

# define base16char(i) ("0123456789ABCDEF"[i])

namespace kaleidoscope
{
    namespace plugin
    {

        uint16_t FirmwareVersion::settings_base_ = 0;

//At the end of the function we need to know if the specifications are different from the ones stored in memory.
//Also we need to act differently depending if the configuration stored in memory is empty or not.
// If it's empty we store the data and don't reset the neuron. If it's not empty we store the data and we DON'T reset the neuron.
        bool left_side_spec_changes = false;
        bool right_side_spec_changes = false;

        FirmwareVersion::Specifications specifications_right_side;
        FirmwareVersion::Specifications specifications_left_side;

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
                            if (are_specifications_diferences(packet ,true))
                            {
                                NRF_LOG_DEBUG("saving specifications LEFT side due to differences");
                                configuration.configuration_receive_left = false;
                            }

                            specifications_left_side.configuration = packet.data[0];
                            NRF_LOG_DEBUG("configuration left_side: %i", specifications_left_side.configuration);

                            specifications_left_side.device_name = packet.data[1];
                            NRF_LOG_DEBUG("device_name left_side: %i",specifications_left_side.device_name);

                            specifications_left_side.connection = packet.data[2];
                            NRF_LOG_DEBUG("connection left_side: %i", specifications_left_side.connection);

                            specifications_left_side.rf_gateway_chip_id = rebuild_64Bit_rf_gateway_id(packet);
                            //NRF_LOG_DEBUG("rf_gateway_chip_id left_side: %lu",  specifications_left_side.rf_gateway_chip_id);

                            for (uint8_t i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE ; ++i)
                            {
                                specifications_left_side.chip_id_rp2040[i - RP2040_ID_START_PACKAGE] = static_cast<char>(packet.data[i]);
                                //NRF_LOG_DEBUG("chip_id left_side: %c", specifications_left_side.chip_id_rp2040[(i - RP2040_ID_START_PACKAGE)]);
                            }

                            //Save the configuration in memory just one time.
                            if ( !configuration.configuration_receive_left )
                            {
                                NRF_LOG_DEBUG("saving specifications left side");
                                Runtime.storage().put(settings_base_, specifications_left_side);
                                Runtime.storage().commit();
                                configuration.configuration_receive_left = true;
                                _BleManager.set_bt_name_from_specifications(get_specification(&specifications_left_side));
                                // Runtime.device().side.reset_sides();
                                left_side_spec_changes = true;
                            }
                        }
                        if (filterHand(packet.header.device, true))
                        {
                            if (are_specifications_diferences(packet ,false))
                            {
                                NRF_LOG_DEBUG("saving specifications right side due to differences");
                                NRF_LOG_DEBUG("configuration right_side: %i", specifications_right_side.configuration);
                                configuration.configuration_receive_right = false;
                            }

                            specifications_right_side.configuration = packet.data[0];
                            NRF_LOG_DEBUG("configuration right_side: %i", specifications_right_side.configuration);

                            specifications_right_side.device_name = packet.data[1];
                            NRF_LOG_DEBUG("device_name right_side: %i",specifications_right_side.device_name);

                            specifications_right_side.connection = packet.data[2];
                            NRF_LOG_DEBUG("conection right_side: %i", specifications_right_side.connection);

                            specifications_right_side.rf_gateway_chip_id = rebuild_64Bit_rf_gateway_id(packet);
                            // NRF_LOG_DEBUG("rf_gateway_chip_id right_side: %lu",  specifications_right_side.rf_gateway_chip_id);

                            for (uint8_t i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE ; ++i)
                            {
                                specifications_right_side.chip_id_rp2040[i - RP2040_ID_START_PACKAGE] = static_cast<char>(packet.data[i]);
                                // NRF_LOG_DEBUG("chip_id right_side: %c", specifications_right_side.chip_id_rp2040[(i - RP2040_ID_START_PACKAGE)]);
                            }

                            //Save the configuration in memory just once.
                            if (!configuration.configuration_receive_right )
                            {
                                NRF_LOG_DEBUG("saving specifications right side");
                                Runtime.storage().put(settings_base_ + sizeof(specifications_left_side), specifications_right_side);
                                Runtime.storage().commit();
                                configuration.configuration_receive_right = true;
                                _BleManager.set_bt_name_from_specifications(get_specification(&specifications_right_side));
                                //Runtime.device().side.reset_sides();
                                right_side_spec_changes = true;
                            }
                        }

                        if (right_side_spec_changes || left_side_spec_changes)
                        {
                            if(check_specifications_in_memory() == false)
                            {
                                NRF_LOG_DEBUG("One side has been set, reseting the MCU");
                                reset_mcu();
                            }
                        }

                    }));

            Runtime.storage().get(settings_base_, specifications_left_side);

            Runtime.storage().get(settings_base_ + sizeof (specifications_left_side), specifications_right_side);

            /*Left side*/
            if (specifications_left_side.configuration == 0xFF || specifications_left_side.configuration == 0 )
            {
                configuration.configuration_receive_left = false;
                configuration.configuration_left_empty = true;
            }
            else if (specifications_left_side.configuration != 0)
            {
                configuration.configuration_receive_left = true;
                configuration.configuration_left_empty  = false;
            }

            /*Right side*/
            if (specifications_right_side.configuration == 0xFF || specifications_right_side.configuration == 0 )
            {
                configuration.configuration_receive_right = false;
                configuration.configuration_right_empty = true;
            }
            else if (specifications_right_side.configuration != 0)
            {
                configuration.configuration_receive_right = true;
                configuration.configuration_right_empty = false;
            }

            NRF_LOG_DEBUG("Getting configurations right %i", specifications_right_side.configuration);
            NRF_LOG_DEBUG("Getting configurations left %i", specifications_left_side.configuration);

            /*Depending on which specification side we receive, we set the BT name.
             * It's not necessary to get the two sides to set the BT with one side is sufficient. */
            if (configuration.configuration_receive_left&& !conf_set){
                _BleManager.set_bt_name_from_specifications(get_specification(&specifications_left_side));
                conf_set = true;
            }
            else if (configuration.configuration_receive_right && !conf_set){
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
                strcpy(cstr, RAISE2_FW_VERSION);
                ::Focus.sendRaw<char *>(cstr);
                return EventHandlerResult::OK;
            }

            /*********************** COMMON SPECS ***********************/
            if (strcmp(command + 9, "layout") == 0)
            {
                if (::Focus.isEOL())
                {
                    NRF_LOG_DEBUG("read request: sides.layout");
                    check_and_send_specifications(LAYOUT);
                }
                return EventHandlerResult::EVENT_CONSUMED;
            }

            if (strcmp(command + 9, "wireless") == 0)
            {
                if (::Focus.isEOL())
                {
                    NRF_LOG_DEBUG("read request: sides.info.wireless");
                    check_and_send_specifications(CONNECTION_TYPE);
                }
                return EventHandlerResult::EVENT_CONSUMED;
            }

            if (strcmp(command + 9, "device_name") == 0)
            {
                if (::Focus.isEOL())
                {
                    NRF_LOG_DEBUG("read request: sides.info.device_name.left");
                    check_and_send_specifications(HARDWARE_NAME);
                }
                return EventHandlerResult::EVENT_CONSUMED;
            }

            /*********************** LEFT SIDE ***********************/
            if (strcmp(command + 9, "chip_id.left") == 0)
            {
                if (::Focus.isEOL())
                {
                    NRF_LOG_DEBUG("read request: sides.info.chip_id.left");
                    check_and_send_specifications(CHIP_ID_LEFT);
                }
                return EventHandlerResult::EVENT_CONSUMED;
            }

            if (strcmp(command + 9, "chip_id.left_rf") == 0)
            {
                if (::Focus.isEOL())
                {
                    NRF_LOG_DEBUG("read request: hardware.chip_id.right_rf");
                    check_and_send_specifications(CHIP_ID_LEFT_RF);
                }
                return EventHandlerResult::EVENT_CONSUMED;
            }

            /*********************** RIGHT SIDE ***********************/
            if (strcmp(command + 9, "chip_id.right") == 0)
            {
                if (::Focus.isEOL())
                {
                    NRF_LOG_DEBUG("read request: sides.info.chip_id.right");
                    check_and_send_specifications(CHIP_ID_RIGHT);
                    return EventHandlerResult::EVENT_CONSUMED;
                }
            }

            if (strcmp(command + 9, "chip_id.right_rf") == 0)
            {
                if (::Focus.isEOL())
                {
                    NRF_LOG_DEBUG("read request: hardware.chip_id.right_rf");
                    check_and_send_specifications(CHIP_ID_RIGHT_RF);
                }
                return EventHandlerResult::EVENT_CONSUMED;
            }

            return EventHandlerResult::OK;
        }

        EventHandlerResult FirmwareVersion::beforeEachCycle()
        {
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
        snprintf(FirmwareVersion::device_name, sizeof(FirmwareVersion::device_name), "Defy-%s", connection_type);
    }*/

            return device_name;
        }

        bool FirmwareVersion::are_specifications_diferences( Communications_protocol::Packet  const &packet_check , bool side)
        {
            uint8_t configuration = packet_check.data[0];
            uint8_t connection = packet_check.data[2];
            bool chip_id_diferences = false;

            FirmwareVersion::Specifications mem_spec{};

            if (side)
            {
                mem_spec = specifications_left_side;
            } else
            {
                mem_spec =  specifications_right_side;
            }

            for (uint8_t i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE ; ++i) {
                if (mem_spec.chip_id_rp2040[i - RP2040_ID_START_PACKAGE] == static_cast<char>(packet_check.data[i]))
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

        __attribute__((unused)) FirmwareVersion::Device FirmwareVersion::get_layout()
        {
            return static_cast<FirmwareVersion::Device> (specifications_left_side.configuration);
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
            String layout;
            NRF_LOG_DEBUG("read request: sides.layout");
            if (configuration.configuration_receive_left&& static_cast<Device> (specifications_left_side.configuration) == Device::ISO){
                layout = "ISO";
            }
            else if (configuration.configuration_receive_right && static_cast<Device> (specifications_left_side.configuration) == Device::ISO)
            {
                layout = "ISO";
            }
            else
            {
                layout = "ANSI";
            }
            ::Focus.sendRaw(layout);
        }

        void FirmwareVersion::send_device_name()
        {
            String hardware_name = "";
            if (configuration.configuration_receive_left)
            {
                if (static_cast<Device>(specifications_left_side.device_name) == Device::Raise2)
                {
                    hardware_name = "Raise2";
                }
                else if (static_cast<Device>(specifications_left_side.device_name) == Device::Defy)
                {
                    hardware_name = "Defy";
                }
            }
            else if (configuration.configuration_receive_right)
            {
                if (static_cast<Device>(specifications_right_side.device_name) == Device::Raise2)
                {
                    hardware_name = "Raise2";
                }
                else if (static_cast<Device>(specifications_right_side.device_name) == Device::Defy)
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
                if (isprint(specifications_left_side.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ])) {
                    cstr += specifications_left_side.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ];
                }
            }
            ::Focus.sendRaw(cstr);
        }

        void FirmwareVersion::send_chip_id_right()
        {
            String cstrs = "";
            for (int i = RP2040_ID_START_PACKAGE; i < RP2040_ID_END_PACKAGE; ++i) {
                if (isprint(specifications_right_side.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ])) {
                    cstrs += specifications_right_side.chip_id_rp2040[ i-RP2040_ID_START_PACKAGE ];
                }
            }
            ::Focus.sendRaw(cstrs);
        }

        void FirmwareVersion::send_chip_id_left_rf()
        {
            char buffer[21] = {'0'};
            uint64_t chip_id = specifications_left_side.rf_gateway_chip_id;
            snprintf(buffer, sizeof(buffer), "%8lx%8lx", static_cast<uint32_t>(chip_id >> 32), static_cast<uint32_t>(chip_id & 0xFFFFFFFF));
            ::Focus.sendRaw(buffer);
        }

        void FirmwareVersion::send_chip_id_right_rf()
        {
            char buffer[21] = {'0'};
            uint64_t chip_id = specifications_right_side.rf_gateway_chip_id;
            snprintf(buffer, sizeof(buffer), "%8lx%8lx", static_cast<uint32_t>(chip_id >> 32), static_cast<uint32_t>(chip_id & 0xFFFFFFFF));
            ::Focus.sendRaw(buffer);
        }

        void FirmwareVersion::send_connection_type()
        {
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

        bool FirmwareVersion::check_specifications_in_memory()
        {
            bool result = false;

            Runtime.storage().get(settings_base_, specifications_left_side);

            Runtime.storage().get(settings_base_ + sizeof (specifications_left_side), specifications_right_side);

            /*Left side*/
            if (specifications_left_side.configuration == 0xFF || specifications_left_side.configuration == 0 )
            {
                result =   true;
            }
            else if (specifications_left_side.configuration != 0)
            {
                result =  false;
            }

            /*Right side*/
            if (specifications_right_side.configuration == 0xFF || specifications_right_side.configuration == 0 )
            {
                result = true;
            }
            else if (specifications_right_side.configuration != 0)
            {
                result = false;
            }
            return result;
        }


    } // namespace plugin
} // namespace kaleidoscope

kaleidoscope::plugin::FirmwareVersion FirmwareVersion;
