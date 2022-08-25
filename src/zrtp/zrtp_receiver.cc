#include "zrtp_receiver.hh"

#include "uvgrtp/socket.hh"
#include "uvgrtp/util.hh"

#include "defines.hh"
#include "dh_kxchng.hh"
#include "commit.hh"
#include "confack.hh"
#include "confirm.hh"
#include "hello.hh"
#include "hello_ack.hh"
#include "../poll.hh"
#include "../crypto.hh"
#include "../debug.hh"


#ifdef _WIN32
#include <winsock2.h>
#include <mswsock.h>
#include <inaddr.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#endif

#include <cstring>


using namespace uvgrtp::zrtp_msg;

uvgrtp::zrtp_msg::receiver::receiver():
    mem_(new uint8_t[1024]),
    len_(1024),
    rlen_(0)
{}

uvgrtp::zrtp_msg::receiver::~receiver()
{
    UVG_LOG_DEBUG("destroy receiver");
    delete[] mem_;
}

rtp_error_t uvgrtp::zrtp_msg::receiver::recv_msg(std::shared_ptr<uvgrtp::socket> socket, int timeout, 
   int recv_flags, int& out_type)
{
    rtp_error_t ret = RTP_GENERIC_ERROR;
    int nread       = 0;
    rlen_           = 0;

    if (timeout > 0)
    {
        UVG_LOG_DEBUG("Waiting for ZRTP messages with timeout of %i ms", timeout);
    }
    else
    {
        UVG_LOG_DEBUG("Checking if there is a ZRTP message in buffer");
    }

#ifdef _WIN32

    (void)recv_flags;

    if ((ret = uvgrtp::poll::blocked_recv(socket, mem_, len_, timeout, &nread)) != RTP_OK) {
        if (ret == RTP_INTERRUPTED)
            return ret;

        log_platform_error("blocked_recv() failed");
        return RTP_RECV_ERROR;
    }
#else
    size_t msec = timeout % 1000;
    size_t sec  = timeout - msec;

    struct timeval tv = {
        (int)sec  / 1000,
        (int)msec * 1000,
    };

    if (socket->setsockopt(SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) != RTP_OK)
        return RTP_GENERIC_ERROR;

    if ((ret = socket->recv(mem_, len_, recv_flags, &nread)) != RTP_OK) {
        if (ret == RTP_INTERRUPTED)
            return ret;

        log_platform_error("recv(2) failed");
        return RTP_RECV_ERROR;
    }
#endif

    zrtp_msg *msg = (zrtp_msg *)mem_;
    rlen_         = nread;

    if (msg->header.version != 0) {
        UVG_LOG_DEBUG("Invalid header version 0");
        return RTP_INVALID_VALUE;
    }

    if (msg->header.magic != ZRTP_HEADER_MAGIC) {
        UVG_LOG_DEBUG("Invalid ZRTP header magic");
        return RTP_INVALID_VALUE;
    }

    if (msg->magic != ZRTP_MSG_MAGIC) {
        UVG_LOG_DEBUG("invalid ZRTP magic");
        return RTP_INVALID_VALUE;
    }

    switch (msg->msgblock) {
        case ZRTP_MSG_HELLO:
        {
            UVG_LOG_DEBUG("Hello message received, verify CRC32!");

            zrtp_hello *hello = (zrtp_hello *)msg;

            if (!uvgrtp::crypto::crc32::verify_crc32(mem_, rlen_ - 4, hello->crc))
                return RTP_NOT_SUPPORTED;
        }
        out_type = ZRTP_FT_HELLO;
        return RTP_OK;

        case ZRTP_MSG_HELLO_ACK:
        {
            UVG_LOG_DEBUG("HelloACK message received, verify CRC32!");

            zrtp_hello_ack *ha_msg = (zrtp_hello_ack *)msg;

            if (!uvgrtp::crypto::crc32::verify_crc32(mem_, rlen_ - 4, ha_msg->crc))
                return RTP_NOT_SUPPORTED;
        }
        out_type = ZRTP_FT_HELLO_ACK;
        return RTP_OK;

        case ZRTP_MSG_COMMIT:
        {
            UVG_LOG_DEBUG("Commit message received, verify CRC32!");

            zrtp_commit *commit = (zrtp_commit *)msg;

            if (!uvgrtp::crypto::crc32::verify_crc32(mem_, rlen_ - 4, commit->crc))
                return RTP_NOT_SUPPORTED;
        }
        out_type = ZRTP_FT_COMMIT;
        return RTP_OK;

        case ZRTP_MSG_DH_PART1:
        {
            UVG_LOG_DEBUG("DH Part1 message received, verify CRC32!");

            zrtp_dh *dh = (zrtp_dh *)msg;

            if (!uvgrtp::crypto::crc32::verify_crc32(mem_, rlen_ - 4, dh->crc))
                return RTP_NOT_SUPPORTED;
        }
        out_type = ZRTP_FT_DH_PART1;
        return RTP_OK;

        case ZRTP_MSG_DH_PART2:
        {
            UVG_LOG_DEBUG("DH Part2 message received, verify CRC32!");

            zrtp_dh *dh = (zrtp_dh *)msg;

            if (!uvgrtp::crypto::crc32::verify_crc32(mem_, rlen_ - 4, dh->crc))
                return RTP_NOT_SUPPORTED;
        }
        out_type = ZRTP_FT_DH_PART2;
        return RTP_OK;

        case ZRTP_MSG_CONFIRM1:
        {
            UVG_LOG_DEBUG("Confirm1 message received, verify CRC32!");

            zrtp_confirm *dh = (zrtp_confirm *)msg;

            if (!uvgrtp::crypto::crc32::verify_crc32(mem_, rlen_ - 4, dh->crc))
                return RTP_NOT_SUPPORTED;
        }
        out_type = ZRTP_FT_CONFIRM1;
        return RTP_OK;

        case ZRTP_MSG_CONFIRM2:
        {
            UVG_LOG_DEBUG("Confirm2 message received, verify CRC32!");

            zrtp_confirm *dh = (zrtp_confirm *)msg;

            if (!uvgrtp::crypto::crc32::verify_crc32(mem_, rlen_ - 4, dh->crc))
                return RTP_NOT_SUPPORTED;
        }
        out_type = ZRTP_FT_CONFIRM2;
        return RTP_OK;

        case ZRTP_MSG_CONF2_ACK:
        {
            UVG_LOG_DEBUG("Conf2 ACK message received, verify CRC32!");

            zrtp_confack *ca = (zrtp_confack *)msg;

            if (!uvgrtp::crypto::crc32::verify_crc32(mem_, rlen_ - 4, ca->crc))
                return RTP_NOT_SUPPORTED;
        }
        out_type = ZRTP_FT_CONF2_ACK;
        return RTP_OK;

        case ZRTP_MSG_ERROR:
        {
            UVG_LOG_DEBUG("Error message received");
            out_type = ZRTP_FT_ERROR;
            return RTP_OK;
        }

        case ZRTP_MSG_ERROR_ACK:
        {
            UVG_LOG_DEBUG("Error ACK message received");
            out_type = ZRTP_FT_ERROR_ACK;
            return RTP_OK;
        }

        case ZRTP_MSG_SAS_RELAY:
        {
            UVG_LOG_DEBUG("SAS Relay message received");
            out_type = ZRTP_FT_SAS_RELAY;
            return RTP_OK;
        }

        case ZRTP_MSG_RELAY_ACK:
        {
            UVG_LOG_DEBUG("Relay ACK message received");
            out_type = ZRTP_FT_RELAY_ACK;
            return RTP_OK;
        }

        case ZRTP_MSG_PING_ACK:
        {
            UVG_LOG_DEBUG("Ping ACK message received");
            out_type = ZRTP_FT_PING_ACK;
            return RTP_OK;
        }
    }

    UVG_LOG_WARN("Unknown message type received: 0x%lx", (int)msg->msgblock);
    return RTP_NOT_SUPPORTED;
}

ssize_t uvgrtp::zrtp_msg::receiver::get_msg(void *ptr, size_t len)
{
    if (!ptr || !len)
        return RTP_INVALID_VALUE;

    size_t cpy_len = rlen_;

    if (len < rlen_) {
        cpy_len = len;
        UVG_LOG_WARN("Destination buffer too small, cannot copy full message (%zu %zu)!", len, rlen_);
    }

    memcpy(ptr, mem_, cpy_len);
    return rlen_;
}

