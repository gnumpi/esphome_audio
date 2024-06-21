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
};

class UdpMRM {
  public:
    std::string multicast_ipv4_addr{"239.255.255.252"};
    int udp_port{1900};
    uint8_t multicast_ttl{5};
    bool listen_all_if{true};
    bool stop_multicast{false};
    size_t multicast_buffer_size{1024};
    int multicast_loop_sleep_ms{500};
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
    media_player::MediaPlayerMRM get_mrm() { return mrm_; }

    int64_t get_timestamp();

    //used by mrm_task
    int socket_add_ipv4_multicast_group(int sock, bool assign_source_if);
    int create_multicast_ipv4_socket(void);
    int create_multicast_ipv6_socket(void);
    void process_multicast_message(std::string &message, std::string &sender);
    std::string prepare_multicast_message();

  protected:
    media_player::MediaPlayerMRM mrm_{media_player::MEDIA_PLAYER_MRM_OFF};
    int64_t ping_timestamp_{};
};

}  // namespace esp_adf
}  // namespace esphome

#endif
