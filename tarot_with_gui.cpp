#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <array>
#include <chrono>
#include <string>
#include <memory>
#include <utility>
#include <stdexcept>
#include <cstdint>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <system_error>
#endif

static constexpr int32_t WINDOW_WIDTH = 1160;
static constexpr int32_t WINDOW_HEIGHT = 700;
static constexpr unsigned int FONT_SIZE = 44;

enum class Arcana {
    major,
    minor,
    all
};

enum class Type {
    the_fool,
    the_magician,
    the_high_priestess,
    the_empress,
    the_emperor,
    the_hierophant,
    the_lovers,
    the_chariot,
    strength,
    the_hermit,
    wheel_fortune,
    justice,
    the_hanged_man,
    death,
    temperance,
    the_devil,
    the_tower,
    the_star,
    the_moon,
    the_sun,
    judgement,
    the_world,

    cups_ace,
    cups_2,
    cups_3,
    cups_4,
    cups_5,
    cups_6,
    cups_7,
    cups_8,
    cups_9,
    cups_10,
    cups_page,
    cups_knight,
    cups_queen,
    cups_king,

    pentacles_ace,
    pentacles_2,
    pentacles_3,
    pentacles_4,
    pentacles_5,
    pentacles_6,
    pentacles_7,
    pentacles_8,
    pentacles_9,
    pentacles_10,
    pentacles_page,
    pentacles_knight,
    pentacles_queen,
    pentacles_king,

    wands_ace,
    wands_2,
    wands_3,
    wands_4,
    wands_5,
    wands_6,
    wands_7,
    wands_8,
    wands_9,
    wands_10,
    wands_page,
    wands_knight,
    wands_queen,
    wands_king,

    swords_ace,
    swords_2,
    swords_3,
    swords_4,
    swords_5,
    swords_6,
    swords_7,
    swords_8,
    swords_9,
    swords_10,
    swords_page,
    swords_knight,
    swords_queen,
    swords_king
};

enum class State {
    positive,
    negative
};

static std::array<std::string, 78> card_paths;
static std::array<std::wstring, 78> card_names;

class CardInfo {
    static void init_paths() {
        const std::string path = "res/paths.txt";
        std::ifstream in{ path };
        
        for (size_t i = 0; i < card_paths.size(); ++i) {
            std::getline(in, card_paths[i]);

            if (!in) {
                throw std::runtime_error{ "init_paths failed, file maybe broken: " + path };
            }
        }
    }

    #ifdef _WIN32
    static std::wstring utf8_to_unicode(const std::string& str) {
        if (str.length() == 0) {
            return L"";
        }

        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);

        if (len == 0) {
            DWORD errCode = GetLastError();
            throw std::system_error(errCode, std::system_category(), "utf8_to_unicode: sys call MultiByteToWideChar get length failed");
        }

        std::vector<wchar_t> buf(len);
        len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buf.data(), len);

        if (len == 0) {
            DWORD errCode = GetLastError();
            throw std::system_error(errCode, std::system_category(), "utf8_to_unicode: sys call MultiByteToWideChar get wstr failed");
        }

        return std::wstring{ buf.cbegin(), buf.cend() - 1 };   // exclude the tail '\0'.
    }
    #endif

    static void init_names() {
        const std::string path = "res/names.txt";
        std::ifstream in{ path };
        std::string line;
        
        for (size_t i = 0; i < card_paths.size(); ++i) {
            std::getline(in, line);

            if (!in) {
                throw std::runtime_error{ "init_paths failed, file maybe broken: " + path };
            }

            card_names[i] = utf8_to_unicode(line);
        }
    }
public:
    static void init() {
        init_paths();
        init_names();
    }

    static std::string path(Type type) noexcept {
        return card_paths[static_cast<int>(type)];
    }

    static std::wstring name(Type type) noexcept {
        return card_names[static_cast<int>(type)];
    }
};

