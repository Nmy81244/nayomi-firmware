#include "nayomi_protocol.h"

#include <cstring>

namespace nayomi_protocol
{
namespace
{
void write_u16_le(uint8_t *dst, uint16_t value)
{
    dst[0] = static_cast<uint8_t>(value & 0xFFU);
    dst[1] = static_cast<uint8_t>(value >> 8);
}
}

uint16_t crc16_ccitt(const uint8_t *data, std::size_t length)
{
    uint16_t crc = 0xFFFFU;

    for(std::size_t i = 0; i < length; ++i)
    {
        crc ^= static_cast<uint16_t>(data[i]) << 8;

        for(uint8_t bit = 0; bit < 8; ++bit)
        {
            crc = (crc & 0x8000U)
                ? static_cast<uint16_t>((crc << 1) ^ 0x1021U)
                : static_cast<uint16_t>(crc << 1);
        }
    }

    return crc;
}

uint16_t encode_frame(
    uint8_t type,
    uint8_t sequence,
    const uint8_t *payload,
    uint16_t length,
    uint8_t *output,
    uint16_t output_capacity
)
{
    if(length > kMaxPayload)
        return 0;

    const uint16_t total_size =
        static_cast<uint16_t>(2 + 5 + length + 2);

    if(output == nullptr || output_capacity < total_size)
        return 0;

    output[0] = kMagic0;
    output[1] = kMagic1;
    output[2] = kVersion;
    output[3] = type;
    output[4] = sequence;
    write_u16_le(&output[5], length);

    if(length != 0 && payload != nullptr)
        std::memcpy(&output[7], payload, length);

    const uint16_t crc =
        crc16_ccitt(&output[2], static_cast<std::size_t>(5 + length));

    write_u16_le(&output[7 + length], crc);

    return total_size;
}

void Decoder::reset_parser()
{
    state_ = State::SEEK_MAGIC0;
    header_pos_ = 0;
    payload_pos_ = 0;
    crc_pos_ = 0;
    current_ = {};
}

void Decoder::reset()
{
    reset_parser();
    queue_head_ = 0;
    queue_tail_ = 0;
    queue_count_ = 0;
    frame_count_ = 0;
    crc_error_count_ = 0;
    malformed_count_ = 0;
}

void Decoder::feed(const uint8_t *data, std::size_t length)
{
    if(data == nullptr)
        return;

    for(std::size_t i = 0; i < length; ++i)
    {
        const uint8_t byte = data[i];

        switch(state_)
        {
            case State::SEEK_MAGIC0:
                if(byte == kMagic0)
                    state_ = State::SEEK_MAGIC1;
                break;

            case State::SEEK_MAGIC1:
                if(byte == kMagic1)
                {
                    header_pos_ = 0;
                    state_ = State::HEADER;
                }
                else
                {
                    state_ = (byte == kMagic0)
                        ? State::SEEK_MAGIC1
                        : State::SEEK_MAGIC0;
                }
                break;

            case State::HEADER:
                header_[header_pos_++] = byte;

                if(header_pos_ == sizeof(header_))
                {
                    current_.version = header_[0];
                    current_.type = header_[1];
                    current_.sequence = header_[2];
                    current_.length =
                        static_cast<uint16_t>(header_[3]) |
                        static_cast<uint16_t>(header_[4] << 8);

                    if(current_.version != kVersion ||
                       current_.length > kMaxPayload)
                    {
                        ++malformed_count_;
                        reset_parser();
                    }
                    else if(current_.length == 0)
                    {
                        crc_pos_ = 0;
                        state_ = State::CRC;
                    }
                    else
                    {
                        payload_pos_ = 0;
                        state_ = State::PAYLOAD;
                    }
                }
                break;

            case State::PAYLOAD:
                current_.payload[payload_pos_++] = byte;

                if(payload_pos_ == current_.length)
                {
                    crc_pos_ = 0;
                    state_ = State::CRC;
                }
                break;

            case State::CRC:
                crc_bytes_[crc_pos_++] = byte;

                if(crc_pos_ == sizeof(crc_bytes_))
                {
                    const uint16_t received_crc =
                        static_cast<uint16_t>(crc_bytes_[0]) |
                        static_cast<uint16_t>(crc_bytes_[1] << 8);

                    uint8_t crc_input[5 + kMaxPayload] = {};

                    crc_input[0] = current_.version;
                    crc_input[1] = current_.type;
                    crc_input[2] = current_.sequence;
                    crc_input[3] = static_cast<uint8_t>(current_.length & 0xFFU);
                    crc_input[4] = static_cast<uint8_t>(current_.length >> 8);

                    if(current_.length != 0)
                    {
                        std::memcpy(
                            &crc_input[5],
                            current_.payload,
                            current_.length
                        );
                    }

                    const uint16_t computed_crc =
                        crc16_ccitt(
                            crc_input,
                            static_cast<std::size_t>(5 + current_.length)
                        );

                    if(received_crc != computed_crc)
                    {
                        ++crc_error_count_;
                        reset_parser();
                        break;
                    }

                    if(queue_count_ == kQueueDepth)
                    {
                        ++malformed_count_;
                        reset_parser();
                        break;
                    }

                    queue_[queue_tail_] = current_;
                    queue_tail_ = static_cast<uint8_t>(
                        (queue_tail_ + 1) % kQueueDepth
                    );
                    ++queue_count_;
                    ++frame_count_;

                    reset_parser();
                }
                break;
        }
    }
}

bool Decoder::take_frame(Frame &frame)
{
    if(queue_count_ == 0)
        return false;

    frame = queue_[queue_head_];
    queue_head_ = static_cast<uint8_t>(
        (queue_head_ + 1) % kQueueDepth
    );
    --queue_count_;

    return true;
}
}
