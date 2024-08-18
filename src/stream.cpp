#include "stream.h"
#include "discord.h"
#include "globals.h"
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
std::string build_auth_object();
std::string build_identify_object();
void message_intake(const ix::WebSocketMessagePtr& msg);

ix::WebSocket* g_web_socket;
discord::stream::WebSocketContext* g_ws_ctx;

namespace discord
{
namespace stream
{
void open()
{
    g_ws_ctx = new WebSocketContext();

    ix::initNetSystem();
    ix::WebSocket web_socket;
    auto auth_blob = build_auth_object();
    unsigned int heartbeat_interval = 41250;
    bool received_interval = false;

    g_web_socket = &web_socket;

    web_socket.setUrl(DISCORD_WS_URL);
    web_socket.setExtraHeaders(ix::WebSocketHttpHeaders{{"User-Agent", UA}, {"Origin", "https://discord.com"}});
    web_socket.disableAutomaticReconnection();
    web_socket.setOnMessageCallback(message_intake);

    web_socket.start();
    web_socket.send(auth_blob);

    bool initial_heartbeat = false;

    // Wait until we've received a packet containing the heartbeat interval before we send initial heartbeat.
    while (!g_ws_ctx->has_received_interval)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Directly after the gateway expects a packet from the client identifying itself.
    web_socket.send(R"({"op": 1, "d": null})");
    web_socket.send(build_identify_object());

    // This keeps stops `web_socket` from being destroyed and also adheres to the gateway's heartbeat specification.
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_interval));
        web_socket.send(R"({"op": 1, "d": null})");
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

std::string build_auth_object()
{
    using namespace rapidjson;

    Document doc;
    doc.SetObject();

    Document::AllocatorType& allocator = doc.GetAllocator();

    doc.AddMember("op", 2, allocator);

    Value d(kObjectType);
    d.AddMember("token", Value().SetString(g_ctx->token.c_str(), allocator), allocator);
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

std::string build_identify_object()
{
    using namespace rapidjson;

    Document document;
    document.SetObject();

    Document::AllocatorType& allocator = document.GetAllocator();

    document.AddMember("op", 2, allocator);

    Value d(kObjectType);

    d.AddMember("token", Value().SetString(g_ctx->token.c_str(), allocator), allocator);

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

void gateway_event_handler(const ix::WebSocketMessagePtr& msg)
{
    rapidjson::Document doc;
    doc.Parse(msg->str.c_str());

    if (!doc.HasMember("op") || !doc.HasMember("d"))
        return;

    auto op = doc["op"].GetInt();

    switch (op)
    {
    // General event transmission. For now we only care about `MESSAGE_CREATE`
    case 0: {
        if (!doc.HasMember("t"))
            return;
        if (strcmp(doc["t"].GetString(), "MESSAGE_CREATE") != 0)
            return;

        auto d = doc["d"].GetObj();

        auto channel_id = d["channel_id"].GetString();
        if (g_ctx->focused_channel->id == channel_id)
        {
            auto timestamp = d["timestamp"].GetString();
            auto author = d["author"].GetObj()["global_name"].GetString();
            auto body = d["content"].GetString();

            g_ctx->focused_channel->messages.push_back(new discord::Message(timestamp, author, body));
        }
        break;
    }
    // Packet informing of us of the heartbeat interval.
    case 10: {
        // Don't need to check for key presence here.
        // Reason being: if they aren't there, we should crash.
        auto d = doc["d"].GetObj();
        g_ws_ctx->has_received_interval = true;
        g_ws_ctx->heartbeat_interval = d["heartbeat_interval"].GetInt();
        break;
    }
    } // ends switch
}

void message_intake(const ix::WebSocketMessagePtr& msg)
{
    switch (msg->type)
    {
    case MsgType::Message:
        gateway_event_handler(msg);
        break;
    case MsgType::Open:
        printf("Discord ws connection opened\n");
        break;
    case MsgType::Close:
        printf("Discord ws connection closed\n");
        break;
    case MsgType::Error:
        printf("Discord ws error: %s\n", msg->errorInfo.reason.c_str());
        break;
    }
}
