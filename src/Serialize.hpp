#pragma once

namespace boza::detail
{
    template<typename T, std::size_t... Is>
    constexpr auto get_field_names_array_impl(std::index_sequence<Is...>) noexcept
    {
        return std::array<std::string_view, sizeof...(Is)>{ pfr::get_name<Is, T>()... };
    }

    template<typename T>
    constexpr auto get_field_names_for_pfr() noexcept
    {
        using CleanT = std::remove_cv_t<std::remove_reference_t<T>>;
        return get_field_names_array_impl<CleanT>(std::make_index_sequence<pfr::tuple_size_v<CleanT>>());
    }
}

#define SERIALIZE_FIELD_TO_JSON(r, data_unused, elem) { BOOST_PP_STRINGIZE(elem), elem },
#define SERIALIZE_FIELD_FROM_JSON(r, data_unused, elem) if (j.contains(BOOST_PP_STRINGIZE(elem))) j.at(BOOST_PP_STRINGIZE(elem)).get_to(elem);

#define SERIALIZE(...)                                                                                                                          \
public:                                                                                                                                         \
    [[nodiscard]] json to_json() const { return { BOOST_PP_SEQ_FOR_EACH(SERIALIZE_FIELD_TO_JSON, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) }; } \
    void from_json(const json& j) { BOOST_PP_SEQ_FOR_EACH(SERIALIZE_FIELD_FROM_JSON, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)); }


#define SERIALIZE_ALL                                                                                                   \
public:                                                                                                                 \
    [[nodiscard]] json to_json() const                                                                                  \
    {                                                                                                                   \
        json j;                                                                                                         \
        constexpr auto field_names_arr = detail::get_field_names_for_pfr<std::decay_t<decltype(*this)>>();              \
        pfr::for_each_field(*this, [&](const auto& field_value, size_t field_index) {                                   \
            if (field_index < field_names_arr.size()) j[std::string(field_names_arr[field_index])] = field_value;       \
        });                                                                                                             \
        return j;                                                                                                       \
    }                                                                                                                   \
    void from_json(const json& j)                                                                                       \
    {                                                                                                                   \
        constexpr auto field_names_arr = detail::get_field_names_for_pfr<std::decay_t<decltype(*this)>>();              \
        pfr::for_each_field(*this, [&](auto& field_value, size_t field_index) {                                         \
            if (field_index < field_names_arr.size()) {                                                                 \
                std::string field_name_str(field_names_arr[field_index]);                                               \
                if (j.contains(field_name_str)) j.at(field_name_str).get_to(field_value);                               \
            }                                                                                                           \
        });                                                                                                             \
    }