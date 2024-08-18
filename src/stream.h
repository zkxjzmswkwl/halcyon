#pragma once
#include <string>

const std::string DISCORD_WS_URL = "wss://gateway.discord.gg/?v=10&encoding=json";
// Temp
const std::string UA = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) discord/0.0.63 "
                       "Chrome/124.0.6367.243 Electron/30.2.0 Safari/537.36";

namespace discord
{
namespace stream
{

struct WebSocketContext
{
    WebSocketContext() {}
    bool has_received_interval = false;
    unsigned int heartbeat_interval;
};

void open();

void send(std::string packet);
} // namespace stream
} // namespace discord
