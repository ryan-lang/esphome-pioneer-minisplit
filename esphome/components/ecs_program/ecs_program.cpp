#include "ecs_program.h"

#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace esphome::ecs_program {

static const char *const TAG = "ecs_program";

template<typename T> static bool read_le(std::span<const uint8_t> packet, size_t &pos, T &value) {
  if (pos + sizeof(T) > packet.size())
    return false;
  value = 0;
  for (size_t i = 0; i < sizeof(T); i++)
    value |= static_cast<T>(packet[pos++]) << (i * 8);
  return true;
}

void ECSProgram::setup() {
  this->preference_a_ = global_preferences->make_preference<StoredSchedule>(PREFERENCE_KEY_A, true);
  this->preference_b_ = global_preferences->make_preference<StoredSchedule>(PREFERENCE_KEY_B, true);
  if (this->udp_ != nullptr) {
    this->udp_->add_listener([this](std::span<const uint8_t> packet) { this->process_packet(packet); });
  }
  this->load_schedule_();
  this->set_interval("evaluate", 1000, [this]() { this->evaluate_(); });
  this->evaluate_();
}

void ECSProgram::dump_config() {
  ESP_LOGCONFIG(TAG, "ECS cached program:");
  ESP_LOGCONFIG(TAG, "  Stored schedule: %s", YESNO(this->schedule_.valid));
  ESP_LOGCONFIG(TAG, "  Revision: %lu", static_cast<unsigned long>(this->schedule_.revision));
}

bool ECSProgram::load_schedule_() {
  StoredSchedule a{}, b{};
  const bool valid_a = this->preference_a_.load(&a) && this->valid_schedule_(a) &&
                       crc32_(reinterpret_cast<const uint8_t *>(&a), offsetof(StoredSchedule, checksum)) == a.checksum;
  const bool valid_b = this->preference_b_.load(&b) && this->valid_schedule_(b) &&
                       crc32_(reinterpret_cast<const uint8_t *>(&b), offsetof(StoredSchedule, checksum)) == b.checksum;
  if (!valid_a && !valid_b) {
    this->schedule_ = StoredSchedule{};
    this->active_slot_ = -1;
    return false;
  }
  if (valid_a && (!valid_b || a.revision >= b.revision)) {
    this->schedule_ = a;
    this->active_slot_ = 0;
  } else {
    this->schedule_ = b;
    this->active_slot_ = 1;
  }
  this->format_hash_();
  return true;
}

bool ECSProgram::valid_schedule_(const StoredSchedule &schedule) const {
  if (schedule.magic != PACKET_MAGIC || schedule.version != PACKET_VERSION || schedule.valid == 0)
    return false;
  if (schedule.state_count == 0 || schedule.state_count > MAX_STATES || schedule.transition_count > MAX_TRANSITIONS)
    return false;
  if (schedule.fallback_state >= schedule.state_count)
    return false;
  bool has_hash = false;
  for (const auto byte : schedule.hash) {
    has_hash = has_hash || byte != 0;
  }
  if (!has_hash)
    return false;
  for (uint8_t i = 0; i < schedule.transition_count; i++) {
    const auto &transition = schedule.transitions[i];
    if (transition.weekday_mask == 0 || transition.minute >= 1440 || transition.state >= schedule.state_count)
      return false;
  }
  return true;
}

