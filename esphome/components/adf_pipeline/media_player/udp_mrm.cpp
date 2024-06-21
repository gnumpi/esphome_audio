#include "udp_mrm.h"

#include <string.h>
#include <sstream>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esphome/core/log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <cJSON.h>

#ifdef USE_ESP_IDF

namespace esphome {
namespace esp_adf {

static const char *const TAG = "udp_mcast";

static void mrm_task(void *pvParameters)
{
  UdpMRM& self = *((UdpMRM*)pvParameters);

  // outer loop
  while (true) {

    char recvbuf[self.multicast_buffer_size];
    char addr_name[32] = { 0 };

    // create socket
    int sock = self.create_multicast_ipv4_socket();
    if (sock < 0) {
      esph_log_e(TAG, "Failed to create IPv4 multicast socket");
      vTaskDelay(5 / portTICK_PERIOD_MS);
      // try again
      continue;
    }

    // set destination multicast addresses for sending from these sockets
    struct sockaddr_in sdestv4 = {
      .sin_family = PF_INET,
      .sin_port = htons(self.udp_port),
    };
    // We know this inet_aton will pass because we did it above already
    inet_aton(self.multicast_ipv4_addr.c_str(), &sdestv4.sin_addr.s_addr);

    // Loop first look for UDP packets and then send UDP packets if send_actions.
    // see any.
    int err = 1;

    // inner loop
    while (err > 0) {

      // break out if stop requested
      if (self.stop_multicast) {
        esph_log_d(TAG, "Stopping multicast");
        break;
      }

      struct timeval tv = {
        .tv_sec = 2,
        .tv_usec = 0,
      };
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(sock, &rfds);

      int s = select(sock + 1, &rfds, NULL, NULL, &tv);

      // socket failure
      if (s < 0) {
        esph_log_e(TAG, "Select failed: errno %d", errno);
        err = s;
        // break out to outer loop
        break;
      }
      // socket has incoming datagram
      if (s > 0) {
        if (FD_ISSET(sock, &rfds)) {
          // Incoming datagram received
          struct sockaddr_storage raddr; // Large enough for both IPv4 or IPv6
          socklen_t socklen = sizeof(raddr);
          int len = recvfrom(sock, recvbuf, sizeof(recvbuf)-1, 0, (struct sockaddr *)&raddr, &socklen);
          if (len < 0) {
            esph_log_e(TAG, "multicast recvfrom failed: errno %d", errno);
            err = -1;
            break;
          }

          // Get the sender's address as a string
          if (raddr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&raddr)->sin_addr, addr_name, sizeof(addr_name)-1);
          }

          esph_log_v(TAG, "received %d bytes from %s:", len, addr_name);

          recvbuf[len] = 0; // Null-terminate whatever we received and treat like a string...
          esph_log_v(TAG, "%s", recvbuf);
          std::string message = recvbuf;
          std::string address = addr_name;
          self.process_multicast_message(message, address);
        }
      }
      
      // send messages to multicast
      if (self.send_actions.size() > 0) {
        std::string message = self.send_actions.front().to_string();
        esph_log_d(TAG, "Send: %s", message.c_str());
        int len = message.length();
        if (len > self.multicast_buffer_size) {
          ESP_LOGE(TAG, "%d is larger than will be able to be received: %d", len, self.multicast_buffer_size);
          err = -1;
          break;
        }

        struct addrinfo hints = {
          .ai_flags = AI_PASSIVE,
          .ai_socktype = SOCK_DGRAM,
        };
        struct addrinfo *res;
        hints.ai_family = AF_INET; // For an IPv4 socket

        int err = getaddrinfo(self.multicast_ipv4_addr.c_str(), NULL, &hints, &res);
        if (err < 0) {
          esph_log_e(TAG, "getaddrinfo() failed for IPV4 destination address. error: %d", err);
          break;
        }
        if (res == 0) {
          esph_log_e(TAG, "getaddrinfo() did not return any addresses");
          break;
        }
        ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(self.udp_port);
        inet_ntoa_r(((struct sockaddr_in *)res->ai_addr)->sin_addr, addr_name, sizeof(addr_name)-1);
        esph_log_v(TAG, "Sending to IPV4 multicast address %s:%d...",  addr_name, self.udp_port);
        esph_log_v(TAG, "message: %s, length: %d",message.c_str(), len);
        err = sendto(sock, message.c_str(), len, 0, res->ai_addr, res->ai_addrlen);
        freeaddrinfo(res);
        if (err < 0) {
          esph_log_e(TAG, "IPV4 sendto failed. errno: %d", errno);
          break;
        }
        self.send_actions.pop();
      }
      vTaskDelay(self.multicast_loop_sleep_ms / portTICK_PERIOD_MS);
    }
    esph_log_d(TAG, "Shutdown and close socket");
    shutdown(sock, 0);
    close(sock);
    if (self.stop_multicast) {
      self.multicast_running = false;
      self.stop_multicast = false;
      vTaskDelete(self.xhandle);
      break;
    }
    else {
      esph_log_d(TAG, "Restarting ...");
    }
  }
}

