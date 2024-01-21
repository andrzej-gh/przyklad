// ConsoleApplication4.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <Windows.h>
#include <wininet.h>
#include <iostream>
#include <string>
#include <sstream>

#pragma comment(lib, "wininet.lib")

std::string FindSubString(HINTERNET hConnect, const char* startPattern, const char* endPattern, const char* searchText) {
    const int bufferSize = 4096;
    char buffer[bufferSize];
    DWORD bytesRead;

    std::string context;

    while (InternetReadFile(hConnect, buffer, bufferSize, &bytesRead) && bytesRead > 0) {
        // Process the buffer (e.g., search for the desired text)
        std::string content(buffer, bytesRead);
        context += content;

        // Check if the end pattern is found
        size_t endPatternPos = context.find(endPattern);
        if (endPatternPos != std::string::npos) {
            // Extract the substring starting from the start pattern and ending with the end pattern
            size_t startPatternPos = context.rfind(startPattern, endPatternPos);
            if (startPatternPos != std::string::npos) {
                std::string extractedText = context.substr(startPatternPos, endPatternPos - startPatternPos);

                // Check if the searched text is present in the extracted substring
                size_t searchTextPos = extractedText.find(searchText);
                if (searchTextPos != std::string::npos) {
                    // We have book name and author correct so this is probably it...
                    return extractedText;
                }
            }

            // Reset context after processing
            context.clear();
        }
    }

    return NULL;
}

std::string getStringBetween(const std::string& input, const std::string& start, const std::string& end) {
    size_t startPos = input.find(start);
    if (startPos == std::string::npos) {
        // Start string not found
        return "";
    }

    startPos += start.length();

    size_t endPos = input.find(end, startPos);
    if (endPos == std::string::npos) {
        // End string not found
        return "";
    }

    return input.substr(startPos, endPos - startPos);
}

int downloadBookData(const char* bookName, const char* bookAuthor) {
    HINTERNET hInternet, hConnect;
    std::wstring fullUrl = L"https://lubimyczytac.pl/szukaj/ksiazki?phrase=";
    std::wstringstream wss;
    std::string bookNameStr(bookName);
    wss << std::wstring(bookNameStr.begin(), bookNameStr.end());


    fullUrl += wss.str();

    // Initialize WinINet
    hInternet = InternetOpen(L"WinINet Example", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        std::cerr << "Failed to initialize WinINet." << std::endl;
        return 1;
    }

    // Open a connection to search book
    hConnect = InternetOpenUrl(hInternet, fullUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        std::cerr << "Failed to open a connection." << std::endl;
        InternetCloseHandle(hInternet);
        return 1;
    }

    std::string bookCtx = FindSubString(hConnect, "authorAllBooks__singleTextTitle", "listLibrary__info", bookAuthor);


    // get link to the book
    size_t searchLinkPos = bookCtx.find("/ksiazka");
    size_t searchLinkPosE = bookCtx.find("\"", searchLinkPos);
    std::string link = "https://lubimyczytac.pl" + bookCtx.substr(searchLinkPos, searchLinkPosE - searchLinkPos);

    // Cleanup
    InternetCloseHandle(hConnect);

    // Open a connection to the book to gather information
    wss = std::wstringstream();
    wss << std::wstring(link.begin(), link.end());
    std::wstring wLink = wss.str();

    hConnect = InternetOpenUrl(hInternet, wLink.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        std::cerr << "Failed to open a connection." << std::endl;
        InternetCloseHandle(hInternet);
        return 1;
    }

    bookCtx = FindSubString(hConnect, "Kategoria", "umacz:", "Data");
    std::string isbn = getStringBetween(bookCtx, "ISBN:</dt><dd> ", " </dd>");
    std::string pages = getStringBetween(bookCtx, "Liczba stron:</dt><dd> ", " </dd>");

    std::cout <<
        "Link: " << link << std::endl <<
        "ISBN: " << isbn << std::endl <<
        "Liczba stron: " << pages << std::endl;
    //we can gather more in similar way

    InternetCloseHandle(hConnect);

    //get book cover url
    hConnect = InternetOpenUrl(hInternet, wLink.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        std::cerr << "Failed to open a connection." << std::endl;
        InternetCloseHandle(hInternet);
        return 1;
    }

    bookCtx = FindSubString(hConnect, "picture", "media", "jpg");
    std::string imgLink = getStringBetween(bookCtx, "srcset=\"", "\"");
    std::cout << imgLink << std::endl;
    wss = std::wstringstream();
    wss << std::wstring(imgLink.begin(), imgLink.end());
    wLink = wss.str();
    InternetCloseHandle(hConnect);

    //download book cover
    hConnect = InternetOpenUrl(hInternet, wLink.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        std::cerr << "Failed to open a connection." << std::endl;
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Create and open a local file for writing
    const wchar_t* filePath = L"cover.jpg";
    HANDLE hFile = CreateFile(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateFile failed" << std::endl;
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Download and write to the local file
    DWORD bytesRead, bytesWritten;
    BYTE buffer[4096];
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL);
    }
    
    //Cleanup
    CloseHandle(hFile);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return 0;
}


int _tmain(int argc, _TCHAR* argv[]) {
    downloadBookData("gra endera", "Scott Card");
}
