#include "guild_here.h"

std::string narrow(const std::u32string& s)
{
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
	return conv.to_bytes(s);
}

std::u32string widen(const std::string& s)
{
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
	return conv.from_bytes(s);
}

std::string format_emoji(const nlohmann::json& j)
{
	bool anim = false;
	aegis::snowflake id;
	std::string name;

	if (j.count("animated") && j["animated"].is_boolean()) anim = j["animated"].get<bool>();
	if (j.count("id") && !j["id"].is_null()) id = j["id"];
	if (j.count("name") && j["name"].is_string()) name = j["name"].get<std::string>();

	return id ? fmt::format("<{}:{}:{}>", anim ? "a" : "", name, id) : name;
}

std::string easy_simple_emoji(const std::string& in)
{
	std::string out;
	for (auto& i : in) {
		char temp[5];
		sprintf_s(temp, "%c%X", '%', (unsigned char)(i + sizeof(char) / 2));
		out += temp;
	}
	return out;
}

unsigned long long stdstoulla(std::string str) {
	while (str.length() > 0) if (!std::isdigit(str[0])) str.erase(str.begin()); else break;
	if (str.length() == 0) return 0;
	unsigned long long t;
	if (sscanf_s(str.c_str(), "%llu", &t) == 1) {
		return t;
	}
	return 0;
}

/*
nlohmann::json create_message(aegis::ratelimit::ratelimit_mgr& ratelimit, const aegis::snowflake& chid, const std::string& content)
{
	if (!chid || content.empty()) return nlohmann::json();

	if (content.length() > max_message_len) {
		auto widenn = widen(content); // utf8 should become one.

		const size_t min_parts = 1 + (content.length() - 1) / max_message_len;
		const size_t each_slice_len = widenn.length() / min_parts;

		nlohmann::json last_msg;

		for (size_t p = 0; p * each_slice_len < widenn.length(); p++) {
			auto resnow = narrow(widenn.substr(p * each_slice_len, each_slice_len)); // back utf8
			last_msg = create_message(ratelimit, chid, resnow);
		}
	}

	nlohmann::json obj;
	obj["content"] = content;

	std::string _endpoint = fmt::format("/channels/{}/messages", chid);

	aegis::rest::request_params params = { _endpoint, aegis::rest::Post, obj.dump(-1, ' ', true) };
	auto& bkt = ratelimit.get_bucket(params.path);
	aegis::rest::rest_reply res = bkt.perform(params);

	return res.content;
}
*/
void time_iso::input(const std::string& iso) // format: 2020-09-23T18:15:46.827642+00:00
{
	if (iso.empty()) throw std::exception("null iso time format");
	sscanf_s(iso.c_str(), "%u-%u-%uT%u:%u:%f", &year, &month, &day, &hour, &minute, &second);
	iso_format = iso;
}

void time_iso::input_epoch_ms(const int64_t dt)
{
	time_t temp = dt / 1e3;
	tm tn;
	if (errno_t err = _gmtime64_s(&tn, &temp); err) {
		throw std::exception(("time_t can't be set, errno = " + std::to_string(err)).c_str());
	}
	year = tn.tm_year + 1900;
	month = tn.tm_mon + 1;
	day = tn.tm_mday;
	hour = tn.tm_hour;
	minute = tn.tm_min;
	second = (float)(tn.tm_sec) + (float)((dt % 1000) * 1e-3f);
}

void time_iso::input_epoch_s(const int64_t t)
{
	input_epoch_ms(t * 1000ull);
}

std::string time_iso::nice_format() const
{
	char buf[64];
	sprintf_s(buf, "%04u/%02u/%02u %02u:%02u:%06.1f", year, month, day, hour, minute, second);
	return buf;
}

