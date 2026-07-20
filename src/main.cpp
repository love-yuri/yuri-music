//
// Created by yuri on 2026/1/31.
//

import main_window;
import qq_music_api;
import store;
import core;

int main() {
  store::user_profile_store.reload();
  MainWindow window;
  window.show();
}
