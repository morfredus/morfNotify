cmake_minimum_required(VERSION 3.21)

# Version pilotee par le fichier VERSION (meme convention que morfSensor / morfBeacon).
set(MORFNOTIFY_VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MORFNOTIFY_VERSION_FILE}")
file(STRINGS "${MORFNOTIFY_VERSION_FILE}" MORFNOTIFY_VERSION_FROM_FILE LIMIT_COUNT 1)
string(STRIP "${MORFNOTIFY_VERSION_FROM_FILE}" MORFNOTIFY_VERSION_FROM_FILE)

project(morfNotify VERSION "${MORFNOTIFY_VERSION_FROM_FILE}" LANGUAGES CXX)

# --- Standard C++ ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Qt : generation automatique du moc (classes QObject) -----------------
set(CMAKE_AUTOMOC ON)

# Core + Network suffisent : serveur HTTP entrant (reception) et client HTTP
# sortant (webhooks). Pas de Widgets : service sans interface.
find_package(Qt6 REQUIRED COMPONENTS Core Network)

# --- Dependance morfBeacon (annonce de presence sur le LAN) ---------------
# morfBeacon est EMBARQUE (copie vendoree) dans third_party/morf/beacon, lie
# statiquement, comme morfSensor / ComponentHub / SiteWatch. morfNotify se
# compile sans dependre d'aucun depot externe : parfait du premier coup, a
# l'identique sous Windows, Linux x64 et Raspberry Pi (ARM64).
#
# Ne pas editer le code sous third_party/ : la source de verite est le depot
# morfBeacon. Resynchroniser avec scripts/sync-morf.(sh|ps1).
if(NOT TARGET morfBeacon::morfBeacon)
    add_subdirectory(third_party/morf/beacon)
endif()

# --- Bibliotheque ---------------------------------------------------------
add_library(morfNotify STATIC
    src/NotifierRegistry.cpp
    src/NotifyHttpServer.cpp
    src/NotifyService.cpp
    src/NotifierFactory.cpp
    src/LogNotifier.cpp
    src/WebhookNotifier.cpp
    src/EmailNotifier.cpp
    src/TelegramNotifier.cpp
    include/morfnotify/Notification.h
    include/morfnotify/INotifier.h
    include/morfnotify/NotifierRegistry.h
    include/morfnotify/NotifyConfig.h
    include/morfnotify/NotifierFactory.h
    include/morfnotify/NotifyHttpServer.h
    include/morfnotify/NotifyService.h
    include/morfnotify/LogNotifier.h
    include/morfnotify/WebhookNotifier.h
    include/morfnotify/EmailNotifier.h
    include/morfnotify/TelegramNotifier.h
    include/morfnotify/Version.h
)
add_library(morfNotify::morfNotify ALIAS morfNotify)

target_include_directories(morfNotify PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(morfNotify
    PUBLIC  Qt6::Core Qt6::Network
    PRIVATE morfBeacon::morfBeacon)

# Numero de version accessible depuis le code.
target_compile_definitions(morfNotify PUBLIC MORFNOTIFY_VERSION="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(morfNotify PRIVATE /utf-8)
endif()

# --- Cible service (le demon autonome) ------------------------------------
add_subdirectory(service)

# --- Exemple (compile seulement en build autonome, pas via add_subdirectory) -
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_morfnotify_standalone ON)
else()
    set(_morfnotify_standalone OFF)
endif()
option(MORFNOTIFY_BUILD_EXAMPLES "Compiler l'exemple de demonstration" ${_morfnotify_standalone})

if(MORFNOTIFY_BUILD_EXAMPLES)
    add_subdirectory(examples/minimal)
endif()