GuildConf::GuildConf(GuildConf&& c)
{
	std::unique_lock<std::mutex> l1(bufferring_hold, std::defer_lock);
	std::unique_lock<std::mutex> l2(c.bufferring_hold, std::defer_lock);
	std::lock(l1, l2);

	buffering = std::move(c.buffering);
	last = c.last;
	chat = c.chat;
	last_user = c.last_user;
	format = c.format;
}

void GuildConf::start(const GuildConf& c)
{
	last = c.last;
	chat = c.chat;
	last_user = c.last_user;
	format = c.format;
}

nlohmann::json GuildConf::save() const
{
	nlohmann::json j;
	j["chat"] = chat;
	j["last_user"] = last_user;
	j["format"] = format;

	for(const auto& i : no_see)
		j["no_see_chats"].push_back(i);
	
	return j;
}

void GuildConf::load(const nlohmann::json& j)
{
	if (j.count("chat") && !j["chat"].is_null()) chat = j["chat"];
	if (j.count("last_user") && !j["last_user"].is_null()) last_user = j["last_user"];
	if (j.count("format") && !j["format"].is_null()) format = j["format"];

	if (j.count("no_see_chats") && !j["no_see_chats"].is_null()) {
		for (const auto& _field : j["no_see_chats"])
			no_see.push_back(_field);
	}
}

void GuildConf::push(const std::string& newline, aegis::channel* const chsend)
{
	std::lock_guard<std::mutex> l(bufferring_hold);

	auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	if (now - last > max_t_guild || buffering.size() > max_siz_guild)
	{
		std::string result;
		for (auto& i : buffering) result += i + '\n';
		result += newline;


		if (result.length() > max_message_len) {
			auto widenn = widen(result); // utf8 should become one.

			const size_t min_parts = 1 + (result.length() - 1) / max_message_len;
			const size_t each_slice_len = widenn.length() / min_parts;

			for (size_t p = 0; p * each_slice_len < widenn.length(); p++) {
				auto resnow = narrow(widenn.substr(p * each_slice_len, each_slice_len)); // back utf8
				chsend->create_message(resnow);
			}
		}
		else {
			chsend->create_message(result);
		}

		buffering.clear();
		last = now;
	}
	else {
		buffering.push_back(newline);
	}
}

bool Control::save_nolock(const aegis::snowflake& gid, const GuildConf& gg)
{
	if (!gid) throw std::exception("NULL guild id!");
	CreateDirectoryA("servers", NULL);

	auto cpy = gg.save().dump();
	if (cpy.empty()) {
		std::cout << "Guild #" << gid << " had null/empty config. Not saving." << std::endl;
		return true; // no config also means no one has set a config, so no error.
	}

	std::unique_ptr<FILE, std::function<void(FILE*)>> fp = std::move(std::unique_ptr<FILE, std::function<void(FILE*)>>([&] {
		std::string p = "servers/" + std::to_string(gid) + ".sn1";
		FILE* f;
		fopen_s(&f, p.c_str(), "wb");
		return f;
		}(), [](FILE* f) {fclose(f); }));

	if (!fp.get()) return false;

	fwrite(cpy.c_str(), sizeof(char), cpy.length(), fp.get());
	fp.reset();
	return true;
}

