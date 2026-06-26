//
// Created by yuri on 2026/1/31.
//

import main_window;
import qq_music_api;
import core;

int main() {
  qqmusic_api_config.loginFromFile();

  MainWindow window;
  window.show();
}
