#include <argv.hpp>

#include <exception>
#include <optional>

auto john::make_argv(std::string_view str) -> std::vector<std::string> {
    enum class state {
        waiting_for_start,
        reading_plain,
        reading_plain_esc,
        reading_string_typ_0,  // "
        reading_string_typ_0_esc,
        reading_string_typ_0_end,
        reading_string_typ_1,  // '
        reading_string_typ_1_esc,
        reading_string_typ_1_end,
    };

    struct condition {
        state m_state;
        std::optional<char> m_cur;
    };

    enum class push_type {
        ignore_char,
        push_char,
        push_escaped_char,
    };

    struct action {
        push_type m_push;
        state m_transition;

        bool m_consume = true;
        bool m_finish_token = false;
    };

    // clang-format off
    const std::pair<condition, action> state_transitions[]{
    //  state                     cur     push                    transition                consume finish
      {{state::waiting_for_start, ' ' }, {push_type::ignore_char, state::waiting_for_start}},
      {{state::waiting_for_start, '\t'}, {push_type::ignore_char, state::waiting_for_start}},
      {{state::waiting_for_start, '"' }, {push_type::ignore_char, state::reading_string_typ_0}},
      {{state::waiting_for_start, '\''}, {push_type::ignore_char, state::reading_string_typ_1}},
      {{state::waiting_for_start, '\0'}, {push_type::ignore_char, state::waiting_for_start, false}},
      {{state::waiting_for_start, {}  }, {push_type::ignore_char, state::reading_plain,     false}},

      {{state::reading_plain,     '\\'}, {push_type::ignore_char,       state::reading_plain_esc}},
      {{state::reading_plain,     ' ' }, {push_type::ignore_char,       state::waiting_for_start, false, true}},
      {{state::reading_plain,     '\0'}, {push_type::ignore_char,       state::waiting_for_start, false, true}},
      {{state::reading_plain,     {}  }, {push_type::push_char,         state::reading_plain}},
      {{state::reading_plain_esc, '\0'}, {push_type::ignore_char,       state::waiting_for_start, false, true}},
      {{state::reading_plain_esc, {}  }, {push_type::push_escaped_char, state::reading_plain}},

      {{state::reading_string_typ_0,     '\\'}, {push_type::ignore_char,       state::reading_string_typ_0_esc}},
      {{state::reading_string_typ_0,     '"' }, {push_type::ignore_char,       state::reading_string_typ_0_end}},
      {{state::reading_string_typ_0,     '\0'}, {push_type::ignore_char,       state::waiting_for_start, false, true}}, // fail
      {{state::reading_string_typ_0,     {}  }, {push_type::push_char,         state::reading_string_typ_0}},
      {{state::reading_string_typ_0_esc, '\0'}, {push_type::ignore_char,       state::waiting_for_start, false, true}},
      {{state::reading_string_typ_0_esc, {}  }, {push_type::push_escaped_char, state::reading_string_typ_0}},
      {{state::reading_string_typ_0_end, ' ' }, {push_type::ignore_char,       state::waiting_for_start, false, true}},
      {{state::reading_string_typ_0_end, '\t'}, {push_type::ignore_char,       state::waiting_for_start, false, true}},
      {{state::reading_string_typ_0_end, '\0'}, {push_type::ignore_char,       state::waiting_for_start, false, true}},
      {{state::reading_string_typ_0_end, {}  }, {push_type::ignore_char,       state::reading_plain,     false, false}},
    };
    // clang-format on

    const auto escapes = std::array<std::pair<char, char>, 2>{{
      {'t', '\t'},
      {'n', '\n'},
    }};

    const auto do_escape = [&escapes](char c) -> char {
        const auto* const it = std::find_if(escapes.begin(), escapes.end(), [c](auto const& p) { return p.first == c; });
        return it == escapes.end() ? c : it->second;
    };

    auto ret = std::vector<std::string>{};

    auto state = state::waiting_for_start;
    auto current_token = std::string{};

    const auto process_char = [&](char c) -> bool {
        const auto* const row = std::find_if(std::begin(state_transitions), std::end(state_transitions), [&](auto const& row) {
            return row.first.m_state == state && row.first.m_cur.value_or(c) == c;  //
        });

        if (row == std::end(state_transitions)) {
            std::terminate();
        }

        auto const& action = row->second;

        switch (action.m_push) {
            case push_type::ignore_char: break;
            case push_type::push_char: current_token.push_back(c); break;
            case push_type::push_escaped_char: current_token.push_back(do_escape(c)); break;
        }

        if (action.m_finish_token) {
            ret.emplace_back(std::move(current_token));
        }

        state = action.m_transition;
        return action.m_consume;
    };

    for (auto i = 0uz; i < str.size(); i++) {
        const auto c = str[i];

        const auto consume = process_char(c);

        if (!consume) {
            i--;
        }
    }

    process_char('\0');

    return ret;
}
