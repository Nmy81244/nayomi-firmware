#pragma once

#include <cstddef>
#include <cstdint>

namespace nayomi_protocol
{
constexpr uint8_t kMagic0 = 0x4E; // 'N'
constexpr uint8_t kMagic1 = 0x59; // 'Y'
constexpr uint8_t kVersion = 1;
constexpr uint16_t kMaxPayload = 128;
constexpr uint16_t kMaxFrameSize = 2 + 1 + 1 + 1 + 2 + kMaxPayload + 2;

enum : uint8_t
{
    CMD_PING       = 0x01,
    CMD_GET_INFO   = 0x02,
    CMD_GET_STATUS = 0x03,
    CMD_GET_HALL   = 0x04,
    CMD_RESET      = 0x05,

    RSP_MASK       = 0x80,

    EVT_HALL       = 0x90
};

struct Frame
{
    uint8_t version = 0;
    uint8_t type = 0;
    uint8_t sequence = 0;
    uint16_t length = 0;
    uint8_t payload[kMaxPayload] = {};
};

uint16_t crc16_ccitt(const uint8_t *data, std::size_t length);

uint16_t encode_frame(
    uint8_t type,
    uint8_t sequence,
    const uint8_t *payload,
    uint16_t length,
    uint8_t *output,
    uint16_t output_capacity
);

class Decoder
{
public:
    void reset();

    void feed(const uint8_t *data, std::size_t length);

    bool take_frame(Frame &frame);

    uint32_t frame_count() const { return frame_count_; }
    uint32_t crc_error_count() const { return crc_error_count_; }
    uint32_t malformed_count() const { return malformed_count_; }

private:
    enum class State : uint8_t
    {
        SEEK_MAGIC0,
        SEEK_MAGIC1,
        HEADER,
        PAYLOAD,
        READ_CRC
    };

    void reset_parser();

    State state_ = State::SEEK_MAGIC0;
    uint8_t header_[5] = {};
    uint8_t header_pos_ = 0;
    Frame current_{};
    uint16_t payload_pos_ = 0;
    uint8_t crc_bytes_[2] = {};
    uint8_t crc_pos_ = 0;

    static constexpr uint8_t kQueueDepth = 4;
    Frame queue_[kQueueDepth] = {};
    uint8_t queue_head_ = 0;
    uint8_t queue_tail_ = 0;
    uint8_t queue_count_ = 0;

    uint32_t frame_count_ = 0;
    uint32_t crc_error_count_ = 0;
    uint32_t malformed_count_ = 0;
};
}
