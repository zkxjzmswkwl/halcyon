#pragma once
#include <string>
#include <array>
#include <cstdint>

enum ChannelType : uint8_t
{
  TEXT = 1,
  VOICE = 2,
  SEPARATOR = 4
};

struct Channel;
struct Server;

struct DiscordContext 
{
  DiscordContext(std::string token) : token(token) {}
  std::array<Server*, 200> servers{};
  std::string token;
  unsigned int server_count = 0;
};

struct Server
{
  Server(std::string name, std::string id) : name(name), id(id) {
    channel_count = 0;
  }
  std::string name;
  std::string id;
  std::array<Channel*, 300> channels{};
  unsigned int channel_count;
};

struct Channel {
  Channel(ChannelType channel_type, std::string name, std::string id) : channel_type(channel_type), name(name), id(id) {}
  ChannelType channel_type;
  std::string name;
  std::string id;
};


std::string post_login(std::string email, std::string password);

DiscordContext* get_context(std::string token);

bool get_servers(DiscordContext*);

bool get_channels_for_server(DiscordContext*, Server* server);

bool post_message(DiscordContext*, std::string message, std::string channelId);