struct Card {
    Type type;
    State state;

    Card(Type _type, State _state)
        : type{ _type }, state{ _state }
    {}
};

class Cards {
    static constexpr uint32_t arcana_major_num = 22;
    static constexpr uint32_t arcana_minor_num = 56;
    static constexpr uint32_t arcana_total_num = 78;
public:
    Cards() {
        set_arcana(Arcana::major);
    }

    std::vector<Card>::size_type length() const noexcept {
        return data.size();
    }

    Card& get(int32_t i) noexcept {
        return data[i];
    }

    const Card& get(int32_t i) const noexcept {
        return data[i];
    }

    void set_arcana(Arcana arcana) {
        if (arcana == Arcana::minor) {
            for (int32_t i = 0; i < arcana_minor_num; ++i) {
                data.emplace_back(static_cast<Type>(i + arcana_major_num), State::positive);
            }
        }
        else if (arcana == Arcana::all) {
            for (int i = 0; i < arcana_total_num; ++i) {
                data.emplace_back(static_cast<Type>(i), State::positive);
            }
        }
        else {
            for (int32_t i = 0; i < arcana_major_num; ++i) {
                data.emplace_back(static_cast<Type>(i), State::positive);
            }
        }

        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        mt = std::mt19937_64{ static_cast<uint64_t>(seed) };
        typeDist = std::uniform_int_distribution<int>(0, static_cast<int>(data.size()) - 1);
        stateDist = std::uniform_int_distribution<int>(0, 1);
    }

    void shuffle() noexcept {
        using szType = std::vector<Card>::size_type;

        // Fisher-yates algorithm.
        for (szType i = data.size() - 1; i > 0; --i) {
            int j = typeDist(mt);
            std::swap(data[i], data[j]);
        }

        // re-generate card states.
        for (szType i = 0; i < data.size(); ++i) {
            data[i].state = static_cast<State>(stateDist(mt));
        }
    }
private:
    std::vector<Card> data;
    std::mt19937_64 mt;
    std::uniform_int_distribution<int> typeDist;
    std::uniform_int_distribution<int> stateDist;
};

struct HolyTriangleResult {
    Card past;
    Card now;
    Card future;

    HolyTriangleResult(Card _past, Card _now, Card _future)
        : past{ _past }, now{ _now }, future{ _future }
    {}
};

HolyTriangleResult divine_holy_triangle(Cards& cards) noexcept {
    cards.shuffle();
    cards.shuffle();
    cards.shuffle();

    auto length = cards.length();

    return HolyTriangleResult {
        cards.get(length - 7),
        cards.get(length - 14),
        cards.get(length - 21)
    };
}

struct CardDrawer {
    sf::Texture texture;
    sf::Sprite sprite;
    sf::Text text;
    State state;

    CardDrawer(const std::string& _path, const std::wstring& _name, sf::Font& font, State _state) 
        : texture{ _path }, sprite{ texture }, text{ font, _name, FONT_SIZE }, state{ _state }
    {}

    void render(sf::RenderWindow& window) {
        window.draw(sprite);
        window.draw(text);
    }
};

class HolyTriangleDrawer {
    static constexpr int32_t horizontal_spacing = 50;
    static constexpr int32_t vertical_spacing = 39;
    static constexpr int32_t picture_width = 320;
    static constexpr int32_t picture_height = 533;

