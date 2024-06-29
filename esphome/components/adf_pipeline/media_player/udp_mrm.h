#pragma once

#include <string>
#include <queue>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esphome/components/media_player/media_player.h"

#ifdef USE_ESP_IDF

namespace esphome {
namespace esp_adf {

class UdpMRMAction {
  public:
    std::string type{""};
    std::string data{""};
    int64_t timestamp{};
    std::string to_string();
    int64_t get_timestamp();
};

class UdpMRM {
  public:
    // should be configurable through yaml
    std::string multicast_ipv4_addr{"239.255.255.252"};
    //port 1900 is in use, avoid
    int udp_port{1901};
    uint8_t multicast_ttl{2};
    bool listen_all_if{false};

    // internal
    std::string this_addr_{""};
    bool stop_multicast{false};
    size_t multicast_buffer_size{512};
    bool multicast_running{false};
    TaskHandle_t xhandle{};
    int64_t offset{};
    std::queue<UdpMRMAction> send_actions;
    std::queue<UdpMRMAction> recv_actions;

    void listen(media_player::MediaPlayerMRM mrm);
    void unlisten();
    void set_stream_uri(const std::string& url);
    void start();
    void stop();
    void resume();
    void uninitialize();
    void send_ping();
    void send_position(int64_t timestamp, int64_t position);

    media_player::MediaPlayerMRM get_mrm() { return mrm_; }

    int64_t get_timestamp();

    //used by mrm_task
    int socket_add_ipv4_multicast_group(int sock, bool assign_source_if);
    int create_multicast_ipv4_socket(void);
    void process_multicast_message(std::string &message, std::string &sender);
    int64_t ping_timestamp{};

  protected:
    media_player::MediaPlayerMRM mrm_{media_player::MEDIA_PLAYER_MRM_OFF};
};

}  // namespace esp_adf
}  // namespace esphome

#endif