bool ECSProgram::decode_packet_(std::span<const uint8_t> packet, StoredSchedule &candidate) {
  size_t pos = 0;
  uint32_t magic;
  uint8_t version, kind;
  if (!read_le(packet, pos, magic) || !read_le(packet, pos, version) || !read_le(packet, pos, kind))
    return false;
  if (magic != PACKET_MAGIC || version != PACKET_VERSION || kind != PACKET_KIND_SCHEDULE)
    return false;

  uint32_t revision;
  uint8_t timezone_length, hash_length, fallback, state_count, transition_count, reserved;
  if (!read_le(packet, pos, revision) || !read_le(packet, pos, timezone_length))
    return false;
  if (timezone_length == 0 || timezone_length >= TIMEZONE_SIZE || pos + timezone_length > packet.size())
    return false;

  if (this->expected_timezone_[0] != '\0' &&
      (timezone_length != strlen(this->expected_timezone_.data()) ||
       memcmp(packet.data() + pos, this->expected_timezone_.data(), timezone_length) != 0))
    return false;

  candidate = StoredSchedule{};
  candidate.magic = magic;
  candidate.version = version;
  candidate.valid = 1;
  candidate.revision = revision;
  pos += timezone_length;
  if (!read_le(packet, pos, hash_length) || hash_length != HASH_HEX_SIZE || pos + hash_length > packet.size())
    return false;
  for (size_t i = 0; i < HASH_SIZE; i++) {
    const int high = hex_value_(packet[pos + i * 2]);
    const int low = hex_value_(packet[pos + i * 2 + 1]);
    if (high < 0 || low < 0)
      return false;
    candidate.hash[i] = static_cast<uint8_t>((high << 4) | low);
  }
  pos += hash_length;
  if (!read_le(packet, pos, fallback) || !read_le(packet, pos, state_count) ||
      !read_le(packet, pos, transition_count) || !read_le(packet, pos, reserved))
    return false;
  (void) reserved;
  if (state_count == 0 || state_count > MAX_STATES || transition_count > MAX_TRANSITIONS || fallback >= state_count)
    return false;
  candidate.fallback_state = fallback;
  candidate.state_count = state_count;
  candidate.transition_count = transition_count;

  for (uint8_t i = 0; i < state_count; i++) {
    auto &state = candidate.states[i];
    uint8_t mode, fan, preset;
    uint16_t heat_bits, cool_bits;
    if (!read_le(packet, pos, mode) || !read_le(packet, pos, heat_bits) ||
        !read_le(packet, pos, cool_bits) || !read_le(packet, pos, fan) ||
        !read_le(packet, pos, preset))
      return false;
    state.mode = mode;
    state.heat_deci = static_cast<int16_t>(heat_bits);
    state.cool_deci = static_cast<int16_t>(cool_bits);
    state.fan = fan;
    state.preset = preset;
    if (state.mode > 3 || state.fan > 3 || state.preset > 3)
      return false;
    if (state.heat_deci < 0 || state.heat_deci > 1000 || state.cool_deci < 0 || state.cool_deci > 1000 ||
        state.heat_deci > state.cool_deci)
      return false;
  }
  for (uint8_t i = 0; i < transition_count; i++) {
    auto &transition = candidate.transitions[i];
    uint8_t weekday_mask, state;
    uint16_t minute;
    if (!read_le(packet, pos, weekday_mask) || !read_le(packet, pos, minute) ||
        !read_le(packet, pos, state))
      return false;
    transition.weekday_mask = weekday_mask;
    transition.minute = minute;
    transition.state = state;
    if (transition.weekday_mask == 0 || transition.minute >= 1440 || transition.state >= state_count)
      return false;
  }
  uint32_t received_checksum;
  if (!read_le(packet, pos, received_checksum) || pos != packet.size())
    return false;
  const auto expected = crc32_(packet.data(), packet.size() - sizeof(uint32_t));
  if (expected != received_checksum)
    return false;
  candidate.checksum = 0;
  candidate.checksum = crc32_(reinterpret_cast<const uint8_t *>(&candidate), offsetof(StoredSchedule, checksum));
  return this->valid_schedule_(candidate);
}

void ECSProgram::process_packet(std::span<const uint8_t> packet) {
  StoredSchedule candidate{};
  if (!this->decode_packet_(packet, candidate))
    return;
  if (this->schedule_.valid && candidate.revision < this->schedule_.revision) {
    ESP_LOGW(TAG, "Ignoring older schedule revision %lu", static_cast<unsigned long>(candidate.revision));
    return;
  }
  const int8_t target_slot = this->active_slot_ == 0 ? 1 : 0;
  const bool saved = target_slot == 0 ? this->preference_a_.save(&candidate) : this->preference_b_.save(&candidate);
  if (!saved) {
    ESP_LOGE(TAG, "Could not persist schedule revision %lu", static_cast<unsigned long>(candidate.revision));
    return;
  }
  if (!global_preferences->sync()) {
    ESP_LOGE(TAG, "Could not commit schedule revision %lu", static_cast<unsigned long>(candidate.revision));
    return;
  }
  this->schedule_ = candidate;
  this->active_slot_ = target_slot;
  this->format_hash_();
  this->active_state_ = -1;
  ESP_LOGI(TAG, "Installed cached ECS schedule revision %lu", static_cast<unsigned long>(candidate.revision));
  this->evaluate_();
}

