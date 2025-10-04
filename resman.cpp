#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <stdio.h>
#include <termios.h>
#include <unistd.h> // usleep
#endif

std::atomic<bool> confirmed(false);

// Cross-platform helpers
void clear() {
#ifdef _WIN32
  system("cls");
#else
  system("clear");
#endif
}

void sleep_ms(int ms) {
#ifdef _WIN32
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}

void setColor(int color) {
#ifdef _WIN32
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#else
  // ANSI escape codes (Linux/macOS)
  switch (color) {
  case 10:
    std::cout << "\033[32m";
    break; // green
  case 11:
    std::cout << "\033[36m";
    break; // cyan
  case 12:
    std::cout << "\033[31m";
    break; // red
  case 14:
    std::cout << "\033[33m";
    break; // yellow
  case 15:
    std::cout << "\033[0m";
    break; // reset
  default:
    std::cout << "\033[0m";
    break;
  }
#endif
}

void printBanner() {
  setColor(10); // green
  std::cout << R"( 
+============================================+
|                                            |
|    ______          ___  ___                |
|    | ___ \         |  \/  |                |
|    | |_/ /___  ___ | .  . | __ _ _ __      |
|    |    // _ \/ __|| |\/| |/ _` | '_ \     |
|    | |\ \  __/\__ \| |  | | (_| | | | |    |
|    \_| \_\___||___/\_|  |_/\__,_|_| |_|    |
|                                            |
+============================================+
)" << "\n";
  setColor(15); // reset
}

#ifdef _WIN32
// Windows-specific resolution handling
void setResolution(int width, int height, DEVMODE &backup) {
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &backup);

  DEVMODE dm = {0};
  dm.dmSize = sizeof(dm);
  dm.dmPelsWidth = width;
  dm.dmPelsHeight = height;
  dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

  LONG result = ChangeDisplaySettings(&dm, CDS_UPDATEREGISTRY);

  if (result == DISP_CHANGE_SUCCESSFUL) {
    std::cout << "\nSwitched to " << width << "x" << height << "\n";
  } else {
    std::cout << "\nFailed to change resolution. Error code: " << result
              << "\n";
  }
}

void countdown(DEVMODE original) {
  for (int i = 10; i > 0; --i) {
    if (confirmed)
      return;
    std::cout << "\rConfirm resolution in " << i << " seconds...   ";
    Sleep(1000);
  }
  if (!confirmed) {
    setColor(12);
    std::cout << "\nTime expired. Restoring previous resolution...\n";
    ChangeDisplaySettings(&original, 0);
    setColor(15);
  }
}
#else
// Linux demo
void setResolution(int width, int height) {
  std::cout << "\nPretending to change resolution to " << width << "x" << height
            << " (demo mode)\n";
}

void countdown() {
  for (int i = 10; i > 0; --i) {
    if (confirmed)
      return;
    std::cout << "\rConfirm resolution in " << i << " seconds...   "
              << std::flush;
    sleep_ms(1000);
  }
  if (!confirmed) {
    setColor(12);
    std::cout << "\nTime expired. Restoring previous resolution (demo)...\n";
    setColor(15);
  }
}
#endif

// Menu drawing
int drawMenu(int selected) {
  const char *options[] = {"2560 x 1080", "1920 x 1080", "1366 x 768", "Exit"};
  int numOptions = sizeof(options) / sizeof(options[0]);

  clear();
  printBanner();

  setColor(10);
  std::cout << "+============================================+\n";
  setColor(15);
  std::cout << " Use j/k or arrows to move \n";
  std::cout << " Enter or l to select      \n";
  setColor(10);
  std::cout << "+============================================+\n";
  setColor(15);

  for (int i = 0; i < numOptions; i++) {
    if (i == selected) {
      setColor(10);
      std::cout << " > " << options[i] << " <\n";
      setColor(15);
    } else {
      std::cout << "   " << options[i] << "\n";
    }
  }
  setColor(10);
  std::cout << "+============================================+\n";
  setColor(15);

  return numOptions;
}

// Input handling
#ifdef _WIN32
int getKey() {
  int key = _getch();
  if (key == 224) { // arrow prefix
    int arrow = _getch();
    return 1000 + arrow; // encode arrows
  }
  return key;
}
#else
int getKey() {
  struct termios oldt, newt;
  int ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  if (ch == 27) { // possible arrow
    if (getchar() == 91) {
      int arrow = getchar();
      return 1000 + arrow;
    }
  }
  return ch;
}
#endif

// Main
int main() {
  int selected = 0;
  int numOptions;

  while (true) {
    numOptions = drawMenu(selected);

    int key = getKey();

    if (key == 'k' || key == 1000 + 72 ||
        key == 1000 + 65) { // up: k / WinUp / LinuxUp
      selected = (selected - 1 + numOptions) % numOptions;
    } else if (key == 'j' || key == 1000 + 80 ||
               key == 1000 + 66) { // down: j / WinDown / LinuxDown
      selected = (selected + 1) % numOptions;
    } else if (key == 13 || key == 10 || key == 'l') { // enter / l
      if (selected == numOptions - 1) {
        clear();
        std::cout << "Goodbye!\n";
        break;
      } else {
        clear();
        printBanner();
        int width = 0, height = 0;
        if (selected == 0) {
          width = 2560;
          height = 1080;
        }
        if (selected == 1) {
          width = 1920;
          height = 1080;
        }
        if (selected == 2) {
          width = 1366;
          height = 768;
        }

#ifdef _WIN32
        DEVMODE original;
        setResolution(width, height, original);
        confirmed = false;
        std::thread timer(countdown, original);
        std::cout << "\nPress Y to keep this resolution: ";
        char c = _getch();
        if (c == 'y' || c == 'Y') {
          confirmed = true;
          setColor(10);
          std::cout << "\nResolution kept!\n";
          setColor(15);
        }
        timer.join();
#else
        setResolution(width, height);
        confirmed = false;
        std::thread timer(countdown);

        std::cout << "\nPress Y to keep this resolution: ";
        char c;
        std::cin >> c;
        if (c == 'y' || c == 'Y') {
          confirmed = true;
          setColor(10);
          std::cout << "\nPretending to keep resolution!\n";
          setColor(15);
        }
        timer.join();
#endif
        std::cout << "Press Enter to continue...";
        std::cin.ignore();
        std::cin.get();
      }
    }
  }
  return 0;
}
