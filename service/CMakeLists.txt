# Demon de service morfNotify : execute la bibliotheque en tache de fond.
# Le binaire s'appelle « morfnotify » (comme l'unite systemd et /opt/morfnotify).
add_executable(morfnotify main.cpp)

target_link_libraries(morfnotify PRIVATE morfNotify::morfNotify Qt6::Core)

if(MSVC)
    target_compile_options(morfnotify PRIVATE /utf-8)
endif()

install(TARGETS morfnotify RUNTIME DESTINATION bin)
