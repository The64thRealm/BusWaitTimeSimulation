//
// Created by The 64th Realm on 03/19/26.
//

#ifndef BUSWAITTIMESIMULATION_WINDOWSCLIPBOARDHELPER_H
#define BUSWAITTIMESIMULATION_WINDOWSCLIPBOARDHELPER_H

#include <string_view>
#include <windows.h>

inline void copyToClipboard(std::string_view text)
{
    OpenClipboard(nullptr);
    EmptyClipboard();

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);

    char* clipboardBuffer = static_cast<char*>(GlobalLock(hMem));

    memcpy(GlobalLock(clipboardBuffer), text.data(), text.length());

    clipboardBuffer[text.length()] = '\0';

    GlobalUnlock(hMem);

    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

#endif //BUSWAITTIMESIMULATION_WINDOWSCLIPBOARDHELPER_H