/* below are class methods */


std::string UdpMRMAction::to_string() {

  std::string message = "{\"action\":\"" + this->type + "\"";

  if (this->type == "ping_response") {;
    std::ostringstream otimestamp;
    otimestamp << this->timestamp;
    // send time as a string so that cJSON can parse
    message += ",\"time\":\"" + otimestamp.str()+"\"";
    message += ",\"sender\":\"" + data +"\"";
  }
  else if (this->type == "url") {
    message += ",\"url\":\"" + this->data + "\"";
  }
  message += "}";
  return message;
}

void UdpMRM::listen(media_player::MediaPlayerMRM mrm) {
  mrm_ = mrm;
  stop_multicast = false;
  if (!multicast_running) {
    esph_log_d(TAG, "Start multicast");
    esph_log_d(TAG, "Create Task");
    xTaskCreate(&mrm_task, "mrm_task", 4096, this, 5, &xhandle);
    multicast_running = true;
  }
  else {
    esph_log_d(TAG, "Task already exists");
  }
}

void UdpMRM::unlisten() {
  esph_log_d(TAG, "Stop multicast");
  stop_multicast = true;
}

void UdpMRM::set_stream_uri(const std::string& url) {
  esph_log_d(TAG, "set follower uri");
  UdpMRMAction action;
  action.type = "url";
  action.data = url;
  send_actions.push(action);
}

void UdpMRM::start() {
  esph_log_d(TAG, "Start follower media players");
  UdpMRMAction action;
  action.type = "start";
  send_actions.push(action);
}

void UdpMRM::stop() {
  esph_log_d(TAG, "Stop follower media players");
  UdpMRMAction action;
  action.type = "stop";
  send_actions.push(action);
}

void UdpMRM::process_multicast_message(std::string &message, std::string &sender) {
  if (message.length()) {
    esph_log_d(TAG, "Received: %s from %s", message.c_str(), sender.c_str());
    
    cJSON *root = cJSON_Parse(message.c_str());
    std::string action = cJSON_GetObjectItem(root,"action")->valuestring;
    if (action == "ping" && get_mrm() == media_player::MEDIA_PLAYER_MRM_LEADER) {
      UdpMRMAction action;
      action.type = "ping_response";
      action.data = sender;
      action.timestamp = get_timestamp();
      send_actions.push(action);
    }
    if (action == "ping_response" && get_mrm() == media_player::MEDIA_PLAYER_MRM_FOLLOWER) {
      std::string leader_timestamp_str = cJSON_GetObjectItem(root,"time")->valuestring;
      int64_t leader_timestamp = 0;

      int64_t half_duration = (get_timestamp() - ping_timestamp_) / 2;
      offset = ((leader_timestamp - half_duration) - ping_timestamp_);
    }
    else if (action == "start" && get_mrm() == media_player::MEDIA_PLAYER_MRM_FOLLOWER) {
      UdpMRMAction action;
      action.type = "start";
      recv_actions.push(action);
    }
    else if (action == "stop" && get_mrm() == media_player::MEDIA_PLAYER_MRM_FOLLOWER) {
      UdpMRMAction action;
      action.type = "stop";
      recv_actions.push(action);
    }
    else if (action == "url" && get_mrm() == media_player::MEDIA_PLAYER_MRM_FOLLOWER) {
      std::string url = cJSON_GetObjectItem(root,"url")->valuestring;
      UdpMRMAction action;
      action.type = "url";
      action.data = url;
      recv_actions.push(action);
    }
  }
}

