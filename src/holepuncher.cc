#include "holepuncher.hh"

#include "uvgrtp/clock.hh"

#include "socket.hh"
#include "debug.hh"


#define THRESHOLD 2000

uvgrtp::holepuncher::holepuncher(std::shared_ptr<uvgrtp::socket> socket):
    socket_(socket),
    last_dgram_sent_(0),
    active_(false)
{}

uvgrtp::holepuncher::~holepuncher()
{
    stop();
}

rtp_error_t uvgrtp::holepuncher::start()
{
    active_ = true;
    runner_ = std::unique_ptr<std::thread> (new std::thread(&uvgrtp::holepuncher::keepalive, this));
    return RTP_OK;
}

rtp_error_t uvgrtp::holepuncher::stop()
{
    active_ = false;
    if (runner_ && runner_->joinable())
    {
        runner_->join();
    }
    return RTP_OK;
}

void uvgrtp::holepuncher::notify()
{
    last_dgram_sent_ = uvgrtp::clock::ntp::now();
}

void uvgrtp::holepuncher::keepalive()
{
    UVG_LOG_DEBUG("Starting holepuncher");

    // TODO: Make this follow https://datatracker.ietf.org/doc/html/rfc6263
    while (active_) {
        if (uvgrtp::clock::ntp::diff_now(last_dgram_sent_) < THRESHOLD) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        UVG_LOG_DEBUG("Sending keep-alive");
        uint8_t payload = 0x00;
        socket_->sendto(&payload, 1, 0);
        last_dgram_sent_ = uvgrtp::clock::ntp::now();
    }
    UVG_LOG_DEBUG("Stopping holepuncher");
}
