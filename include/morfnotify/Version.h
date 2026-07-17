/*
 * morfNotify
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <QString>

namespace morfnotify {

// Version de morfNotify, injectee par CMake depuis le fichier VERSION.
#ifndef MORFNOTIFY_VERSION
#  define MORFNOTIFY_VERSION "dev"
#endif

inline QString version() { return QStringLiteral(MORFNOTIFY_VERSION); }

// Version du protocole HTTP/JSON expose. A incrementer si le format de l'API
// (POST /notify) change de facon incompatible.
inline constexpr const char* kProtocol = "morfnotify/1";

} // namespace morfnotify
