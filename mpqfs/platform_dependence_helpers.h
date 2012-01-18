// platform_dependence_helpers.h is part of mpqfs-stormlib, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+mpqfs@googlemail.com>

#ifndef __PLATFORM_DEPENDENCE_HELPERS_H
#define __PLATFORM_DEPENDENCE_HELPERS_H

namespace platform_dependence_helpers
{
  inline static time_t filetime_to_time_t ( const unsigned long long int& lo
                                          , const unsigned long long int& hi
                                          )
  {
    return time_t ((hi << 32 | lo) / 10000000LL - 11644473600LL);
  }

  inline static unsigned long long time_t_to_filetime (const time_t& time)
  {
    const unsigned long long t ((time + 11644473600LL) * 10000000LL);
    return t;
  }
}

#endif
