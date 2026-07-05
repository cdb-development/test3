#pragma once
#include <unordered_map>
#include <string>
#include <vector>

class LangManager {
    int lang = 0;

    struct Lang {
        const char* label;
        int font;
        std::unordered_map<std::string, std::string> dict{};
    };

    std::vector<Lang> languages;
public:
    void add_language(const char* label, int font, std::unordered_map<std::string, std::string> dict) {
        languages.push_back(Lang{ label, font, std::move(dict) });
    }

    int& get_lang() {
        return lang;
    }

    const char* get_lang_name() {
        if (languages.empty()) return "English";
        return languages[lang].label;
    }

    std::vector<Lang>& get_langs() {
        return languages;
    }

    int get_font() {
        if (languages.empty()) return 0;
        return (int)languages[lang].font;
    }

    ImFont* get_font(int sz) {
        return fonts[(int)languages[lang].font].get(sz);
    }

    void set_lang(int i) {
        if (i >= 0 && i < languages.size()) {
            lang = i;
        }
    }

    static LangManager& get() {
        static LangManager s;
        return s;
    }

    const char* translate(const char* str) {
        return str;
    }

    void initialize() {
        add_language("English", font, {});
    }
};