    CardDrawer past;
    CardDrawer now;
    CardDrawer future;
public:
    HolyTriangleDrawer(CardDrawer&& _past, CardDrawer&& _now, CardDrawer&& _future)
        : past{ std::move(_past) }, now{ std::move(_now) }, future{ std::move(_future) }
    {
        past.sprite.setTexture(past.texture);
        now.sprite.setTexture(now.texture);
        future.sprite.setTexture(future.texture);

        past.sprite.setOrigin(sf::Vector2f{ picture_width / 2.0f, picture_height / 2.0f });
        now.sprite.setOrigin(sf::Vector2f{ picture_width / 2.0f, picture_height / 2.0f });
        future.sprite.setOrigin(sf::Vector2f{ picture_width / 2.0f, picture_height / 2.0f });

        past.sprite.setPosition(sf::Vector2f{ horizontal_spacing + picture_width / 2, vertical_spacing + picture_height / 2 });
        now.sprite.setPosition(sf::Vector2f{ 2 * horizontal_spacing + picture_width * 3 / 2, vertical_spacing + picture_height / 2 });
        future.sprite.setPosition(sf::Vector2f{ 3 * horizontal_spacing + picture_width * 5 / 2, vertical_spacing + picture_height / 2 });

        if (past.state == State::negative) {
            past.sprite.rotate(sf::degrees(180));
        }

        if (now.state == State::negative) {
            now.sprite.rotate(sf::degrees(180));
        }

        if (future.state == State::negative) {
            future.sprite.rotate(sf::degrees(180));
        }

        past.text.setFillColor(sf::Color::White);
        now.text.setFillColor(sf::Color::White);
        future.text.setFillColor(sf::Color::White);

        past.text.setPosition(sf::Vector2f{ past.sprite.getPosition().x - past.text.getGlobalBounds().size.x / 2, 2 * vertical_spacing + picture_height });
        now.text.setPosition(sf::Vector2f{ now.sprite.getPosition().x - now.text.getGlobalBounds().size.x / 2, 2 * vertical_spacing + picture_height });
        future.text.setPosition(sf::Vector2f{ future.sprite.getPosition().x - future.text.getGlobalBounds().size.x / 2, 2 * vertical_spacing + picture_height });
    }

    void render(sf::RenderWindow& window) {
        past.render(window);
        now.render(window);
        future.render(window);
    }
};

int main() {
    CardInfo::init();

    Cards cards;
    sf::RenderWindow window(sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), L"塔罗牌");
    sf::Font font("res\\simfang.ttf");

    std::shared_ptr<HolyTriangleDrawer> drawer = nullptr;
    bool isClicked = false;

    sf::Text welcome_text_1{ font, L"请闭上眼睛仔细思考你要问的问题", FONT_SIZE };
    sf::Text welcome_text_2{ font, L"如果你已经准备好了，请点击一下任意位置", FONT_SIZE };

    welcome_text_1.setFillColor(sf::Color::White);
    welcome_text_2.setFillColor(sf::Color::White);

    welcome_text_1.setPosition(sf::Vector2f{ WINDOW_WIDTH / 2.0f - welcome_text_1.getGlobalBounds().size.x / 2.0f, WINDOW_HEIGHT / 2.0f - welcome_text_1.getGlobalBounds().size.y / 2.0f - 75 });
    welcome_text_2.setPosition(sf::Vector2f{ WINDOW_WIDTH / 2.0f - welcome_text_2.getGlobalBounds().size.x / 2.0f, WINDOW_HEIGHT / 2.0f + welcome_text_2.getGlobalBounds().size.y / 2.0f + 25 });

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (!isClicked && event->is<sf::Event::MouseButtonPressed>()) {
                isClicked = true;
                
                HolyTriangleResult result = divine_holy_triangle(cards);
                CardDrawer past{ CardInfo::path(result.past.type), CardInfo::name(result.past.type), font, result.past.state };
                CardDrawer now{ CardInfo::path(result.now.type), CardInfo::name(result.now.type), font, result.now.state };
                CardDrawer future{ CardInfo::path(result.future.type), CardInfo::name(result.future.type), font, result.future.state };

                drawer = std::make_shared<HolyTriangleDrawer>(std::move(past), std::move(now), std::move(future));
            }
        }
 
        window.clear();

        if (isClicked) {
            drawer->render(window);
        }
        else {
            window.draw(welcome_text_1);
            window.draw(welcome_text_2);
        }

        window.display();
    }

    return 0;
}