int ECSProgram::current_state_(const ESPTime &now) const {
  if (!this->schedule_.valid)
    return -1;
  // ESPHome's ESPTime uses 1..7 for Sunday..Saturday; the ECS wire format
  // uses 0..6, matching Go's time.Weekday.
  const uint8_t weekday = now.day_of_week == 0 ? 0 : static_cast<uint8_t>(now.day_of_week - 1);
  const int current_week_minute = static_cast<int>(weekday) * 1440 + now.hour * 60 + now.minute;
  int best_distance = 7 * 1440;
  int best_state = this->schedule_.fallback_state;
  for (uint8_t i = 0; i < this->schedule_.transition_count; i++) {
    const auto &transition = this->schedule_.transitions[i];
    for (uint8_t transition_day = 0; transition_day < 7; transition_day++) {
      if ((transition.weekday_mask & (1u << transition_day)) == 0)
        continue;
      int distance = current_week_minute - (static_cast<int>(transition_day) * 1440 + transition.minute);
      if (distance < 0)
        distance += 7 * 1440;
      if (distance < best_distance) {
        best_distance = distance;
        best_state = transition.state;
      }
    }
  }
  return best_state;
}

void ECSProgram::evaluate_() {
  if (!this->time_ || !this->climate_ || !this->schedule_.valid)
    return;
  const auto now = this->time_->now();
  if (!now.is_valid())
    return;
  const int state = this->current_state_(now);
  if (state < 0 || state >= this->schedule_.state_count || state == this->active_state_)
    return;
  this->active_state_ = state;
  this->apply_state_(this->schedule_.states[state]);
  this->publish_diagnostics_();
}

void ECSProgram::apply_state_(const State &state) {
  auto call = this->climate_->make_call();
  if (state.mode == 0) {
    call.set_mode(climate::CLIMATE_MODE_OFF);
  } else if (state.mode == 1) {
    call.set_mode(climate::CLIMATE_MODE_HEAT_COOL);
    call.set_target_temperature_low(static_cast<float>(state.heat_deci) / 10.0f);
    call.set_target_temperature_high(static_cast<float>(state.cool_deci) / 10.0f);
  } else if (state.mode == 2) {
    call.set_mode(climate::CLIMATE_MODE_FAN_ONLY);
  } else {
    call.set_mode(climate::CLIMATE_MODE_DRY);
  }
  const climate::ClimateFanMode fan_modes[] = {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW,
                                                climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH};
  call.set_fan_mode(fan_modes[std::min<uint8_t>(state.fan, 3)]);
  const climate::ClimatePreset presets[] = {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_BOOST,
                                             climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_SLEEP};
  call.set_preset(presets[std::min<uint8_t>(state.preset, 3)]);
  call.perform();
}

void ECSProgram::publish_diagnostics_() {
  if (this->active_state_ < 0 || this->active_state_ >= this->schedule_.state_count)
    return;
  std::snprintf(this->active_profile_.data(), this->active_profile_.size(), "state_%d", this->active_state_);
}

void ECSProgram::format_hash_() {
  static constexpr char HEX_DIGITS[] = "0123456789abcdef";
  for (size_t i = 0; i < HASH_SIZE; i++) {
    this->active_hash_[i * 2] = HEX_DIGITS[this->schedule_.hash[i] >> 4];
    this->active_hash_[i * 2 + 1] = HEX_DIGITS[this->schedule_.hash[i] & 0x0f];
  }
  this->active_hash_[HASH_HEX_SIZE] = '\0';
}

int ECSProgram::hex_value_(uint8_t value) {
  if (value >= '0' && value <= '9')
    return value - '0';
  if (value >= 'a' && value <= 'f')
    return value - 'a' + 10;
  if (value >= 'A' && value <= 'F')
    return value - 'A' + 10;
  return -1;
}

uint32_t ECSProgram::crc32_(const uint8_t *data, size_t size) {
  uint32_t crc = 0xffffffffu;
  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++)
      crc = (crc >> 1) ^ (0xedb88320u & (-(static_cast<int32_t>(crc) & 1)));
  }
  return ~crc;
}

const char *ECSProgram::mode_name_(uint8_t mode) {
  static const char *names[] = {"off", "heat_cool", "fan_only", "dry"};
  return mode <= 3 ? names[mode] : "unknown";
}

const char *ECSProgram::fan_name_(uint8_t fan) {
  static const char *names[] = {"auto", "low", "medium", "high"};
  return fan <= 3 ? names[fan] : "unknown";
}

const char *ECSProgram::preset_name_(uint8_t preset) {
  static const char *names[] = {"none", "boost", "eco", "sleep"};
  return preset <= 3 ? names[preset] : "unknown";
}

}  // namespace esphome::ecs_program
