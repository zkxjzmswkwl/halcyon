#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace discord
{
enum ChannelType : uint8_t
{
    TEXT = 0,
    VOICE = 2,
    SEPARATOR = 4,
    FORUM = 15
};

static const char* USER_AGENT_VAL = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
                                    "discord/0.0.63 Chrome/124.0.6367.243 Electron/30.2.0 Safari/537.36";

struct Channel;
struct Server;
struct Message;
struct Friend;

struct DiscordContext
{
    DiscordContext(std::string token) : token(token)
    {
    }
    std::array<Server*, 200> servers{};
    std::vector<Friend*> friends{};
    std::string token;
    unsigned int server_count = 0;

    Server* focused_server = nullptr;
    Channel* focused_channel = nullptr;
};

struct Server
{
    Server(std::string name, std::string id) : name(name), id(id)
    {
        channel_count = 0;
    }
    std::string name;
    std::string id;
    std::array<Channel*, 1500> channels{};
    unsigned int channel_count = 0;
};

struct Channel
{
    Channel(ChannelType channel_type, std::string name, std::string id) : channel_type(channel_type), name(name), id(id)
    {
    }
    ChannelType channel_type;
    // 100 temp
    std::vector<Message*> messages{};
    std::string name;
    std::string id;
};

struct Message
{
    Message(std::string timestamp, std::string author, std::string body)
        : timestamp(timestamp), author(author), body(body)
    {
    }
    std::string timestamp;
    std::string author;
    std::string body;
};

struct Friend
{
    Friend(std::string id, std::string username) : id(id), username(username)
    {
    }
    std::string id;
    std::string username;
};

std::string post_login(std::string email, std::string password);

DiscordContext* get_context(std::string token);

bool get_servers(DiscordContext*);

bool get_channels_for_server(DiscordContext*, Server*);

bool get_focused_channel_content(DiscordContext*);

bool get_friends(DiscordContext*);

std::string get_chat_id_from_user(DiscordContext*, std::string);

bool post_message(DiscordContext*, std::string message, std::string channelId);

void display_servers(DiscordContext*);

void display_channels_for_server(Server* server, ChannelType channel_type);

void display_friends(DiscordContext*);
} // namespace discord
