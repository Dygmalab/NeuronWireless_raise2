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

#pragma once

#include "Kaleidoscope.h"

namespace kaleidoscope
{
namespace plugin
{

class FirmwareVersion : public Plugin
{
  public:
    uint32_t configuration_timeout_ = 0;
    static char device_name[16];

    enum class Device {
        Defy,
        Raise2,
        ISO,
        ANSI,
        NONE,
        Wireless,
        Wired
    };

    struct Specifications {
        uint8_t device_name;
        uint8_t configuration;
        uint8_t connection;
        uint64_t rf_gateway_chip_id;
        char chip_id_rp2040[20];

        void reset(void)
        {
            configuration = static_cast<uint8_t>(Device::ANSI);
            device_name = static_cast<uint8_t>(Device::Raise2);
            connection = static_cast<uint8_t>(Device::Wired);
            rf_gateway_chip_id = 0;
            for (int i = 0; i < 20 ; ++i)
            {
                chip_id_rp2040[i] = '0';
            }
        }
    };

    EventHandlerResult onFocusEvent(const char *command);
    EventHandlerResult beforeEachCycle();
    EventHandlerResult onSetup();
    uint8_t get_device_name();
    String get_left_side_chip_id();
    uint64_t get_left_side_rf_chip_id();
    String get_right_side_chip_id();
    uint64_t get_right_side_rf_chip_id();

    static Device get_layout();
    static const char* get_specification(const Specifications* specifications);

  private:
    static uint16_t settings_base_;
    struct ConfigurationReceive {
        bool left;
        bool right;
    };
    ConfigurationReceive configuration_receive{false,false};
    static bool hardware_info_requested;

    static uint64_t rebuild_64Bit_rf_gateway_id(Communications_protocol::Packet const &packet);

    /**
    * @brief Checks the differences in specifications between the provided specification and the one stored in memory.
    * This function compares the provided specifications with the specifications stored in memory.
    * If the specifications differ, it returns true, otherwise it returns false.
    * @param spec The specification to compare.
    * @param side Indicates the side from which to retrieve the specifications stored in memory. true for the left side, false for the right side.
    * @return true if there are differences in the specifications, false otherwise.
     */
    static bool are_specifications_diferences(Communications_protocol::Packet const &packet_check , bool side);
    void check_and_send_specifications() const;
};

}   // namespace plugin
}   // namespace kaleidoscope

extern kaleidoscope::plugin::FirmwareVersion FirmwareVersion;
