#pragma once

#include "uvgrtp/frame.hh"

#include <vector>


namespace uvgrtp
{
    const uint16_t RTCP_HEADER_SIZE = 4;
    const uint16_t SSRC_CSRC_SIZE = 4;
    const uint16_t SENDER_INFO_SIZE = 20;
    const uint16_t REPORT_BLOCK_SIZE = 24;
    const uint16_t APP_NAME_SIZE = 4;

    size_t get_sr_packet_size(int flags, uint16_t reports);
    size_t get_rr_packet_size(int flags, uint16_t reports);
    size_t get_sdes_packet_size(const std::vector<uvgrtp::frame::rtcp_sdes_item>& items);
    size_t get_app_packet_size(size_t payload_len);
    size_t get_bye_packet_size(const std::vector<uint32_t>& ssrcs);

    // Add the RTCP header
    bool construct_rtcp_header(uint8_t* frame, int& ptr, size_t packet_size,
        uint16_t secondField, uvgrtp::frame::RTCP_FRAME_TYPE frame_type);

    // Add an ssrc to packet, not present in all packets
    bool construct_ssrc(uint8_t* frame, int& ptr, uint32_t ssrc);

    // Add sender info for sender report
    bool construct_sender_info(uint8_t* frame, int& ptr,
        uint64_t ntp_ts, uint64_t rtp_ts, uint32_t sent_packets, uint32_t sent_bytes);

    // Add one report block for sender or receiver report
    bool construct_report_block(uint8_t* frame, int& ptr, uint32_t ssrc, uint8_t fraction,
        uint32_t dropped_packets, uint16_t seq_cycles, uint16_t max_seq, uint32_t jitter,
        uint32_t lsr, uint32_t dlsr);

    // Add the items to the frame
    bool construct_sdes_chunk(uint8_t* frame, int& ptr, uvgrtp::frame::rtcp_sdes_chunk chunk);

    // Add the name and payload to APP packet, remember to also add SSRC separately
    bool construct_app_packet(uint8_t* frame, int& ptr,
        const char* name, const uint8_t* payload, size_t payload_len);

    // Add BYE ssrcs, should probably be removed
    bool construct_bye_packet(uint8_t* frame, int& ptr, const std::vector<uint32_t>& ssrcs);
}