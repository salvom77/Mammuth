#ifndef MAMMUTH_UTF8_H
#define MAMMUTH_UTF8_H

#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>

// Eccezione per errori UTF-8
struct Utf8Error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Decodifica stringa UTF-8 in codepoint (char32_t).
// Lancia Utf8Error se la stringa non è UTF-8 valida.
inline std::vector<char32_t> decodeUtf8(const std::string& s) {
    std::vector<char32_t> out;
    const unsigned char* p   = reinterpret_cast<const unsigned char*>(s.data());
    const unsigned char* end = p + s.size();

    while (p < end) {
        uint32_t cp = 0;
        unsigned char c = *p;

        if (c < 0x80) {
            cp = c;
            ++p;
        } else if ((c >> 5) == 0x6) { // 2 byte
            if (p + 1 >= end) throw Utf8Error("Stringa UTF-8 troncata (attesi 2 byte).");
            unsigned char c1 = *(p + 1);
            if ((c1 & 0xC0) != 0x80)
                throw Utf8Error("Byte di continuazione UTF-8 non valido (sequenza a 2 byte).");
            cp = (c & 0x1F) << 6;
            cp |= (c1 & 0x3F);
            p += 2;
        } else if ((c >> 4) == 0xE) { // 3 byte
            if (p + 2 >= end) throw Utf8Error("Stringa UTF-8 troncata (attesi 3 byte).");
            unsigned char c1 = *(p + 1);
            unsigned char c2 = *(p + 2);
            if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80)
                throw Utf8Error("Byte di continuazione UTF-8 non valido (sequenza a 3 byte).");
            cp = (c & 0x0F) << 12;
            cp |= (c1 & 0x3F) << 6;
            cp |= (c2 & 0x3F);
            p += 3;
        } else if ((c >> 3) == 0x1E) { // 4 byte
            if (p + 3 >= end) throw Utf8Error("Stringa UTF-8 troncata (attesi 4 byte).");
            unsigned char c1 = *(p + 1);
            unsigned char c2 = *(p + 2);
            unsigned char c3 = *(p + 3);
            if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
                throw Utf8Error("Byte di continuazione UTF-8 non valido (sequenza a 4 byte).");
            cp = (c & 0x07) << 18;
            cp |= (c1 & 0x3F) << 12;
            cp |= (c2 & 0x3F) << 6;
            cp |= (c3 & 0x3F);
            p += 4;
        } else {
            throw Utf8Error("Byte iniziale UTF-8 non valido.");
        }

        // Escludiamo surrogate e valori fuori Unicode
        if (cp >= 0xD800 && cp <= 0xDFFF)
            throw Utf8Error("Codepoint UTF-8 surrogato non valido.");
        if (cp > 0x10FFFF)
            throw Utf8Error("Codepoint UTF-8 fuori range Unicode.");

        out.push_back(static_cast<char32_t>(cp));
    }

    return out;
}

// Ricodifica una sequenza di codepoint in UTF-8.
inline std::string encodeUtf8(const std::vector<char32_t>& cps) {
    std::string out;
    out.reserve(cps.size()); // stima minima

    for (char32_t cp : cps) {
        uint32_t c = static_cast<uint32_t>(cp);
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
        } else if (c < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (c >> 18)));
            out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }

    return out;
}

// Concatenazione di due stringhe UTF-8
inline std::string utf8Concat(const std::string& s1, const std::string& s2) {
    return s1 + s2;
}

// Estrae una sottostringa (substring) da una stringa UTF-8,
// usando indici in unità di codepoint (non byte).
// start è indice zero-based, length è numero di codepoint da estrarre.
inline std::string utf8Substring(const std::string& s, size_t start, size_t length) {
    auto cps = decodeUtf8(s);

    if (start > cps.size()) start = cps.size();
    if (start + length > cps.size()) length = cps.size() - start;

    std::vector<char32_t> slice(cps.begin() + start, cps.begin() + start + length);
    return encodeUtf8(slice);
}

// Slice simile a substring, ma permette anche indici negativi (per contare da fine)
// start < 0 significa cps.size() + start
// length < 0 significa fino alla fine della stringa
inline std::string utf8Slice(const std::string& s, int start, int length = -1) {
    auto cps = decodeUtf8(s);
    int size = static_cast<int>(cps.size());

    if (start < 0)
        start = size + start;
    if (start < 0)
        start = 0;
    if (start > size)
        start = size;

    if (length < 0 || start + length > size)
        length = size - start;
    if (length < 0)
        length = 0;

    std::vector<char32_t> slice(cps.begin() + start, cps.begin() + start + length);
    return encodeUtf8(slice);
}

// Slice di stringa UTF-8 usando range start:end (incluso/escluso).
// Indici negativi contano dalla fine della stringa.
inline std::string utf8SliceRange(const std::string& s, int start, int end) {
    auto cps = decodeUtf8(s);
    int size = static_cast<int>(cps.size());

    // Normalizza indici negativi
    if (start < 0) start = size + start;
    if (end < 0) end = size + end;

    // Limita agli estremi validi
    if (start < 0) start = 0;
    if (end > size) end = size;
    if (start > end) return ""; // slice vuota

    std::vector<char32_t> slice(cps.begin() + start, cps.begin() + end);
    return encodeUtf8(slice);
}



#endif // MAMMUTH_UTF8_H
