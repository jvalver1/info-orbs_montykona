#ifndef POSIX_TIMEZONES_H
#define POSIX_TIMEZONES_H

// POSIX timezone strings for IANA timezone identifiers
// Source: https://github.com/nayarsystems/posix_tz_db
// These strings encode DST rules so the ESP32 can compute local time
// without any external API calls.

struct TzEntry {
    const char *iana;
    const char *posix;
};

// Sorted by IANA name for binary search
static const TzEntry TZ_DATABASE[] = {
    {"Africa/Abidjan", "GMT0"},
    {"Africa/Accra", "GMT0"},
    {"Africa/Addis_Ababa", "EAT-3"},
    {"Africa/Algiers", "CET-1"},
    {"Africa/Cairo", "EET-2EEST,M4.5.5/0,M10.5.4/24"},
    {"Africa/Casablanca", "<+01>-1"},
    {"Africa/Dar_es_Salaam", "EAT-3"},
    {"Africa/Johannesburg", "SAST-2"},
    {"Africa/Lagos", "WAT-1"},
    {"Africa/Nairobi", "EAT-3"},
    {"Africa/Tunis", "CET-1"},
    {"America/Adak", "HST10HDT,M3.2.0,M11.1.0"},
    {"America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0"},
    {"America/Argentina/Buenos_Aires", "<-03>3"},
    {"America/Bogota", "<-05>5"},
    {"America/Chicago", "CST6CDT,M3.2.0,M11.1.0"},
    {"America/Denver", "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Detroit", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Edmonton", "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Halifax", "AST4ADT,M3.2.0,M11.1.0"},
    {"America/Havana", "CST5CDT,M3.2.0/0,M11.1.0/1"},
    {"America/Lima", "<-05>5"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Manaus", "<-04>4"},
    {"America/Mexico_City", "CST6"},
    {"America/Montreal", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Panama", "EST5"},
    {"America/Phoenix", "MST7"},
    {"America/Santiago", "<-04>4<-03>,M9.1.6/24,M4.1.6/24"},
    {"America/Sao_Paulo", "<-03>3"},
    {"America/St_Johns", "NST3:30NDT,M3.2.0,M11.1.0"},
    {"America/Toronto", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Vancouver", "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Winnipeg", "CST6CDT,M3.2.0,M11.1.0"},
    {"Asia/Almaty", "<+05>-5"},
    {"Asia/Baghdad", "<+03>-3"},
    {"Asia/Bangkok", "<+07>-7"},
    {"Asia/Beirut", "EET-2EEST,M3.5.0/0,M10.5.0/0"},
    {"Asia/Colombo", "<+0530>-5:30"},
    {"Asia/Dhaka", "<+06>-6"},
    {"Asia/Dubai", "<+04>-4"},
    {"Asia/Ho_Chi_Minh", "<+07>-7"},
    {"Asia/Hong_Kong", "HKT-8"},
    {"Asia/Irkutsk", "<+08>-8"},
    {"Asia/Jakarta", "WIB-7"},
    {"Asia/Jerusalem", "IST-2IDT,M3.4.4/26,M10.5.0"},
    {"Asia/Kabul", "<+0430>-4:30"},
    {"Asia/Karachi", "PKT-5"},
    {"Asia/Kathmandu", "<+0545>-5:45"},
    {"Asia/Kolkata", "IST-5:30"},
    {"Asia/Krasnoyarsk", "<+07>-7"},
    {"Asia/Kuala_Lumpur", "<+08>-8"},
    {"Asia/Kuwait", "<+03>-3"},
    {"Asia/Manila", "PST-8"},
    {"Asia/Muscat", "<+04>-4"},
    {"Asia/Novosibirsk", "<+07>-7"},
    {"Asia/Riyadh", "<+03>-3"},
    {"Asia/Seoul", "KST-9"},
    {"Asia/Shanghai", "CST-8"},
    {"Asia/Singapore", "<+08>-8"},
    {"Asia/Taipei", "CST-8"},
    {"Asia/Tashkent", "<+05>-5"},
    {"Asia/Tehran", "<+0330>-3:30"},
    {"Asia/Tokyo", "JST-9"},
    {"Asia/Vladivostok", "<+10>-10"},
    {"Asia/Yekaterinburg", "<+05>-5"},
    {"Atlantic/Azores", "<-01>1<+00>,M3.5.0/0,M10.5.0/1"},
    {"Atlantic/Canary", "WET0WEST,M3.5.0/1,M10.5.0"},
    {"Atlantic/Reykjavik", "GMT0"},
    {"Australia/Adelaide", "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
    {"Australia/Brisbane", "AEST-10"},
    {"Australia/Darwin", "ACST-9:30"},
    {"Australia/Hobart", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Australia/Melbourne", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Australia/Perth", "AWST-8"},
    {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Belgrade", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Brussels", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Bucharest", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Budapest", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Copenhagen", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Dublin", "IST-1GMT0,M10.5.0,M3.5.0/1"},
    {"Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Istanbul", "<+03>-3"},
    {"Europe/Kiev", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Lisbon", "WET0WEST,M3.5.0/1,M10.5.0"},
    {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Europe/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Moscow", "MSK-3"},
    {"Europe/Oslo", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Prague", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Riga", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Rome", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Stockholm", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Vienna", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Vilnius", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Warsaw", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Zurich", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
    {"Pacific/Fiji", "<+12>-12"},
    {"Pacific/Guam", "ChST-10"},
    {"Pacific/Honolulu", "HST10"},
    {"Pacific/Tongatapu", "<+13>-13"},
    {"Etc/GMT", "GMT0"},
    {"Etc/UTC", "UTC0"},
};

static const int TZ_DATABASE_SIZE = sizeof(TZ_DATABASE) / sizeof(TZ_DATABASE[0]);

// Lookup POSIX timezone string from IANA identifier
// Returns nullptr if not found
inline const char *getPosixTz(const char *iana) {
    // Linear search (database is small enough for embedded use)
    for (int i = 0; i < TZ_DATABASE_SIZE; i++) {
        if (strcmp(TZ_DATABASE[i].iana, iana) == 0) {
            return TZ_DATABASE[i].posix;
        }
    }
    return nullptr;
}

#endif // POSIX_TIMEZONES_H
