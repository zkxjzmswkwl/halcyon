#include "discord.h"
#include "globals.h"
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
        cpr::Post(cpr::Url{"https://discord.com/api/v9/auth/login"}, cpr::Header{{"content-type", "application/json"}},
                  cpr::Header{{"User-Agent", USER_AGENT_VAL}}, cpr::Timeout{10000}, cpr::Body{body});

    rapidjson::Document doc;
    doc.Parse(r.text.c_str());

    if (r.status_code != 200)
        return "";

    if (doc["token"].IsString())
        return doc["token"].GetString();

    return "";
}
//---------------------------------------------------------------------------
bool fetch_data()
{
    printf(g_ctx->token.c_str());
    printf("\n");

    if (!get_servers() || !get_friends())
    {
        printf("ERR: Could not get servers or friends.\n");
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------
bool get_servers()
{
    cpr::Response r =
        cpr::Get(cpr::Url{"https://discord.com/api/v9/users/@me/guilds"}, cpr::Header{{"Authorization", g_ctx->token}},
                 cpr::Header{{"User-Agent", USER_AGENT_VAL}}, cpr::Timeout{10000});

    rapidjson::Document doc;
    doc.Parse(r.text.c_str());

    assert(r.status_code == 200);
    assert(doc.IsArray());

    for (unsigned int i = 0; i < doc.Size(); ++i)
    {
        const rapidjson::Value& item = doc[i];
        Server* newServer = new Server(item["name"].GetString(), item["id"].GetString());
        g_ctx->servers[i] = newServer;
        g_ctx->server_count++;
        if (!get_channels_for_server(newServer))
            printf("ERR: Could not retrieve channel listing for server %s\n", newServer->name.c_str());
    }

    return true;
}
//---------------------------------------------------------------------------
bool get_channels_for_server(Server* server)
{
    cpr::Response r = cpr::Get(cpr::Url{"https://discord.com/api/v9/guilds/" + server->id + "/channels"},
                               cpr::Header{{"Authorization", g_ctx->token}}, cpr::Header{{"User-Agent", USER_AGENT_VAL}},
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
bool get_friends()
{
    if (!g_ctx->friends.empty())
    {
        for (Friend* friendPtr : g_ctx->friends)
            delete friendPtr;

        g_ctx->friends.clear();
    }

    cpr::Response r = cpr::Get(cpr::Url{"https://discord.com/api/v9/users/@me/relationships"},
                               cpr::Header{{"Authorization", g_ctx->token}}, cpr::Header{{"User-Agent", USER_AGENT_VAL}},
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

        g_ctx->friends.push_back(new Friend(id, name));
    }

    return true;
}
//---------------------------------------------------------------------------
std::string get_chat_id_from_user(std::string user_id)
{
    std::string body("{\"recipient_id\": " + user_id + "}");
    cpr::Response r = cpr::Post(cpr::Url{"https://discord.com/api/v9/users/@me/channels"},
                                cpr::Header{{"content-type", "application/json"}},
                                cpr::Header{{"Authorization", g_ctx->token}}, cpr::Timeout{10000}, cpr::Body{body});

    if (r.status_code != 200)
        return "";

    rapidjson::Document doc;
    doc.Parse(r.text.c_str());

    return doc["id"].GetString();
}
//---------------------------------------------------------------------------
bool get_focused_channel_content()
{
    g_ctx->focused_channel->messages.clear();

    cpr::Response r = cpr::Get(
        cpr::Url{"https://discord.com/api/v9/channels/" + g_ctx->focused_channel->id + "/messages?limit=10"},
        cpr::Header{{"Authorization", g_ctx->token}}, cpr::Header{{"User-Agent", USER_AGENT_VAL}}, cpr::Timeout{10000});

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

        g_ctx->focused_channel->messages.push_back(new Message(timestamp, author, body));
    }

    return true;
}
//---------------------------------------------------------------------------
bool post_message(std::string message, std::string channelId)
{
    std::string body("{\"content\": \"" + message + "\"}");
    cpr::Response r = cpr::Post(cpr::Url{"https://discord.com/api/v9/channels/" + channelId + "/messages"},
                                cpr::Header{{"content-type", "application/json"}},
                                cpr::Header{{"Authorization", g_ctx->token}}, cpr::Timeout{10000}, cpr::Body{body});

    return true;
}
//---------------------------------------------------------------------------
void display_servers()
{
    for (unsigned int i = 0; i < g_ctx->server_count; i++)
    {
        if (i == 3 || i == 5)
            continue;

        auto server = g_ctx->servers.at(i);
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
void display_friends()
{
    for (unsigned int i = 0; i < g_ctx->friends.size(); i++)
    {
        auto f = g_ctx->friends.at(i);
        printf("[%d]\t%s\n", i, f->username.c_str());
    }
}
//---------------------------------------------------------------------------
} // namespace discord