GuildConf Control::load_nolock(const aegis::snowflake& gid)
{
	if (!gid) throw std::exception("NULL guild id!");
	CreateDirectoryA("servers", NULL);

	std::unique_ptr<FILE, std::function<void(FILE*)>> fp = std::move(std::unique_ptr<FILE, std::function<void(FILE*)>>([&]{
		std::string p = "servers/" + std::to_string(gid) + ".sn1";
		FILE* f;
		fopen_s(&f, p.c_str(), "rb");
		return f;
		}(), [](FILE* f) {fclose(f); }));

	if (!fp.get()) return GuildConf{};

	std::string all_buf;
	char quickbuf[256];
	size_t u = 0;
	while (u = fread_s(quickbuf, 256, sizeof(char), 256, fp.get())) {
		for (size_t k = 0; k < u; k++) all_buf += quickbuf[k];
	}

	nlohmann::json j = nlohmann::json::parse(all_buf);

	GuildConf gg;
	gg.load(j);
	return std::move(gg);
}
/*
CustomMember& Control::get_member(const aegis::snowflake& id, const std::string& nusr, const std::string& ndis)
{
    std::lock_guard<std::mutex> l(access_members);
    auto& ref = known_members[id];
    if (!nusr.empty() && !ndis.empty()) {
        ref.username = nusr;
        ref.discriminator = ndis;
    }
    return ref;
}

CustomChannel& Control::get_channel(const aegis::snowflake& id, const aegis::snowflake& guild, const std::string& nchi)
{
	if (!guild) throw std::exception("invalid guild!");
	std::lock_guard<std::mutex> l(access_channels);
	auto& ref = known_channels[id];
	ref.guild = guild;
	if (!nchi.empty()) {
		ref.name = nchi;
	}
	return ref;
}
*/
GuildConf& Control::grab_guild(const aegis::snowflake& guild)
{
	if (!guild) throw std::exception("invalid guild!");
	std::lock_guard<std::mutex> l(access_guilds);

	auto a = known_guilds.find(guild);
	if (a != known_guilds.end()) {
		return a->second;
	}

	auto& ref = known_guilds[guild];
	ref.start(load_nolock(guild));
	return ref;
}
void Control::set_guild_chat(const aegis::snowflake& guild, const aegis::snowflake& nchat)
{
	if (!guild) throw std::exception("invalid guild!");
	std::lock_guard<std::mutex> l(access_guilds);

	auto a = known_guilds.find(guild);
	if (a != known_guilds.end()) {

		a->second.chat = nchat;
		save_nolock(guild, a->second);
	}
}
bool Control::toggle_chat_see(const aegis::snowflake& guild, const aegis::snowflake& nchat)
{
	if (!guild) throw std::exception("invalid guild!");
	std::lock_guard<std::mutex> l(access_guilds);

	auto a = known_guilds.find(guild);
	if (a != known_guilds.end()) {

		auto& k = a->second;
		bool seeing = true;

		for (size_t p = 0; p < k.no_see.size(); p++) {
			if (k.no_see[p] == nchat) {
				k.no_see.erase(k.no_see.begin() + p);
				seeing = false;
				break;
			}
		}
		if (seeing) k.no_see.push_back(nchat);

		save_nolock(guild, a->second);
		return seeing;
	}
	return false;
}
bool Control::check_can_see(const aegis::snowflake& guild, const aegis::snowflake& nchat) const
{
	if (!guild) throw std::exception("invalid guild!");
	std::lock_guard<std::mutex> l(access_guilds);

	auto a = known_guilds.find(guild);
	if (a != known_guilds.end()) {
		for (const auto& i : a->second.no_see) {
			if (i == nchat) return false;
		}
		return true;
	}
	return false;
}
void Control::flush(const aegis::snowflake& guild)
{
	if (!guild) throw std::exception("invalid guild!");
	std::lock_guard<std::mutex> l(access_guilds);

	auto a = known_guilds.find(guild);
	if (a != known_guilds.end()) {
		save_nolock(guild, a->second);
	}
}
/*
void Control::remove_channel(const aegis::snowflake& id)
{
	std::lock_guard<std::mutex> l(access_channels);
	known_channels.erase(id);
}


void Control::free_guild_channels(const aegis::snowflake& guild)
{
	std::lock_guard<std::mutex> l(access_channels);
	for (auto it = known_channels.begin(); it != known_channels.end(); )
	{
		if (it->second.guild == guild) { it = known_channels.erase(it); }
		else { ++it; }
	}
}
*/
void Control::flush()
{
	std::lock_guard<std::mutex> l(access_guilds);
	for (auto& g : known_guilds)
	{
		save_nolock(g.first, g.second);
	}
}
