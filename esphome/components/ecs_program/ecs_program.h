#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/udp/udp_component.h"
#include "esphome/core/component.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace esphome::ecs_program {

class ECSProgram : public Component {
 public:
  void set_udp(udp::UDPComponent *udp) { this->udp_ = udp; }
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_climate(climate::Climate *climate) { this->climate_ = climate; }
  void set_timezone(const char *timezone) {
    this->expected_timezone_.fill('\0');
    strncpy(this->expected_timezone_.data(), timezone, this->expected_timezone_.size() - 1);
  }

  void setup() override;
  void dump_config() override;

  // Called by the shared ESPHome UDP listener. The packet is validated in
  // memory before it can replace the persisted active schedule.
  void process_packet(std::span<const uint8_t> packet);

  const char *active_hash() const { return this->active_hash_.data(); }
  uint32_t active_revision() const { return this->schedule_.revision; }
  const char *active_profile() const { return this->active_profile_.data(); }
  bool has_schedule() const { return this->schedule_.valid; }

 protected:
  static constexpr uint32_t PACKET_MAGIC = 0x31534345;  // "ECS1"
  static constexpr uint8_t PACKET_VERSION = 1;
  static constexpr uint8_t PACKET_KIND_SCHEDULE = 1;
  // Two preference slots must fit in the 256-byte ESP8266 preferences
  // sector. The wire packet remains self-describing; this is the bounded
  // persistent representation used after decoding it.
  static constexpr uint8_t MAX_STATES = 4;
  static constexpr uint8_t MAX_TRANSITIONS = 8;
  static constexpr size_t HASH_SIZE = 32;
  static constexpr size_t HASH_HEX_SIZE = HASH_SIZE * 2;
  static constexpr size_t TIMEZONE_SIZE = 64;
  static constexpr uint32_t PREFERENCE_KEY_A = 0x45435350;
  static constexpr uint32_t PREFERENCE_KEY_B = 0x45435351;

  struct State {
    uint8_t mode{0};
    int16_t heat_deci{0};
    int16_t cool_deci{0};
    uint8_t fan{0};
    uint8_t preset{0};
  } __attribute__((packed));

  struct Transition {
    uint8_t weekday_mask{0};
    uint16_t minute{0};
    uint8_t state{0};
  } __attribute__((packed));

  // Fixed-size storage keeps preference writes bounded and avoids dynamic
  // allocation on the 1 MB ESP8266 target.
  struct StoredSchedule {
    uint32_t magic{0};
    uint8_t version{0};
    uint8_t valid{0};
    uint32_t revision{0};
    uint8_t fallback_state{0};
    uint8_t state_count{0};
    uint8_t transition_count{0};
    uint8_t hash[HASH_SIZE]{};
    State states[MAX_STATES]{};
    Transition transitions[MAX_TRANSITIONS]{};
    uint32_t checksum{0};
  } __attribute__((packed));

  static_assert(sizeof(StoredSchedule) == 109, "compact schedule layout changed");
  static_assert(((sizeof(StoredSchedule) + 3) / 4 + 1) * 2 <= 64,
                "two compact schedule preference slots must fit in ESP8266 storage");

  void evaluate_();
  bool decode_packet_(std::span<const uint8_t> packet, StoredSchedule &candidate);
  bool valid_schedule_(const StoredSchedule &schedule) const;
  bool load_schedule_();
  void apply_state_(const State &state);
  int current_state_(const ESPTime &now) const;
  void publish_diagnostics_();
  void format_hash_();
  static uint32_t crc32_(const uint8_t *data, size_t size);
  static int hex_value_(uint8_t value);
  static const char *mode_name_(uint8_t mode);
  static const char *fan_name_(uint8_t fan);
  static const char *preset_name_(uint8_t preset);

  udp::UDPComponent *udp_{nullptr};
  time::RealTimeClock *time_{nullptr};
  climate::Climate *climate_{nullptr};
  ESPPreferenceObject preference_a_{};
  ESPPreferenceObject preference_b_{};
  StoredSchedule schedule_{};
  int8_t active_slot_{-1};
  int active_state_{-1};
  std::array<char, HASH_HEX_SIZE + 1> active_hash_{};
  std::array<char, 32> active_profile_{};
  std::array<char, 64> expected_timezone_{};
};

}  // namespace esphome::ecs_program
