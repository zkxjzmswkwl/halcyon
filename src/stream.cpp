#include "stream.h"
#include "discord.h"
#include <chrono>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXUserAgent.h>
#include <ixwebsocket/IXWebSocket.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <thread>

typedef ix::WebSocketMessageType MsgType;
std::string build_auth_object(std::string& token);
std::string build_identify_object(std::string& token);

ix::WebSocket* g_web_socket;

namespace discord
{
namespace stream
{
// This entire function is out of hand.
// Want to figure shit out before I fix it.
void open(__int64* disc_ctx_ptr)
{
    // it works so it must be fine >.<
    // TODO: Fix this entire god damn thing.
    DiscordContext* disc_ctx = (DiscordContext*)disc_ctx_ptr;
    ix::initNetSystem();
    ix::WebSocket web_socket;
    auto auth_blob = build_auth_object(disc_ctx->token);
    unsigned int heartbeat_interval = 41250;
    bool received_interval = false;

    g_web_socket = &web_socket;

    web_socket.setUrl(DISCORD_WS_URL);
    web_socket.setExtraHeaders(ix::WebSocketHttpHeaders{{"User-Agent", UA}, {"Origin", "https://discord.com"}});
    web_socket.disableAutomaticReconnection();

    web_socket.setOnMessageCallback(
        [&disc_ctx, &received_interval, &heartbeat_interval](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == MsgType::Message)
            {
                rapidjson::Document doc;
                doc.Parse(msg->str.c_str());

                auto op = doc["op"].GetInt();

                // General event recv
                if (op == 0)
                {
                    std::string t = doc["t"].GetString();
                    std::string wtf = "MESSAGE_CREATE";
                    if (t == wtf)
                    {
                        auto d = doc["d"].GetObj();
                        if (d.HasMember("channel_id"))
                        {
                            auto channel_id = d["channel_id"].GetString();
                            if (disc_ctx->focused_channel->id == channel_id)
                            {
                                auto timestamp = d["timestamp"].GetString();
                                auto author = d["author"].GetObj()["global_name"].GetString();
                                auto body = d["content"].GetString();

                                // pretty dumb.
                                disc_ctx->focused_channel->messages.push_back(new Message(timestamp, author, body));
                                printf("added message with content %s.\n", body);
                            }
                        }
                    }
                }
                // Packet informing of us of the heartbeat interval.
                else if (op == 10)
                {
                    auto d = doc["d"].GetObj();
                    heartbeat_interval = d["heartbeat_interval"].GetInt();
                    received_interval = true;
                }
                // Packet acking our heartbeat.
                else if (op == 11)
                {
                    printf("Discord ack heartbeat\n");
                }
            }
            else if (msg->type == MsgType::Open)
            {
                printf("Connected to Discord ws.\n");
            }
            else if (msg->type == MsgType::Error)
            {
                printf("Websocket connection issue -> %s\n", msg->errorInfo.reason.c_str());
            }
        });

    web_socket.start();
    web_socket.send(auth_blob);

    bool initial_heartbeat = false;
    while (true)
    {
        if (received_interval)
        {
            // This shit's braindead. Fix it.
            if (!initial_heartbeat)
            {
                web_socket.send(R"({"op": 1, "d": null})");
                web_socket.send(build_identify_object(disc_ctx->token).c_str());
                initial_heartbeat = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_interval));
            web_socket.send(R"({"op": 1, "d": null})");
        }
    }
}

void send(std::string packet)
{
    g_web_socket->send(packet);
}

std::string build_request_messages_object(std::string guild_id, std::string channel_id)
{
    using namespace rapidjson;

    Document doc;
    doc.SetObject();

    Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember("op", 34, allocator);

    Value d(kObjectType);
    d.AddMember("guild_id", Value().SetString(guild_id.c_str(), allocator), allocator);

    Value channel_ids(kArrayType);
    channel_ids.PushBack(Value().SetString(channel_id.c_str(), allocator), allocator);

    d.AddMember("channel_ids", channel_ids, allocator);
    doc.AddMember("d", d, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

} // namespace stream
} // namespace discord

std::string build_auth_object(std::string& token)
{
    using namespace rapidjson;

    Document doc;
    doc.SetObject();

    Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember("op", 2, allocator);

    Value d(kObjectType);
    d.AddMember("token", Value().SetString(token.c_str(), allocator), allocator);
    d.AddMember("compress", Value().SetBool(false), allocator);
    d.AddMember("properties", Value(kObjectType), allocator);

    Value presence(kObjectType);
    presence.AddMember("status", "online", allocator);
    presence.AddMember("activities", Value(kArrayType), allocator);

    d.AddMember("presence", presence, allocator);
    doc.AddMember("d", d, allocator);

    // Convert that shit to std::string
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

std::string build_identify_object(std::string& token)
{
    using namespace rapidjson;

    Document document;
    document.SetObject();

    Document::AllocatorType& allocator = document.GetAllocator();

    document.AddMember("op", 2, allocator);

    Value d(kObjectType);

    d.AddMember("token", Value().SetString(token.c_str(), allocator), allocator);

    Value properties(kObjectType);
    properties.AddMember("os", Value().SetString("linux", allocator), allocator);
    properties.AddMember("browser", Value().SetString("disco", allocator), allocator);
    properties.AddMember("device", Value().SetString("disco", allocator), allocator);

    d.AddMember("properties", properties, allocator);
    d.AddMember("compress", false, allocator);

    Value presence(kObjectType);
    presence.AddMember("activities", Value(kArrayType), allocator);
    presence.AddMember("status", Value().SetString("unknown", allocator), allocator);
    presence.AddMember("since", 0, allocator);
    presence.AddMember("afk", false, allocator);

    d.AddMember("presence", presence, allocator);
    d.AddMember("capabilities", 16381, allocator);

    Value client_state(kObjectType);
    client_state.AddMember("api_code_version", 0, allocator);

    Value guild_versions(kObjectType);
    client_state.AddMember("guild_versions", guild_versions, allocator);

    d.AddMember("client_state", client_state, allocator);

    document.AddMember("d", d, allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    document.Accept(writer);

    return buffer.GetString();
}