int64_t UdpMRM::get_timestamp() {
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  return (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
}

/* Add a socket to the IPV4 multicast group */
int UdpMRM::socket_add_ipv4_multicast_group(int sock, bool assign_source_if)
{
  struct ip_mreq imreq = { 0 };
  struct in_addr iaddr = { 0 };
  int err = 0;
  // Configure source interface
  if (this->listen_all_if) {
    imreq.imr_interface.s_addr = IPADDR_ANY;
  } else {
    /* TBD - need to replace get_example_netif with correct call
    esp_netif_ip_info_t ip_info = { 0 };
    err = esp_netif_get_ip_info(get_example_netif(), &ip_info);
    if (err != ESP_OK) {
      esph_log_e(TAG, "Failed to get IP address info. Error 0x%x", err);
      return err;
    }
    inet_addr_from_ip4addr(&iaddr, &ip_info.ip);
    */
  }
  // Configure multicast address to listen to
  err = inet_aton(this->multicast_ipv4_addr.c_str(), &imreq.imr_multiaddr.s_addr);
  if (err != 1) {
    esph_log_e(TAG, "Configured IPV4 multicast address '%s' is invalid.", this->multicast_ipv4_addr.c_str());
    // Errors in the return value have to be negative
    err = -1;
    return err;
  }
  esph_log_i(TAG, "Configured IPV4 Multicast address %s", inet_ntoa(imreq.imr_multiaddr.s_addr));
  if (!IP_MULTICAST(ntohl(imreq.imr_multiaddr.s_addr))) {
    esph_log_w(TAG, "Configured IPV4 multicast address '%s' is not a valid multicast address. This will probably not work.", this->multicast_ipv4_addr.c_str());
  }

  if (assign_source_if) {
    // Assign the IPv4 multicast source interface, via its IP
    // (only necessary if this socket is IPV4 only)
    err = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &iaddr,
             sizeof(struct in_addr));
    if (err < 0) {
      esph_log_e(TAG, "Failed to set IP_MULTICAST_IF. Error %d", errno);
      return err;
    }
  }

  err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
             &imreq, sizeof(struct ip_mreq));
  if (err < 0) {
    esph_log_e(TAG, "Failed to set IP_ADD_MEMBERSHIP. Error %d", errno);
    return err;
  }

  return ESP_OK;
}

int UdpMRM::create_multicast_ipv4_socket(void)
{
  struct sockaddr_in saddr = { 0 };
  int sock = -1;
  int err = 0;

  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    esph_log_e(TAG, "Failed to create socket. Error %d", errno);
    return -1;
  }

  // Bind the socket to any address
  saddr.sin_family = PF_INET;
  saddr.sin_port = htons(this->udp_port);
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
  if (err < 0) {
    esph_log_e(TAG, "Failed to bind socket. Error %d", errno);
    close(sock);
    return err;
  }

  // Assign multicast TTL (set separately from normal interface TTL)
  uint8_t ttl = this->multicast_ttl;
  err = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(uint8_t));
  if (err < 0) {
    esph_log_e(TAG, "Failed to set IP_MULTICAST_TTL. Error %d", errno);
    close(sock);
    return err;
  }

  uint8_t loopback_val = true;
  err = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback_val, sizeof(uint8_t));
  if (err < 0) {
    esph_log_e(TAG, "Failed to set IP_MULTICAST_LOOP. Error %d", errno);
    close(sock);
    return err;
  }

  // this is also a listening socket, so add it to the multicast
  // group for listening...
  err = socket_add_ipv4_multicast_group(sock, true);
  if (err < 0) {
    close(sock);
    return err;
  }

  // All set, socket is configured for sending and receiving
  return sock;
}

}  // namespace esp_adf
}  // namespace esphome
#endif
