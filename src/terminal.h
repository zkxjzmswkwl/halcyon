#pragma once
#include <Windows.h>
#include <processenv.h>
#include <winbase.h>
#include <stdio.h>
#include <string>


namespace term
{
  static void clear()
  {
    printf("\033c");
  }

  static void set_color_green()
  {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
  }

  static void set_color_white()
  {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
  }

  static void print_label(std::string text)
  {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | BACKGROUND_RED);
    printf(text.c_str());
    set_color_white();
  }

  static void print_green(std::string text)
  {
    set_color_green();
    printf(text.c_str());
    set_color_white();
  }
}
