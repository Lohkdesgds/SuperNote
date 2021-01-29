#include <aegis.hpp>
#include <Windows.h>

#include "guild_here.h"
#include "keys.h"

bool up = true;

void save_all(int u = 0) {
    std::cout << "Saving..." << std::endl;
    up = false;
    global_control.flush();
    std::cout << "Saved all configuration." << std::endl;
}

BOOL onConsoleEvent(DWORD event) {

    switch (event) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case WM_CLOSE:
        save_all();
        break;
    }
    return 0;
}

const aegis::snowflake meedev           = 280450898885607425;

const std::string default_cmd           = u8"lsw/sn";
const std::string version_app           = u8"V1.0.2";

const std::string new_message           = u8"📃";
const std::string edit_message          = u8"📝";
const std::string react_message         = u8"🎯";
const std::string dereact_message       = u8"🧤";
const std::string joined_message        = u8"🔺";
const std::string left_message          = u8"🔻";

const std::string emoji_yes             = u8"✅";
const std::string emoji_no              = u8"❎";

const std::string emoji_seeing          = u8"👀";
const std::string emoji_no_see          = u8"🙈";
//const std::string emoji_no_perm         = u8"🈲";


int main(int argc, char* argv[])
{
    signal(SIGABRT, save_all);
    signal(SIGTERM, save_all);

    if (!SetConsoleCtrlHandler(onConsoleEvent, TRUE)) {
        std::cout << "Something went wrong when trying to setup close handling..." << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 1;
    }

    //std::vector<Guild> gds;
    std::mutex gds_m;

    try
    {
        // Create bot object
        aegis::core bot(aegis::create_bot_t().log_level(spdlog::level::trace).token(key));

        bot.set_on_message_create_raw([&](nlohmann::json j, aegis::shards::shard* _shard) {
            if (!up) return;

            auto& data = j["d"];

            if (!data.count("author") || data["author"].is_null()) return; // non user

            aegis::snowflake guild = data["guild_id"];
            aegis::snowflake channel = data["channel_id"];
            aegis::snowflake message = data["id"];
            auto& author = data["author"];
            std::string content = j["d"]["content"];

            // check cmd

            auto& guild_conf = global_control.grab_guild(guild);

            aegis::snowflake who = author["id"];


            aegis::channel* ch_a = bot.find_channel(channel);
            if (!ch_a) {
                bot.log->error("Cannot find what channel event came from in guild {}", guild);
                return;
            }


            aegis::guild* guild_a = bot.find_guild(guild);
            if (guild_a && content.find(default_cmd + " ") == 0) {
                if (guild_a->get_owner() == who || who == meedev) {
                    auto argg = content.substr(default_cmd.length());
                    aegis::snowflake newid = stdstoulla(argg);

                    if (argg.find(" ?") == 0 || argg.find(" help") == 0)
                    {
                        ch_a->create_message(
                            u8"`" + default_cmd + u8" <id>`: set a chat as log chat\n"
                            u8"`" + default_cmd + u8" ?`: shows this\n"
                            u8"`" + default_cmd + u8" help`: shows this\n"
                            u8"`" + default_cmd + u8" simple`: enable simple logging\n"
                            u8"`" + default_cmd + u8" complex`: enable complex logging\n"
                            u8"`" + default_cmd + u8" ignore`: enable/disable this chat logging"
                        );
                    }
                    if (argg.find(" simple") == 0)
                    {
                        guild_conf.format = SIMPLISTIC;
                        global_control.flush(guild);
                        ch_a->create_reaction(message, easy_simple_emoji(emoji_yes));
                    }
                    else if (argg.find(" restart") == 0 && who == meedev)
                    {
                        ch_a->create_reaction(message, easy_simple_emoji(emoji_yes));
                        bot.update_presence(default_cmd + " - restarting...", aegis::gateway::objects::activity::Game, aegis::gateway::objects::presence::Idle);
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        up = false;
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        bot.shutdown();
                        save_all();
                        return;
                    }
                    else if (argg.find(" complex") == 0)
                    {
                        guild_conf.format = MEMBER_RULES;
                        global_control.flush(guild);
                        ch_a->create_reaction(message, easy_simple_emoji(emoji_yes));
                    }
                    else if (argg.find(" ignore") == 0)
                    {
                        if (global_control.toggle_chat_see(guild, channel)) {
                            ch_a->create_reaction(message, easy_simple_emoji(emoji_seeing));
                        }
                        else {
                            ch_a->create_reaction(message, easy_simple_emoji(emoji_no_see));
                        }
                    }
                    else if (newid)
                    {
                        global_control.set_guild_chat(guild, newid);
                        bot.log->info("Guild {} has set its chat to {}.", guild, newid);
                        ch_a->create_reaction(message, easy_simple_emoji(emoji_yes));
                    }
                    else
                    {
                        global_control.set_guild_chat(guild, 0);
                        bot.log->info("Guild {} removed chat logging.", guild);
                        ch_a->create_reaction(message, easy_simple_emoji(emoji_yes));
                    }
                }
                else {
                    ch_a->create_reaction(message, easy_simple_emoji(emoji_no));
                }
            }

            if (!guild_conf.chat) return;

            if (!global_control.check_can_see(guild, channel)) return;

            if (author.count("bot") && author["bot"].is_boolean() && author["bot"].get<bool>()) return; // bot
            if ((author.count("discriminator") && !author["discriminator"].is_string())) return; // webhook

            time_iso timed;
            timed.input(data["timestamp"].get<std::string>());

            aegis::channel* output = bot.find_channel(guild_conf.chat);
            if (!output) {
                bot.log->error("Cannot find channel to send in guild {}", guild);
                return;
            }


            std::string endd;

            switch (guild_conf.format) {
            case SIMPLISTIC:
                endd =
                    "`[" + new_message + "][" + timed.nice_format() + "]"
                    "(" + std::to_string(ch_a->get_id()) + u8"¦" +
                    author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>() + "):` " + content;
                break;
            case MEMBER_RULES:
                if (who != guild_conf.last_user) endd = "```md\n[" + std::to_string(who) + "](" + author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>() + ")```";
                guild_conf.last_user = who;
                endd += "`[" + new_message + "][" + timed.nice_format() + "]"
                    "(" + std::to_string(ch_a->get_id()) + u8"):` " + content;
                break;
            }

            if (output) guild_conf.push(endd, output);
        });
        bot.set_on_message_update_raw([&](nlohmann::json j, aegis::shards::shard* _shard) {
            if (!up) return;

            auto& data = j["d"];

            if (!data.count("author") || data["author"].is_null()) return; // non user

            aegis::snowflake guild = data["guild_id"];
            aegis::snowflake channel = data["channel_id"];
            aegis::snowflake message = data["id"];
            auto& author = data["author"];

            auto& guild_conf = global_control.grab_guild(guild);
            if (!guild_conf.chat) return;

            aegis::snowflake who = author["id"];

            if (author.count("bot") && author["bot"].is_boolean() && author["bot"].get<bool>()) return; // bot
            if (author.count("discriminator") && !author["discriminator"].is_string()) return; // webhook


            time_iso timed;
            if (data.count("edited_timestamp") && !data["edited_timestamp"].is_null()) timed.input(data["edited_timestamp"].get<std::string>());
            else timed.input(data["timestamp"].get<std::string>());

            // Discriminator can be null if webhook!
            //auto& memb = global_control.get_member(who, author["username"].get<std::string>(), author["discriminator"].get<std::string>());

            aegis::channel* ch_a = bot.find_channel(channel);
            if (!ch_a) {
                bot.log->error("Cannot find what channel event came from in guild {}", guild);
                return;
            }
            aegis::channel* output = bot.find_channel(guild_conf.chat);
            if (!output) {
                bot.log->error("Cannot find channel to send in guild {}", guild);
                return;
            }

            std::string content = j["d"]["content"];

            std::string endd;

            switch (guild_conf.format) {
            case SIMPLISTIC:
                endd =
                    "`[" + edit_message + "][" + timed.nice_format() + "]"
                    "(" + std::to_string(ch_a->get_id()) + u8"¦" +
                    author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>() + "):` " + content;
                break;
            case MEMBER_RULES:
                if (who != guild_conf.last_user) endd = "```md\n[" + std::to_string(who) + "](" + author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>() + ")```";
                guild_conf.last_user = who;
                endd += "`[" + edit_message + "][" + timed.nice_format() + "]"
                    "(" + std::to_string(ch_a->get_id()) + u8"):` " + content;
                break;
            }



            if (output) guild_conf.push(endd, output);
        });
        bot.set_on_message_reaction_add_raw([&](nlohmann::json j, aegis::shards::shard* _shard) {
            if (!up) return;

            auto& data = j["d"];

            if (!(data.count("guild_id") && !data["guild_id"].is_null())) return; // not from guild
            if (!(data.count("member") && !data["member"].is_null())) return; // not from member?

            aegis::snowflake guild = data["guild_id"];
            aegis::snowflake channel = data["channel_id"];
            aegis::snowflake message = data["id"];
            auto& member = data["member"];
            auto& author = member["user"];
            auto& emoji = data["emoji"];

            auto& guild_conf = global_control.grab_guild(guild);

            if (!guild_conf.chat) return;
            aegis::snowflake who = author["id"]; // user doesn't exist in message_create and message_update

            std::string translated_emoji = format_emoji(emoji);

            if (author.count("bot") && author["bot"].is_boolean() && author["bot"].get<bool>()) return; // bot
            if ((author.count("discriminator") && !author["discriminator"].is_string())) return; // webhook


            time_iso timed;
            timed.input_epoch_ms(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

            // Discriminator can be null if webhook!
            //auto& memb = global_control.get_member(who, author["username"].get<std::string>(), author["discriminator"].get<std::string>());
            //auto& ch = global_control.get_channel(channel, guild);

            aegis::channel* ch_a = bot.find_channel(channel);
            if (!ch_a) {
                bot.log->error("Cannot find what channel event came from in guild {}", guild);
                return;
            }
            aegis::channel* output = bot.find_channel(guild_conf.chat);
            if (!output) {
                bot.log->error("Cannot find channel to send in guild {}", guild);
                return;
            }

            std::string endd;

            switch (guild_conf.format) {
            case SIMPLISTIC:
                endd = "`[" + react_message + "][" + timed.nice_format() + "]"
                    "(" + std::to_string(ch_a->get_id()) + u8"¦" +
                    author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>() + "):` " + translated_emoji;
                break;
            case MEMBER_RULES:
                if (who != guild_conf.last_user) endd = "```md\n[" + std::to_string(who) + "](" + author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>() + ")```";
                guild_conf.last_user = who;
                endd += "`[" + react_message + "][" + timed.nice_format() + "]"
                    "(" + std::to_string(ch_a->get_id()) + u8"):` " + translated_emoji;
                break;
            }


            if (output) guild_conf.push(endd, output);
        });
        bot.set_on_guild_member_add_raw([&](nlohmann::json j, aegis::shards::shard* _shard) {
            if (!up) return;

            auto& data = j["d"];

            if (!(data.count("guild_id") && !data["guild_id"].is_null())) return; // not from guild
            if (!(data.count("user") && !data["user"].is_null())) return; // not from member?

            aegis::snowflake guild = data["guild_id"];
            auto& member = data;
            auto& author = member["user"];

            auto& guild_conf = global_control.grab_guild(guild);

            if (!guild_conf.chat) return;
            aegis::snowflake who = author["id"]; // user doesn't exist in message_create and message_update

            if (author.count("bot") && author["bot"].is_boolean() && author["bot"].get<bool>()) return; // bot
            if ((author.count("discriminator") && !author["discriminator"].is_string())) return; // webhook


            time_iso timed;
            timed.input(member["joined_at"].get<std::string>());

            aegis::channel* output = bot.find_channel(guild_conf.chat);
            if (!output) {
                bot.log->error("Cannot find channel to send in guild {}", guild);
                return;
            }

            std::string endd;

            switch (guild_conf.format) {
            case SIMPLISTIC:
                endd = "`[" + joined_message + "][" + timed.nice_format() + "]> " +
                    author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>() + "`";
                break;
            case MEMBER_RULES:
                if (who != guild_conf.last_user) endd = "```md\n[" + std::to_string(who) + "](" + author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>() + ")```";
                guild_conf.last_user = who;
                endd += "`[" + joined_message + "][" + timed.nice_format() + "]`";
                break;
            }


            if (output) guild_conf.push(endd, output);
        });
        bot.set_on_guild_member_remove_raw([&](nlohmann::json j, aegis::shards::shard* _shard) {
            if (!up) return;

            auto& data = j["d"];

            if (!(data.count("guild_id") && !data["guild_id"].is_null())) return; // not from guild
            if (!(data.count("user") && !data["user"].is_null())) return; // not from member?

            aegis::snowflake guild = data["guild_id"];
            auto& member = data;
            auto& author = member["user"];

            auto& guild_conf = global_control.grab_guild(guild);

            if (!guild_conf.chat) return;
            aegis::snowflake who = author["id"]; // user doesn't exist in message_create and message_update

            if (author.count("bot") && author["bot"].is_boolean() && author["bot"].get<bool>()) return; // bot
            if ((author.count("discriminator") && !author["discriminator"].is_string())) return; // webhook


            time_iso timed;
            timed.input_epoch_ms(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

            aegis::channel* output = bot.find_channel(guild_conf.chat);
            if (!output) {
                bot.log->error("Cannot find channel to send in guild {}", guild);
                return;
            }

            std::string endd;

            switch (guild_conf.format) {
            case SIMPLISTIC:
                endd = "`[" + left_message + "][" + timed.nice_format() + "]> " +
                    author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>() + "`";
                break;
            case MEMBER_RULES:
                if (who != guild_conf.last_user) endd = "```md\n[" + std::to_string(who) + "](" + author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>() + ")```";
                guild_conf.last_user = who;
                endd += "`[" + left_message + "][" + timed.nice_format() + "]`";
                break;
            }


            if (output) guild_conf.push(endd, output);
        });

        // start the bot
        bot.run();

        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::thread thr([&] {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            while (up) {
                bot.update_presence(default_cmd + " - " + version_app, aegis::gateway::objects::activity::Game, aegis::gateway::objects::presence::Online);
                for (size_t cc = 0; cc < 250 && up; cc++) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    std::this_thread::yield();
                }
            }
            std::cout << "Got close signal. Ending..." << std::endl;

            bot.shutdown();
        });

        // yield thread execution to the library
        bot.yield();

        up = false;
        thr.join();
    }
    catch (const std::exception& e)
    {
        std::cout << "Error during initialization: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cout << "Error during initialization: uncaught" << std::endl;
        return 1;
    }
    return 0;
}
