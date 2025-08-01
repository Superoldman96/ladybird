/*
 * Copyright (c) 2024-2025, Tim Flynn <trflynn89@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Optional.h>
#include <AK/OwnPtr.h>
#include <AK/String.h>
#include <AK/StringView.h>
#include <AK/Utf16String.h>
#include <AK/Vector.h>
#include <LibUnicode/DurationFormat.h>

#include <unicode/locid.h>
#include <unicode/strenum.h>
#include <unicode/stringpiece.h>
#include <unicode/uiter.h>
#include <unicode/uloc.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>
#include <unicode/uversion.h>

U_NAMESPACE_BEGIN
class DateTimePatternGenerator;
class LocaleDisplayNames;
class NumberingSystem;
class TimeZone;
class TimeZoneNames;
U_NAMESPACE_END

namespace Unicode {

class LocaleData {
public:
    static Optional<LocaleData&> for_locale(StringView locale);

    ALWAYS_INLINE icu::Locale& locale() { return m_locale; }

    String to_string();

    icu::LocaleDisplayNames& standard_display_names();
    icu::LocaleDisplayNames& dialect_display_names();

    icu::NumberingSystem& numbering_system();

    icu::DateTimePatternGenerator& date_time_pattern_generator();

    icu::TimeZoneNames& time_zone_names();

    Optional<DigitalFormat> const& digital_format() { return m_digital_format; }
    void set_digital_format(DigitalFormat digital_format) { m_digital_format = move(digital_format); }

private:
    explicit LocaleData(icu::Locale locale);

    icu::Locale m_locale;
    Optional<String> m_locale_string;

    OwnPtr<icu::LocaleDisplayNames> m_standard_display_names;
    OwnPtr<icu::LocaleDisplayNames> m_dialect_display_names;
    OwnPtr<icu::NumberingSystem> m_numbering_system;
    OwnPtr<icu::DateTimePatternGenerator> m_date_time_pattern_generator;
    OwnPtr<icu::TimeZoneNames> m_time_zone_names;
    Optional<DigitalFormat> m_digital_format;
};

class TimeZoneData {
public:
    static Optional<TimeZoneData&> for_time_zone(StringView time_zone);

    ALWAYS_INLINE icu::TimeZone& time_zone() { return *m_time_zone; }

private:
    explicit TimeZoneData(NonnullOwnPtr<icu::TimeZone>);

    NonnullOwnPtr<icu::TimeZone> m_time_zone;
};

constexpr bool icu_success(UErrorCode code)
{
    return static_cast<bool>(U_SUCCESS(code));
}

constexpr bool icu_failure(UErrorCode code)
{
    return static_cast<bool>(U_FAILURE(code));
}

ALWAYS_INLINE icu::StringPiece icu_string_piece(StringView string)
{
    return { string.characters_without_null_termination(), static_cast<i32>(string.length()) };
}

ALWAYS_INLINE icu::UnicodeString icu_string(StringView string)
{
    return icu::UnicodeString::fromUTF8(icu_string_piece(string));
}

// If the Utf16View has ASCII storage, this creates an owned icu::UnicodeString. Otherwise, the icu::UnicodeString is
// unowned (i.e. it is effectively a view).
ALWAYS_INLINE icu::UnicodeString icu_string(Utf16View const& string)
{
    if (string.has_ascii_storage())
        return icu::UnicodeString::fromUTF8(icu_string_piece(string.bytes()));
    return { false, string.utf16_span().data(), static_cast<i32>(string.length_in_code_units()) };
}

Vector<icu::UnicodeString> icu_string_list(ReadonlySpan<Utf16String> strings);

String icu_string_to_string(icu::UnicodeString const& string);
String icu_string_to_string(UChar const*, i32 length);

Utf16String icu_string_to_utf16_string(icu::UnicodeString const& string);
Utf16String icu_string_to_utf16_string(UChar const*, i32 length);

Utf16View icu_string_to_utf16_view(icu::UnicodeString const& string);

UCharIterator icu_string_iterator(Utf16View const&);

template<typename Filter>
Vector<String> icu_string_enumeration_to_list(OwnPtr<icu::StringEnumeration> enumeration, char const* bcp47_keyword, Filter&& filter)
{
    UErrorCode status = U_ZERO_ERROR;
    Vector<String> result;

    if (!enumeration)
        return {};

    while (true) {
        i32 length = 0;
        auto const* value = enumeration->next(&length, status);

        if (icu_failure(status) || value == nullptr)
            break;

        if (!filter(value, static_cast<size_t>(length)))
            continue;

        if (bcp47_keyword) {
            if (auto const* bcp47_value = uloc_toUnicodeLocaleType(bcp47_keyword, value))
                result.append(MUST(String::from_utf8({ bcp47_value, strlen(bcp47_value) })));
        } else {
            result.append(MUST(String::from_utf8({ value, static_cast<size_t>(length) })));
        }
    }

    return result;
}

ALWAYS_INLINE Vector<String> icu_string_enumeration_to_list(OwnPtr<icu::StringEnumeration> enumeration, char const* bcp47_keyword)
{
    return icu_string_enumeration_to_list(move(enumeration), bcp47_keyword, [](char const*, size_t) { return true; });
}

}
