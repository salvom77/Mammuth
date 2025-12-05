#ifndef MAMMUTH_RANGE_H
#define MAMMUTH_RANGE_H

#include <optional>
#include <string>

// Struttura per rappresentare un range parsed
struct RangeInfo {
    std::optional<int> start;  // nullopt = "dall'inizio"
    std::optional<int> end;    // nullopt = "fino alla fine"

    bool isFullCopy() const {
        return !start.has_value() && !end.has_value();
    }

    bool hasStart() const { return start.has_value(); }
    bool hasEnd() const { return end.has_value(); }
};

// Normalizza un indice (gestisce negativi)
// size: dimensione totale della sequenza
// index: indice (può essere negativo)
// returns: indice normalizzato [0, size) oppure -1 se fuori range
inline int normalizeIndex(int index, size_t size) {
    int sz = static_cast<int>(size);

    if (index < 0) {
        index = sz + index;  // -1 diventa size-1, -2 diventa size-2, etc.
    }

    if (index < 0 || index >= sz) {
        return -1;  // fuori range
    }

    return index;
}

// Normalizza un range per slicing
// size: dimensione totale
// range: RangeInfo da normalizzare
// outStart, outEnd: indici normalizzati (output)
// returns: true se valido, false se errore
inline bool normalizeRange(size_t size, const RangeInfo& range,
                          int& outStart, int& outEnd) {
    int sz = static_cast<int>(size);

    // Start
    if (range.hasStart()) {
        outStart = normalizeIndex(range.start.value(), size);
        if (outStart < 0) return false;  // fuori range
    } else {
        outStart = 0;  // dall'inizio
    }

    // End
    if (range.hasEnd()) {
        outEnd = normalizeIndex(range.end.value(), size);
        if (outEnd < 0) return false;  // fuori range
    } else {
        outEnd = sz - 1;  // fino alla fine
    }

    // Verifica: start <= end (no slicing discendente)
    if (outStart > outEnd) {
        return false;
    }

    return true;
}

#endif // MAMMUTH_RANGE_H
