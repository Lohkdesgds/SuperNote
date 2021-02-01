#pragma once

#include <aegis.hpp>
#include <codecvt>

constexpr size_t max_message_len = 1990;
constexpr std::chrono::milliseconds max_t_guild = std::chrono::milliseconds{1300};
constexpr size_t max_siz_guild = 10;

std::string narrow(const std::u32string&);
std::u32string widen(const std::string&);

std::string format_emoji(const nlohmann::json&);
//std::string easy_simple_emoji(const std::string&);

unsigned long long stdstoulla(std::string);

//nlohmann::json create_message(aegis::ratelimit::ratelimit_mgr&, const aegis::snowflake&, const std::string&);

enum guild_format {
	SIMPLISTIC,
	MEMBER_RULES
};


struct time_iso {// GMT0
	unsigned year;
	unsigned month;
	unsigned day;
	unsigned hour;
	unsigned minute;
	float second;
	std::string iso_format;

	void input(const std::string&); // format: 2020-09-23T18:15:46.827642+00:00
	void input_epoch_ms(const int64_t);
	void input_epoch_s(const int64_t);
	std::string nice_format() const;
};
class GuildConf {
	std::vector<std::string> buffering;
	std::chrono::milliseconds last{};
	std::mutex bufferring_hold;
public:
	GuildConf() = default;
	GuildConf(GuildConf&&);
	// for start only, does not copy buffer!
	void start(const GuildConf&);

	aegis::snowflake chat = 0;
	std::vector<aegis::snowflake> no_see;
	aegis::snowflake last_user = 0;
	int format = MEMBER_RULES;

	nlohmann::json save() const;
	void load(const nlohmann::json&);

	void push(const std::string&, aegis::channel* const);
};

class Control {
	std::unordered_map<aegis::snowflake, GuildConf> known_guilds;
	mutable std::mutex access_guilds;

	bool save_nolock(const aegis::snowflake&, const GuildConf&);
	GuildConf load_nolock(const aegis::snowflake&);
public:
	GuildConf& grab_guild(const aegis::snowflake&);

	// guild, chat. return if seeing right now.
	void set_guild_chat(const aegis::snowflake&, const aegis::snowflake&);

	// guild, chat
	bool toggle_chat_see(const aegis::snowflake&, const aegis::snowflake&);

	// true if can see.
	bool check_can_see(const aegis::snowflake&, const aegis::snowflake&) const;
	
	void flush(const aegis::snowflake&);
	//void flush();
};

inline Control global_control;


/*
struct CustomMember {
	std::string username,
				discriminator;
};
struct CustomChannel {
	std::string name;
	aegis::snowflake guild;
};*/






/*class Guild {

};*/