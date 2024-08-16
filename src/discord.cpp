#include "discord.h"
#include <cpr/cpr.h>
#include <rapidjson/document.h>

namespace discord
{
//---------------------------------------------------------------------------
std::string post_login(std::string username, std::string password)
{
    std::string body(R"({
      "login": ")" + username +
                     R"(",
      "password": ")" +
                     password + R"(",
      "undelete": false,
      "login_source": null,
      "gift_code_sku_id": null
    })");

    cpr::Response r =
        cpr::Post(cpr::Url{"https://discord.com/api/v9/auth/login"},
                  cpr::Header{{"content-type", "application/json"}},
                  cpr::Header{{"User-Agent", USER_AGENT_VAL}},
                  cpr::Timeout{10000},
                  cpr::Body{body});

    rapidjson::Document doc;
    doc.Parse(r.text.c_str());

    if (r.status_code != 200)
        return "";

    if (doc["token"].IsString())
        return doc["token"].GetString();

    return "";
}
/*---------------------------------------------------------------------------
* Allocates `DiscordContext* ctx`
* Passes it to `get_servers(ctx)`
* `get_servers(ctx)` allocates one `Server*` for each server user is in,
* which are added to `ctx->servers`.
*
* For each server user is in, calls `get_channels_for_server(ctx, Server*)`,
* which allocates one `Channel*` for each channel in the given server.
* Which are then pushed to `server->channels`.
---------------------------------------------------------------------------*/
DiscordContext* get_context(std::string token)
{
    DiscordContext* ctx = new DiscordContext(token);

    if (!get_servers(ctx))
        printf("ERR: Could not get servers.\n");

    if (!get_friends(ctx))
        printf("ERR: Could not get friends.\n");

    return ctx;
}
//---------------------------------------------------------------------------
bool get_servers(DiscordContext* ctx)
{
    cpr::Response r =
        cpr::Get(cpr::Url{"https://discord.com/api/v9/users/@me/guilds"},
                 cpr::Header{{"Authorization", ctx->token}},
                 cpr::Header{{"User-Agent", USER_AGENT_VAL}},
                 cpr::Timeout{10000});

    rapidjson::Document doc;
    doc.Parse(r.text.c_str());

    assert(r.status_code == 200);
    assert(doc.IsArray());

    for (unsigned int i = 0; i < doc.Size(); ++i)
    {
        const rapidjson::Value& item = doc[i];
        Server* newServer = new Server(item["name"].GetString(), item["id"].GetString());
        ctx->servers[i] = newServer;
        ctx->server_count++;
        if (!get_channels_for_server(ctx, newServer))
            printf("ERR: Could not retrieve channel listing for server %s\n", newServer->name.c_str());
    }

    return true;
}
//---------------------------------------------------------------------------
bool get_channels_for_server(DiscordContext* ctx, Server* server)
{
    cpr::Response r = cpr::Get(cpr::Url{"https://discord.com/api/v9/guilds/" + server->id + "/channels"},
                               cpr::Header{{"Authorization", ctx->token}},
                               cpr::Header{{"User-Agent", USER_AGENT_VAL}},
                               cpr::Timeout{10000});

    rapidjson::Document doc;
    doc.Parse(r.text.c_str());

    if (!doc.IsArray())
        return false;

    for (unsigned int i = 0; i < doc.Size(); ++i)
    {
        const rapidjson::Value& item = doc[i];

        auto channel_type = (ChannelType)item["type"].GetUint();
        auto name = item["name"].GetString();
        auto id = item["id"].GetString();
        server->channels[i] = new Channel(channel_type, name, id);
        server->channel_count++;
    }

    return true;
}
//---------------------------------------------------------------------------
bool get_friends(DiscordContext* ctx)
{
    if (!ctx->friends.empty())
    {
        for (Friend* friendPtr : ctx->friends)
            delete friendPtr;

        ctx->friends.clear();
    }

    cpr::Response r = cpr::Get(cpr::Url{"https://discord.com/api/v9/users/@me/relationships"},
                               cpr::Header{{"Authorization", ctx->token}},
                               cpr::Header{{"User-Agent", USER_AGENT_VAL}},
                               cpr::Timeout{10000});

    if (r.status_code != 200)
        return false;

    rapidjson::Document doc;
    doc.Parse(r.text.c_str());

    if (!doc.IsArray())
        return false;

    for (unsigned int i = 0; i < doc.Size(); i++)
    {
        const rapidjson::Value& item = doc[i];

        auto user_obj = item["user"].GetObj();
        auto name = user_obj["username"].GetString();
        auto id = item["id"].GetString();

        ctx->friends.push_back(new Friend(id, name));
    }

    return true;
}
//---------------------------------------------------------------------------
std::string get_chat_id_from_user(DiscordContext* ctx, std::string user_id)
{
    std::string body("{\"recipient_id\": " + user_id + "}");
    cpr::Response r = cpr::Post(cpr::Url{"https://discord.com/api/v9/users/@me/channels"},
                                cpr::Header{{"content-type", "application/json"}},
                                cpr::Header{{"Authorization", ctx->token}},
                                cpr::Timeout{10000}, cpr::Body{body});

    if (r.status_code != 200)
        return "";

    rapidjson::Document doc;
    doc.Parse(r.text.c_str());

    return doc["id"].GetString();
}
//---------------------------------------------------------------------------
bool get_focused_channel_content(DiscordContext* ctx)
{
    ctx->focused_channel->messages.fill(nullptr);
    ctx->focused_channel->message_count = 0;

    cpr::Response r = cpr::Get(cpr::Url{"https://discord.com/api/v9/channels/" + ctx->focused_channel->id + "/messages?limit=10"},
                               cpr::Header{{"Authorization", ctx->token}},
                               cpr::Header{{"User-Agent", USER_AGENT_VAL}},
                               cpr::Timeout{10000});

    if (r.status_code != 200)
        return false;

    rapidjson::Document doc;
    doc.Parse(r.text.c_str());

    if (!doc.IsArray())
        return false;

    for (unsigned int i = doc.Size(); i-- > 0;)
    {
        const rapidjson::Value& item = doc[i];

        auto timestamp = item["timestamp"].GetString();
        auto author = item["author"].GetObj()["global_name"].GetString();
        auto body = item["content"].GetString();

        ctx->focused_channel->messages[i] = new Message(timestamp, author, body);
        ctx->focused_channel->message_count++;
    }

    return true;
}
//---------------------------------------------------------------------------
bool post_message(DiscordContext* ctx, std::string message, std::string channelId)
{
    std::string body("{\"content\": \"" + message + "\"}");
    cpr::Response r = cpr::Post(cpr::Url{"https://discord.com/api/v9/channels/" + channelId + "/messages"},
                                cpr::Header{{"content-type", "application/json"}},
                                cpr::Header{{"Authorization", ctx->token}},
                                cpr::Timeout{10000},
                                cpr::Body{body});

    return true;
}
//---------------------------------------------------------------------------
void display_servers(DiscordContext* ctx)
{
    for (unsigned int i = 0; i < ctx->server_count; i++)
    {
        auto server = ctx->servers.at(i);
        if (server == nullptr)
            break;

        printf("[%d]\t%s\n", i, server->name.c_str());
    }
}
//---------------------------------------------------------------------------
void display_channels_for_server(Server* server, ChannelType channel_type)
{
    printf("Listing channels of %s\n", server->name.c_str());
    for (unsigned int i = 0; i < server->channel_count; i++)
    {
        Channel* channel = server->channels.at(i);
        // note: `channel->channel_type` seems silly. `type` is reserved, though.
        // maybe `designation`?
        if (channel->channel_type != channel_type)
            continue;

        printf("[%d]\t%s\n", i, channel->name.c_str());
    }
}
//---------------------------------------------------------------------------
void display_friends(DiscordContext* ctx)
{
    for (unsigned int i = 0; i < ctx->friends.size(); i++)
    {
        auto f = ctx->friends.at(i);
        printf("[%d]\t%s\n", i, f->username.c_str());
    }
}
//---------------------------------------------------------------------------
} // namespace discord
