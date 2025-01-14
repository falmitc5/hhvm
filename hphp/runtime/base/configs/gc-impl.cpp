/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-present Facebook, Inc. (http://www.facebook.com)  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#include "hphp/runtime/base/configs/gc-loader.h"

#include <folly/lang/Bits.h>

namespace HPHP::Cfg {

#ifdef HHVM_EAGER_GC
  constexpr bool kEagerGC = true;
#else
  constexpr bool kEagerGC = false;
#endif

bool GCLoader::EnabledDefault() {
  return kEagerGC;
}

bool GCLoader::EagerDefault() {
  return kEagerGC;
}

bool GCLoader::QuarantineDefault() {
  return kEagerGC;
}

void GCLoader::SlabAllocAlignPostProcess(uint32_t& val) {
  val = folly::nextPowTwo(val);
  val = std::min(val, 4096U);
}

}
