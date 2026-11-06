// The Controls screen's calls into the gamepad (ControlsView.swift → DmGamepad.m "Bindings"), beside the binding model's own (keymap_api.h)
#pragma once
#include "../../host/keymap_api.h"

#ifdef __cplusplus
extern "C" {
#endif
const char *dm_keymap_game(void);   // the game's bindings over the base map (the profile's mouse_buttons and key_binds)
const char *dm_keymap_player(void);   // the player's bindings now (DM_KEYMAP, else the saved ones)
void dm_keymap_set(const char *player);   // the player's new bindings, at once while the game runs (the caller saves them as controls.keymap)
void dm_keymap_capture(void (*cb)(const char *token));   // non-NULL: the screen is open, every key, mouse button (but a left click) and controller button goes to cb as a token, none to the game; NULL: closed
#ifdef __cplusplus
}
#endif
