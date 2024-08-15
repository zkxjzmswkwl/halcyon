#pragma once
#include <string>
#include <array>
#include <cstdint>

enum ChannelType : uint8_t
{
  TEXT = 0,
  VOICE = 2,
  SEPARATOR = 4,
  FORUM = 15
};

static const char* USER_AGENT_VAL = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) discord/0.0.63 Chrome/124.0.6367.243 Electron/30.2.0 Safari/537.36";

struct Channel;
struct Server;
struct Message;

struct DiscordContext 
{
  DiscordContext(std::string token) : token(token) {}
  std::array<Server*, 200> servers{};
  std::string token;
  unsigned int server_count = 0;

  Server* focused_server = nullptr;
  Channel* focused_channel = nullptr;
};

struct Server
{
  Server(std::string name, std::string id) : name(name), id(id) {
    channel_count = 0;
  }
  std::string name;
  std::string id;
  std::array<Channel*, 300> channels{};
  unsigned int channel_count = 0;
};

struct Channel
{
  Channel(ChannelType channel_type, std::string name, std::string id) : channel_type(channel_type), name(name), id(id) {}
  ChannelType channel_type;
  // 100 temp
  std::array<Message*, 100> messages{};
  unsigned int message_count = 0;
  std::string name;
  std::string id;
};

struct Message
{
  Message(std::string timestamp, std::string author, std::string body) : timestamp(timestamp), author(author), body(body) {}
  std::string timestamp;
  std::string author;
  std::string body;
};


std::string post_login(std::string email, std::string password);

DiscordContext* get_context(std::string token);

bool get_servers(DiscordContext*);

bool get_channels_for_server(DiscordContext*, Server*);

bool get_focused_channel_content(DiscordContext*);

bool post_message(DiscordContext*, std::string message, std::string channelId);

void display_servers(DiscordContext*);

void display_channels_for_server(Server* server, ChannelType channel_type);
