#include "Encoder.h"

Encoder::Encoder() {

}

void Encoder::SetKey(std::wstring key) {
    this->iniKey = SumBytes(key);
    this->Key = 0;
}

uint32_t Encoder::SumBytes(const std::wstring& input) {
    uint32_t sum = 0;
    int i = 1;
    for (const wchar_t& ch : input) {
        sum += static_cast<uint32_t>(static_cast<uint8_t>(ch)) * i; 
        i++;
    }
    return sum;
}

void Encoder::shift() {
    Key = 0;
    for (int i = 0; i < 8; i++) {
        // Получить новый бит (младший байт регистра сдвига)
        uint8_t xorBit = (iniKey & 1) ^ ((iniKey >> 26) & 1) ^ ((iniKey >> 27) & 1) ^ ((iniKey >> 31) & 1);
        // Получить новый бит ключа
        uint8_t bit32 = (iniKey >> 31) & 1;
        // Запись нового бита в ключ справа
        Key = (Key << 1) | bit32;
        // Записать в регистр новый бит
        iniKey = (iniKey << 1) | xorBit;
    }
}

bool Encoder::EncryptDecryptFile(const std::wstring& input, const std::wstring& output) {
    std::ifstream file_in(input, std::ios::binary);
    std::ofstream file_out(output, std::ios::binary);

    if (!file_in.is_open()) {        
        return false;
    }

    if (!file_out.is_open()) {
        return false;
    }

    std::vector<uint8_t> buffer(BlockSize);
    while (file_in.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || file_in.gcount() > 0) {
        int bytesRead = file_in.gcount();
        int j = 0;

        for (int i = 0; i < bytesRead; i++) {
            shift();
            buffer[i] ^= Key;

        }
        file_out.write(reinterpret_cast<char*>(buffer.data()), bytesRead);
    }

    DWORD attributes = GetFileAttributes(output.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributes(output.c_str(), attributes | FILE_ATTRIBUTE_READONLY);
    }   
    file_in.close();
    file_out.close();
    return true;    
}

std::wstring Encoder::EncryptLine(const std::wstring& input) {
    std::wstring output;
    output.resize(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        shift();
        output[i] = input[i] ^ Key;

    return output;
}
