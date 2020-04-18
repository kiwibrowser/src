#include "isnt.hpp"

#ifdef _WIN_ALL
#include "versionhelpers.h"

DWORD WinNT()
{
  if (!IsWinXpOrGreater()) return WNT_NONE;
  if (!IsWinVistaOrGreater()) return WNT_WXP;
  if (!IsWindows7OrGreater()) return WNT_VISTA;
  if (!IsWindows8OrGreater()) return WNT_W7;
  if (!IsWindows8Point1OrGreater()) return WNT_W8;
  if (!IsWindows10OrGreater()) return WNT_W81;
  return WNT_W10;
}
#endif
