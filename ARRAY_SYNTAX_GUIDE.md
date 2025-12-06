# 📋 MAMMUTH ARRAY SYNTAX - Guida Definitiva

## ✅ SINTASSI CORRETTA (C-Style)

Mammuth usa la sintassi **C/C++ style** per le dichiarazioni array:

```mammoth
int arr[] = 1, 2, 3, 4, 5          # ✅ CORRETTO
double values[] = 1.5, 2.5, 3.5    # ✅ CORRETTO
string names[] = "Alice", "Bob"     # ✅ CORRETTO
```

### Regola:
```
tipo nome[] = elementi
     ^^^^
     Le parentesi quadre vanno DOPO il nome!
```

---

## ❌ SINTASSI SBAGLIATA (Java/C# Style)

**Queste NON funzionano in Mammuth:**

```mammoth
int[] arr = 1, 2, 3        # ❌ SBAGLIATO
double[] values = 1.5, 2.5 # ❌ SBAGLIATO
string[] names = "Alice"   # ❌ SBAGLIATO
```

**Errore che ottieni:**
```
Errore: atteso nome variabile
Token inatteso in parsePrimary: ]
```

---

## 🎯 PERCHÉ C-STYLE?

### Vantaggi:
1. ✅ **Familiare** - Milioni di programmatori C/C++ la conoscono
2. ✅ **Chiara** - Mostra che il nome è un array
3. ✅ **Standard** - Convenzione consolidata da 50+ anni
4. ✅ **Consistente** - C, C++, Go usano questa sintassi

### Confronto:
```c
// C/C++
int arr[] = {1, 2, 3};

// Go
arr := []int{1, 2, 3}

// Mammuth (stile C semplificato)
int arr[] = 1, 2, 3
```

---

## 📚 ESEMPI COMPLETI

### Dichiarazione Base
```mammoth
int numbers[] = 1, 2, 3, 4, 5
double prices[] = 9.99, 19.99, 29.99
string words[] = "hello", "world", "mammuth"
```

### Array Dinamici
```mammoth
dynamic int scores[] = 100, 200, 300
scores[0] = 999  # Modifica permessa
```

### Array con Operazioni
```mammoth
int a[] = 1, 2, 3
int b[] = 4, 5, 6
int combined[] = a $ b  # Concatenazione

echo str(combined[0])   # 1
echo str(combined[5])   # 6
```

### Array con Slicing
```mammoth
int nums[] = 10, 20, 30, 40, 50

int slice1[] = nums[1..3]  # [20, 30, 40]
int slice2[] = nums[2..]   # [30, 40, 50]
int slice3[] = nums[..2]   # [10, 20, 30]
```

### Array con Filter
```mammoth
int numbers[] = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10

int evens[] = numbers => x % 2 == 0
# evens = [2, 4, 6, 8, 10]

int big[] = numbers => x > 5
# big = [6, 7, 8, 9, 10]
```

### Array con For-In
```mammoth
int items[] = 100, 200, 300, 400

for item in items::
    echo "Item: " $ str(item)
end

# Output:
# Item: 100
# Item: 200
# Item: 300
# Item: 400
```

### Array Nested (Future)
```mammoth
# Sintassi per array 2D (quando implementato)
int matrix[][] = ...  # Da definire in v3.7+
```

---

## 🔍 ACCESSO ELEMENTI

### Indice Positivo
```mammoth
int arr[] = 10, 20, 30, 40, 50

echo str(arr[0])   # 10 (primo)
echo str(arr[1])   # 20
echo str(arr[4])   # 50 (ultimo)
```

### Indice Negativo
```mammoth
echo str(arr[-1])  # 50 (ultimo)
echo str(arr[-2])  # 40 (penultimo)
echo str(arr[-5])  # 10 (primo)
```

---

## 🛠️ FUNZIONI BUILT-IN

```mammoth
int arr[] = 1, 2, 3, 4, 5

# Lunghezza
echo str(len(arr))              # 5

# Primo elemento
echo str(array_first(arr))      # 1

# Ultimo elemento
echo str(array_last(arr))       # 5

# Tipo
echo typeOf(arr)                # "array"
```

---

## 📖 RIFERIMENTO RAPIDO

| Operazione | Sintassi | Esempio |
|------------|----------|---------|
| Dichiarazione | `tipo nome[] = ...` | `int arr[] = 1, 2, 3` |
| Accesso | `nome[indice]` | `arr[0]` |
| Slicing | `nome[start..end]` | `arr[1..3]` |
| Negativo | `nome[-indice]` | `arr[-1]` |
| Concatenazione | `arr1 $ arr2` | `a $ b` |
| Filter | `arr => condizione` | `arr => x > 5` |
| For-in | `for x in arr::` | `for n in nums::` |
| Lunghezza | `len(arr)` | `len(numbers)` |

---

## ⚠️ ERRORI COMUNI

### Errore 1: Sintassi Java-style
```mammoth
int[] arr = 1, 2, 3  # ❌ SBAGLIATO
int arr[] = 1, 2, 3  # ✅ CORRETTO
```

### Errore 2: Virgole sbagliate
```mammoth
int arr[] = [1, 2, 3]     # ❌ NO parentesi quadre
int arr[] = 1, 2, 3       # ✅ CORRETTO
```

### Errore 3: Dichiarazione senza elementi
```mammoth
int arr[]                 # ❌ Mancano gli elementi
int arr[] = 1, 2, 3       # ✅ CORRETTO
```

---

## 🎓 BEST PRACTICES

### 1. Nomi Descrittivi
```mammoth
# ✅ Buono
int scores[] = 100, 200, 300
string names[] = "Alice", "Bob"

# ❌ Evita
int a[] = 1, 2, 3
string s[] = "x", "y"
```

### 2. Usa Dynamic Solo Se Necessario
```mammoth
# Se non modifichi
int fixed_data[] = 1, 2, 3

# Se modifichi
dynamic int mutable_data[] = 1, 2, 3
```

### 3. Filter per Chiarezza
```mammoth
# ❌ Loop manuale
dynamic int evens[] = 
for n in numbers::
    if n % 2 == 0::
        # come aggiungere?
    end
end

# ✅ Filter elegante
int evens[] = numbers => x % 2 == 0
```

---

## 🚀 PROSSIME FEATURE (Roadmap)

### v3.7 - Mask Operators
```mammoth
# Pattern matching avanzato
int matches[] = numbers mask (x > 5 and x < 10)
```

### v3.8 - Array Multidimensionali
```mammoth
# Sintassi da definire
int matrix[][] = ...
```

---

## 📞 SUMMARY

**RICORDA:**
```mammoth
# ✅ SEMPRE COSÌ
tipo nome[] = elementi

# ❌ MAI COSÌ
tipo[] nome = elementi
```

**Mammuth = C-style arrays!** 🦣


