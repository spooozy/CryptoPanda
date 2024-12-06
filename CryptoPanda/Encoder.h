#pragma once
#include <fstream>
#include <bitset>
#include <sstream>
#include <vector>
#include <locale>
#include <codecvt>
#include <filesystem>
#include <Windows.h>

class Encoder {
private:
    uint32_t iniKey; //исходный ключ
    uint8_t Key; //ключ для шифрования
    const int BlockSize = 256;
    void shift();
    uint32_t SumBytes(const std::wstring& input);
public:    
    Encoder();
    void SetKey(std::wstring key);
    bool EncryptDecryptFile(const std::wstring& input, const std::wstring& output);  
    std::wstring EncryptLine(const std::wstring& input);
};