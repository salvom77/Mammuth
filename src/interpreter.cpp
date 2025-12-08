#include "interpreter.h"
#include "debug.h"
#include "value.h"
#include "range.h"  // ⭐ NUOVO
#include "utf8.h"   // per slicing stringhe UTF-8

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdlib>  // rand(), srand()
#include <ctime>    // time()
#include <memory>

// =======================
// Utility di tipo
// =======================

static std::string typeOfValue(const Value& v) {
    if (isType<int>(v))            return "int";
    if (isType<double>(v))         return "double";
    if (isType<std::string>(v))    return "string";
    if (isType<FunctionValue>(v))  return "func";
    if (isType<ArrayValue>(v))     return "array";
    return "unknown";
}

// helper per creare un ArrayValue di N zeri
static ArrayValue makeArrayOfSize(int size) {
    ArrayValue arr;
    if (size < 0) size = 0;
    for (int i = 0; i < size; ++i) {
        arr.push_back(std::make_shared<Value>(0));
    }
    return arr;
}

// helper per espandere inizializzatori di array che contengono CommaList
static void appendArrayInitExpr(Interpreter* interp,
                                ArrayValue& out,
                                const std::shared_ptr<ASTNode>& expr)
{
    if (!expr) return;

    // Se l'espressione è una lista separata da virgole, espandi ricorsivamente
    if (expr->type == "CommaList") {
        for (auto& sub : expr->children) {
            appendArrayInitExpr(interp, out, sub);
        }
        return;
    }

    // Caso base: espressione normale → valuta e aggiungi all'array
    Value v = interp->eval(expr);

    // ⭐ NUOVO: Se il valore è un array, espandi i suoi elementi
    if (isType<ArrayValue>(v)) {
        auto& arr = as<ArrayValue>(v);
        for (auto& elem : arr.elements) {
            out.push_back(elem);  // Aggiungi ogni elemento dell'array
        }
    } else {
        out.push_back(std::make_shared<Value>(v));
    }
}



// =======================
// Interpreter: scope
// =======================

Interpreter::Interpreter() {
    Scope* global = new Scope(nullptr);
    scopes.push_back(global);
    DEBUG_INTERP_LOG("Interpreter creato, scope globale inizializzato");
}

Interpreter::~Interpreter() {
    for (auto* s : scopes) delete s;
}

Scope& Interpreter::currentScope() {
    return *scopes.back();
}

void Interpreter::pushScope() {
    Scope* parent = scopes.empty() ? nullptr : scopes.back();
    Scope* s = new Scope(parent);
    scopes.push_back(s);
    DEBUG_SCOPE_LOG("pushScope, depth=" << scopes.size());
}

void Interpreter::popScope() {
    if (scopes.size() <= 1) {
        DEBUG_SCOPE_LOG("popScope su globale ignorato");
        return;
    }
    DEBUG_SCOPE_LOG("popScope, depth=" << scopes.size());
    Scope* s = scopes.back();
    scopes.pop_back();
    delete s;
}

Value Interpreter::lookup(const std::string& name) {
    // Prima cerca in variabili
    StoredVar* sv = currentScope().lookup(name);
    if (sv) {
        DEBUG_SCOPE_LOG("lookup '" << name << "' OK");
        return sv->value;
    }
    
    // Poi cerca in funzioni locali (nested)
    auto localFunc = currentScope().lookupLocalFunction(name);
    if (localFunc) {
        DEBUG_SCOPE_LOG("lookup function '" << name << "' OK (local)");
        
        // Converti FunctionDef in FunctionValue
        FunctionValue fv;
        
        // Estrai parametri
        for (auto& child : localFunc->children) {
            if (child->type == "Param") {
                fv.params.push_back(child->value);
            } else if (child->type == "Body") {
                fv.body = child;
            }
        }
        
        // Cattura variabili da scope corrente (closure)
        Scope& current = currentScope();
        for (const auto& pair : current.vars) {
            fv.capturedVars[pair.first] = pair.second.value;
        }
        
        // Cattura anche da parent scopes
        Scope* parent = current.parent;
        while (parent) {
            for (const auto& pair : parent->vars) {
                if (fv.capturedVars.find(pair.first) == fv.capturedVars.end()) {
                    fv.capturedVars[pair.first] = pair.second.value;
                }
            }
            parent = parent->parent;
        }
        
        return fv;
    }
    
    // ============================================
    // NUOVO: Cerca in funzioni globali (def top-level)
    // Questo permette a if/else di ritornare riferimenti a funzioni
    // ============================================
    auto it = functions.find(name);
    if (it != functions.end()) {
        DEBUG_SCOPE_LOG("lookup function '" << name << "' OK (global)");
        
        // Converti FunctionDef in FunctionValue
        FunctionValue fv;
        auto funcDef = it->second;
        
        // Estrai parametri
        for (auto& child : funcDef->children) {
            if (child->type == "Param") {
                fv.params.push_back(child->value);
            } else if (child->type == "Body") {
                fv.body = child;
            }
        }
        
        // Cattura variabili da scope corrente (per closure)
        Scope& current = currentScope();
        for (const auto& pair : current.vars) {
            fv.capturedVars[pair.first] = pair.second.value;
        }
        
        // Cattura anche da parent scopes
        Scope* parent = current.parent;
        while (parent) {
            for (const auto& pair : parent->vars) {
                if (fv.capturedVars.find(pair.first) == fv.capturedVars.end()) {
                    fv.capturedVars[pair.first] = pair.second.value;
                }
            }
            parent = parent->parent;
        }
        
        return fv;
    }
    
    DEBUG_SCOPE_LOG("lookup '" << name << "' non trovato, ritorno 0");
    return 0;
}

void Interpreter::defineVar(const std::string& name, const Value& v, bool isDynamic, bool isFixed) {
    StoredVar sv;
    sv.value = v;
    sv.isDynamic = isDynamic;
    sv.isFixed = isFixed;
    currentScope().define(name, sv);
    DEBUG_SCOPE_LOG("defineVar '" << name << "', dynamic=" << (isDynamic ? "true" : "false") 
                    << ", fixed=" << (isFixed ? "true" : "false"));
}

void Interpreter::setVar(const std::string& name, const Value& v) {
    StoredVar* sv = currentScope().lookup(name);
    if (!sv) {
        defineVar(name, v, false, false);  // Nuova variabile: non dynamic, non fixed
        return;
    }
    
    // Controlla se è fixed
    if (sv->isFixed) {
        // Messaggio specifico per variabili funzione
        if (isType<FunctionValue>(sv->value)) {
            runtimeError(nullptr, 
                "Impossibile riassegnare variabile funzione '" + name + "'\n" +
                "Le funzioni sono immutabili per natura.\n" +
                "Suggerimento: Crea una nuova variabile con un nome diverso.");
        } else {
            runtimeError(nullptr, "Impossibile riassegnare variabile 'fixed': " + name);
        }
        return;
    }
    
    // Per array, controlla se è dynamic
    if (isType<ArrayValue>(sv->value) && !sv->isDynamic) {
        runtimeError(nullptr, "Array '" + name + "' non è dynamic, non può essere riassegnato");
        return;
    }
    
    sv->value = v;
}

// =======================
// Helper semantici
// =======================

bool Interpreter::isTruthy(const Value& v) const {
    if (isType<int>(v))
        return as<int>(const_cast<Value&>(v)) != 0;

    if (isType<double>(v))
        return as<double>(const_cast<Value&>(v)) != 0.0;

    if (isType<std::string>(v))
        return !as<std::string>(const_cast<Value&>(v)).empty();

    if (isType<ArrayValue>(v))
        return !as<ArrayValue>(const_cast<Value&>(v)).empty();

    return false;
}

std::string Interpreter::toString(const Value& v) const {
    if (isType<int>(v))
        return std::to_string(as<int>(const_cast<Value&>(v)));

    if (isType<double>(v)) {
        std::ostringstream oss;
        oss << as<double>(const_cast<Value&>(v));
        return oss.str();
    }

    if (isType<std::string>(v))
        return as<std::string>(const_cast<Value&>(v));

    if (isType<ArrayValue>(v)) {
        std::ostringstream oss;
        const auto& arr = as<ArrayValue>(const_cast<Value&>(v));
        oss << "[";
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i > 0) oss << ", ";
            if (arr[i]) {
                oss << toString(*arr[i]);
            } else {
                oss << "0";
            }
        }
        oss << "]";
        return oss.str();
    }

    if (isType<FunctionValue>(v))
        return "<function>";

    return "0";
}

void Interpreter::runtimeError(const ASTNode* node, const std::string& msg) const {
    if (node)
        std::cerr << "Errore (riga " << node->line
                  << ", colonna " << node->column
                  << "): " << msg << "\n";
    else
        std::cerr << "Errore: " << msg << "\n";
}

void Interpreter::printValue(const Value& v) const {
    std::cout << toString(v);
}

// =======================
// ⭐ NUOVO: Parsing RangeExpr → RangeInfo
// =======================

RangeInfo Interpreter::parseRangeNode(const std::shared_ptr<ASTNode>& node) {
    RangeInfo range;

    bool hasStart = (node->extra.count("hasStart") &&
                     node->extra.at("hasStart") == "true");

    bool hasEnd = (node->extra.count("hasEnd") &&
                   node->extra.at("hasEnd") == "true");

    size_t childIdx = 0;

    // -----------------------------
    // START
    // -----------------------------
    if (hasStart && childIdx < node->children.size()) {

        Value startVal = eval(node->children[childIdx++]);

        if (!isType<int>(startVal)) {
            runtimeError(node.get(),
                "Indice start del range deve essere int");

            // Range invalido → lascia nullopt (or invalida tutto)
            range.start.reset();
            range.end.reset();
            return range;
        }

        range.start = as<int>(startVal);
    }

    // -----------------------------
    // END
    // -----------------------------
    if (hasEnd && childIdx < node->children.size()) {

        Value endVal = eval(node->children[childIdx]);

        if (!isType<int>(endVal)) {
            runtimeError(node.get(),
                "Indice end del range deve essere int");

            range.start.reset();
            range.end.reset();
            return range;
        }

        range.end = as<int>(endVal);
    }

    return range;
}


// =======================
// ⭐ NUOVO: Slicing di stringhe UTF-8
// =======================

Value Interpreter::sliceString(const std::string& s,
                               const RangeInfo& range,
                               const ASTNode* node)
{
    // 1) Decodifica UTF-8 in codepoints
    std::vector<char32_t> cps;
    try {
        cps = decodeUtf8(s);
    } catch (const Utf8Error& e) {
        runtimeError(node, std::string("Errore UTF-8: ") + e.what());
        return "";
    }

    if (cps.empty())
        return std::string("");

    // 2) Normalizzazione indici
    int start = 0, end = 0;
    if (!normalizeRange(cps.size(), range, start, end)) {
        runtimeError(node, "Range non valido per slicing stringa");
        return "";
    }

    // 3) Costruzione nuovo vettore
    std::vector<char32_t> out;
    out.reserve(end - start + 1);

    for (int i = start; i <= end; ++i)
        out.push_back(cps[i]);

    // 4) Ricodifica UTF-8
    try {
        return encodeUtf8(out);
    } catch (const Utf8Error& e) {
        runtimeError(node, std::string("Errore UTF-8: ") + e.what());
        return "";
    }
}


// =======================
// ⭐ NUOVO: Slicing di array
// =======================

Value Interpreter::sliceArray(const ArrayValue& arr,
                              const RangeInfo& range,
                              const ASTNode* node)
{
    size_t size = arr.size();
    if (size == 0)
        return ArrayValue{};  // array vuoto

    int start = 0, end = 0;

    if (!normalizeRange(size, range, start, end)) {
        runtimeError(node, "Range non valido per slicing array");
        return ArrayValue{};
    }

    // nuovo array immutabile
    ArrayValue out;
    out.elements.reserve(end - start + 1);

    for (int i = start; i <= end; ++i) {
        auto& elemPtr = arr.elements[i];
        if (!elemPtr)
            out.elements.push_back(std::make_shared<Value>(0));
        else
            out.elements.push_back(std::make_shared<Value>(*elemPtr));
    }

    return out;
}






// =======================
// EVAL
// =======================

Value Interpreter::eval(const std::shared_ptr<ASTNode>& node) {
    if (!node) return 0;

    const std::string& t = node->type;

    // -------- Literal --------
    if (t == "Literal") {
        if (node->tokenType == TokenType::NUMBER_INT) {
            return std::stoi(node->value);
        } else if (node->tokenType == TokenType::NUMBER_DBL) {
            return std::stod(node->value);
        } else if (node->tokenType == TokenType::STRING) {
            return node->value;
        }

        bool isNumber = !node->value.empty() &&
                        std::all_of(node->value.begin(), node->value.end(),
                                    [](char c){ return (c >= '0' && c <= '9'); });
        if (isNumber) return std::stoi(node->value);
        return node->value;
    }

    // -------- Identifier --------
    if (t == "Identifier") {
        return lookup(node->value);
    }

    // -------- Lambda --------
    if (t == "Lambda") {
        FunctionValue fv;
        
        // Estrai parametri (primi children sono Param)
        size_t bodyIdx = 0;
        for (size_t i = 0; i < node->children.size(); ++i) {
            if (node->children[i]->type == "Param") {
                fv.params.push_back(node->children[i]->value);
            } else {
                bodyIdx = i;
                break;
            }
        }
        
        // Body
        if (bodyIdx < node->children.size()) {
            fv.body = node->children[bodyIdx];
        }
        
        // ============================================
        // CATTURA CLOSURE: copia TUTTE le variabili dello scope corrente
        // Questo permette alla lambda di accedere alle variabili
        // anche dopo che lo scope outer è stato poppato
        // ============================================
        Scope& current = currentScope();
        for (const auto& pair : current.vars) {
            fv.capturedVars[pair.first] = pair.second.value;
        }
        
        // Cattura anche variabili da parent scopes
        Scope* parent = current.parent;
        while (parent) {
            for (const auto& pair : parent->vars) {
                // Non sovrascrivere se già catturata da scope più vicino
                if (fv.capturedVars.find(pair.first) == fv.capturedVars.end()) {
                    fv.capturedVars[pair.first] = pair.second.value;
                }
            }
            parent = parent->parent;
        }
        
        // Closure scope (deprecato, ma lo mantengo per compatibilità)
        fv.closureScope = &currentScope();
        
        return fv;
    }

    // -------- IfExpr (v3.5) --------
    if (t == "IfExpr") {
        if (node->children.size() < 2) {
            runtimeError(node.get(), "IfExpr malformato");
            return 0;
        }
        
        int elifCount = 0;
        bool hasElse = false;
        
        if (node->extra.count("elifCount")) {
            elifCount = std::stoi(node->extra.at("elifCount"));
        }
        if (node->extra.count("hasElse")) {
            hasElse = (node->extra.at("hasElse") == "true");
        }
        
        // Eval main if condition
        Value condValue = eval(node->children[0]);
        if (isTruthy(condValue)) {
            // Eval then body
            return eval(node->children[1]);
        }
        
        // Eval elif branches
        int childIdx = 2; // Start after condition and thenBody
        for (int i = 0; i < elifCount; i++) {
            if (childIdx + 1 >= node->children.size()) break;
            
            Value elifCondValue = eval(node->children[childIdx]);
            if (isTruthy(elifCondValue)) {
                // Eval elif body
                return eval(node->children[childIdx + 1]);
            }
            childIdx += 2; // Skip condition + body
        }
        
        // Eval else branch if present
        if (hasElse) {
            int elseIdx = 2 + (elifCount * 2); // After all if/elif pairs
            if (elseIdx < node->children.size()) {
                return eval(node->children[elseIdx]);
            }
        }
        
        // No branch matched, return 0
        return 0;
    }

    // -------- While --------
    if (t == "While") {
        if (node->children.size() < 2) {
            runtimeError(node.get(), "While malformato");
            return 0;
        }
        
        auto condNode = node->children[0];
        auto bodyNode = node->children[1];
        
        // Variabile di return (opzionale)
        std::string returnVar = node->extra.count("returnVar") 
                              ? node->extra.at("returnVar") 
                              : "";
        
        Value lastVal = 0;
        
        while (isTruthy(eval(condNode))) {
            eval(bodyNode);
            
            // Se c'è returnVar, leggi il suo valore
            if (!returnVar.empty()) {
                lastVal = lookup(returnVar);
            }
        }
        
        return returnVar.empty() ? 0 : lastVal;
    }

    // -------- ForIn --------
    if (t == "ForIn") {
        if (node->children.size() < 2) {
            runtimeError(node.get(), "ForIn malformato");
            return 0;
        }
        
        std::string iterVar = node->value;
        auto collectionNode = node->children[0];
        auto bodyNode = node->children[1];
        
        // Variabile di return (opzionale)
        std::string returnVar = node->extra.count("returnVar") 
                              ? node->extra.at("returnVar") 
                              : "";
        
        Value collection = eval(collectionNode);
        
        if (!isType<ArrayValue>(collection)) {
            runtimeError(node.get(), "for-in richiede un array");
            return 0;
        }
        
        auto& arr = as<ArrayValue>(collection);
        Value lastVal = 0;
        
        for (auto& elem : arr.elements) {
            // Definisci variabile iteratore
            defineVar(iterVar, *elem, false, false);
            
            eval(bodyNode);
            
            // Se c'è returnVar, leggi il suo valore
            if (!returnVar.empty()) {
                lastVal = lookup(returnVar);
            }
        }
        
        return returnVar.empty() ? 0 : lastVal;
    }

    // -------- CommaList --------
    if (t == "CommaList") {
        Value last = 0;
        for (auto& ch : node->children) last = eval(ch);
        return last;
    }

    // -------- Assign (nuovo) --------
    if (t == "Assign") {
        return evalAssignment(node);
    }

   // -------- BinaryOp / LogicalOp --------
    if (t == "BinaryOp" || t == "LogicalOp") {

        // ⭐ Caso speciale: "$" con RangeExpr a destra
        if (t == "BinaryOp" &&
            node->value == "$" &&
            node->children.size() == 2 &&
            node->children[1] &&
            node->children[1]->type == "RangeExpr")
        {
            // 1) valuta solo il left
            Value leftVal = eval(node->children[0]);

            // 2) parsa il range dalla AST (usa già eval per start/end)
            RangeInfo range = parseRangeNode(node->children[1]);

            // 2b) VALIDAZIONE PRE-SLICE (Opzione A: abort on invalid)
            // per stringhe: dobbiamo conoscere la lunghezza in codepoints
            if (isType<std::string>(leftVal)) {
                try {
                    auto cps = decodeUtf8(as<std::string>(leftVal));
                    int start, end;
                    if (!normalizeRange(cps.size(), range, start, end)) {
                        runtimeError(node.get(), "Range invalido per stringa durante concatenazione '$' (abort)");
                        return 0;
                    }
                } catch (const Utf8Error& e) {
                    runtimeError(node.get(), std::string("Errore UTF-8: ") + e.what());
                    return 0;
                }
            }
            // per array: lunghezza elementi
            else if (isType<ArrayValue>(leftVal)) {
                auto& arr = as<ArrayValue>(leftVal);
                int start, end;
                if (!normalizeRange(arr.size(), range, start, end)) {
                    runtimeError(node.get(), "Range invalido per array durante concatenazione '$' (abort)");
                    return 0;
                }
            }
            else {
                runtimeError(node.get(),
                    "Range dopo '$' supportato solo su stringhe e array");
                return 0;
            }

            // 3) applica lo slice SU left (sliceString/sliceArray ritornano Value)
            Value rightVal;
            if (isType<std::string>(leftVal)) {
                rightVal = sliceString(as<std::string>(leftVal), range, node.get());
                // sliceString segnalerà errori con runtimeError; per Opzione A abbiamo già validato
            } else { // array
                rightVal = sliceArray(as<ArrayValue>(leftVal), range, node.get());
            }

            // 4) esegui normalmente l'operatore "$" sui due valori
            return evalBinaryOp(node->value, leftVal, rightVal, node.get());
        }

        // caso normale per tutti gli altri operatori
        Value left  = eval(node->children[0]);
        Value right = eval(node->children[1]);
        return evalBinaryOp(node->value, left, right, node.get());
    }


    // -------- UnaryOp --------
    if (t == "UnaryOp") {
        Value v = eval(node->children[0]);
        return evalUnaryOp(node->value, v, node.get());
    }

    // -------- CondChain --------
    if (t == "CondChain") {
        return evalCondChain(node);
    }

    // -------- SimpleCond --------
    // SimpleCond viene usato dentro CondChain, ma può anche apparire standalone
    // in alcune situazioni di parsing multi-line
    if (t == "SimpleCond") {
        // SimpleCond ha 2 figli:
        // [0] = condizione
        // [1] = espressione se vera
        if (node->children.size() < 2) {
            runtimeError(node.get(), "SimpleCond richiede 2 figli (condizione, espressione)");
            return 0;
        }
        
        Value condVal = eval(node->children[0]);
        if (isTruthy(condVal)) {
            return eval(node->children[1]);
        }
        // Se falso, restituisce 0 (o potrebbe essere un errore)
        return 0;
    }

    if (t == "Elvis") {
        return evalElvis(node);
    }

    if (t == "Filter") {
        return evalFilter(node);
    }

    // -------- ArrayAccess --------
    if (t == "ArrayAccess") {
        Value arrayVal;
        size_t idxNodePos = 0;
        
        // ============================================
        // Nuovo formato: expr[index]
        // value="" e children=[expr, index]
        // ============================================
        if (node->value.empty() && !node->children.empty()) {
            // Evalua espressione left
            arrayVal = eval(node->children[0]);
            idxNodePos = 1;
        } 
        // ============================================
        // Old formato: varname[index]
        // value="varname" e children=[index]
        // ============================================
        else {
            std::string name = node->value;
            StoredVar* sv = currentScope().lookup(name);
            if (!sv) {
                runtimeError(node.get(), "Variabile '" + name + "' non definita");
                return 0;
            }
            arrayVal = sv->value;
            idxNodePos = 0;
        }
        
        auto idxNode = node->children[idxNodePos];

        // Range
        if (idxNode->type == "RangeExpr") {
            RangeInfo range = parseRangeNode(idxNode);

            // String slice
            if (isType<std::string>(arrayVal)) {
                try {
                    auto cps = decodeUtf8(as<std::string>(arrayVal));
                    int start, end;
                    if (!normalizeRange(cps.size(), range, start, end)) {
                        runtimeError(node.get(), "Range invalido per stringa (abort)");
                        return 0;
                    }
                } catch (const Utf8Error& e) {
                    runtimeError(node.get(), std::string("Errore UTF-8: ") + e.what());
                    return 0;
                }
                return sliceString(as<std::string>(arrayVal), range, node.get());
            }

            // Array slice
            if (isType<ArrayValue>(arrayVal)) {
                auto& arr = as<ArrayValue>(arrayVal);
                int start, end;
                if (!normalizeRange(arr.size(), range, start, end)) {
                    runtimeError(node.get(), "Range invalido per array (abort)");
                    return 0;
                }
                return sliceArray(as<ArrayValue>(arrayVal), range, node.get());
            }

            runtimeError(node.get(), "Slicing supportato solo su stringhe e array");
            return 0;
        }

        // Indice singolo
        Value idxV = eval(idxNode);
        if (!isType<int>(idxV)) {
            runtimeError(node.get(), "Indice deve essere int");
            return 0;
        }
        int idx = as<int>(idxV);

        // Stringa → singolo carattere
        if (isType<std::string>(arrayVal)) {
            try {
                auto codepoints = decodeUtf8(as<std::string>(arrayVal));
                int normIdx = normalizeIndex(idx, codepoints.size());
                if (normIdx < 0) {
                    runtimeError(node.get(), "Indice stringa fuori limite");
                    return "";
                }
                return encodeUtf8({codepoints[normIdx]});
            } catch (const Utf8Error& e) {
                runtimeError(node.get(), std::string("Errore UTF-8: ") + e.what());
                return "";
            }
        }

        // Array
        if (isType<ArrayValue>(arrayVal)) {
            auto& arr = as<ArrayValue>(arrayVal);
            int normIdx = normalizeIndex(idx, arr.size());
            if (normIdx < 0) {
                runtimeError(node.get(), "Indice array fuori limite");
                return 0;
            }
            if (!arr[(size_t)normIdx]) return 0;
            return *arr[(size_t)normIdx];
        }

        runtimeError(node.get(), "Valore non indicizzabile (richiesto array o stringa)");
        return 0;
    }

    // -------- RangeExpr standalone --------
    if (t == "RangeExpr") {
        runtimeError(node.get(), "Range non può essere valutato direttamente (serve un target)");
        return 0;
    }

    // -------- Call --------
    if (t == "Call") {
        std::string fname = node->value;
        
        // ============================================
        // FIRST-CLASS FUNCTION CALL
        // Usa Interpreter::lookup() che cerca:
        // 1. Variabili (incluse quelle con FunctionValue)
        // 2. Funzioni locali (nested)
        // 3. Funzioni globali (def top-level) ← NUOVO v3.5.1!
        // ============================================
        Value fnameValue = lookup(fname);  // ← Usa Interpreter::lookup()!
        
        if (isType<FunctionValue>(fnameValue)) {
            auto& fv = as<FunctionValue>(fnameValue);
            
            // Valuta argomenti
            ArrayValue args;
            for (auto& ch : node->children)
                args.push_back(std::make_shared<Value>(eval(ch)));
            
            // Controlla numero argomenti
            if (args.size() != fv.params.size()) {
                runtimeError(node.get(), "Numero argomenti errato per funzione first-class");
                return 0;
            }
            
            // ============================================
            // FUNZIONE COMPOSTA: f $ g
            // ============================================
            if (!fv.composedFuncs.empty()) {
                // Esegui composizione: (f $ g)(x) = g(f(x))
                Value result = *args[0];  // Valore iniziale
                
                // Applica ogni funzione in sequenza
                for (auto& funcPtr : fv.composedFuncs) {
                    const FunctionValue& func = *funcPtr;
                    
                    // Crea scope temporaneo
                    pushScope();
                    
                    // Ripristina variabili catturate
                    for (const auto& pair : func.capturedVars) {
                        defineVar(pair.first, pair.second, false, false);
                    }
                    
                    // Bind parametro
                    defineVar(func.params[0], result, false, false);
                    
                    // Esegui funzione
                    result = eval(func.body);
                    
                    popScope();
                }
                
                return result;
            }
            
            // ============================================
            // FUNZIONE NORMALE
            // ============================================
            
            // Crea nuovo scope
            pushScope();
            
            // ============================================
            // RIPRISTINA VARIABILI CATTURATE (closure)
            // Questo permette alla funzione di accedere alle variabili
            // dello scope in cui è stata definita
            // ============================================
            for (const auto& pair : fv.capturedVars) {
                defineVar(pair.first, pair.second, false, false);
            }
            
            // Bind parametri (possono sovrascrivere variabili catturate)
            for (size_t i = 0; i < fv.params.size(); ++i) {
                defineVar(fv.params[i], *args[i], false, false);
            }
            
            // Esegui body
            Value ret = eval(fv.body);
            
            popScope();
            
            return ret;
        }
        
        // ============================================
        // BUILT-IN FUNCTIONS
        // ============================================
        
        // --- str() ---
        if (fname == "str") {
            if (node->children.size() != 1) {
                runtimeError(node.get(), "str() richiede esattamente 1 argomento");
                return "";
            }
            Value arg = eval(node->children[0]);
            return toString(arg);
        }
        
        // --- len() ---
        if (fname == "len") {
            if (node->children.size() != 1) {
                runtimeError(node.get(), "len() richiede esattamente 1 argomento");
                return 0;
            }
            Value arg = eval(node->children[0]);
            if (isType<std::string>(arg)) {
                return static_cast<int>(decodeUtf8(as<std::string>(arg)).size());
            }
            if (isType<ArrayValue>(arg)) {
                return static_cast<int>(as<ArrayValue>(arg).size());
            }
            runtimeError(node.get(), "len() supporta solo string e array");
            return 0;
        }
        
        // --- randInt(min, max) → int in [min, max) ---
        if (fname == "randInt") {
            if (node->children.size() != 2) {
                runtimeError(node.get(), "randInt() richiede 2 argomenti (min, max)");
                return 0;
            }
            Value minVal = eval(node->children[0]);
            Value maxVal = eval(node->children[1]);
            
            if (!isType<int>(minVal) || !isType<int>(maxVal)) {
                runtimeError(node.get(), "randInt(): argomenti devono essere int");
                return 0;
            }
            
            int min = as<int>(minVal);
            int max = as<int>(maxVal);
            
            if (min >= max) {
                runtimeError(node.get(), "randInt(): min deve essere < max");
                return 0;
            }
            
            // Generate random int in [min, max)
            static bool seeded = false;
            if (!seeded) {
                std::srand(static_cast<unsigned>(std::time(nullptr)));
                seeded = true;
            }
            
            int range = max - min;
            int randomInt = min + (std::rand() % range);
            return randomInt;
        }
        
        // --- randDouble() → double in [0.0, 1.0) ---
        if (fname == "randDouble") {
            if (node->children.size() != 0) {
                runtimeError(node.get(), "randDouble() non accetta argomenti");
                return 0;
            }
            
            // Generate random double in [0.0, 1.0)
            static bool seeded = false;
            if (!seeded) {
                std::srand(static_cast<unsigned>(std::time(nullptr)));
                seeded = true;
            }
            
            double randomDouble = static_cast<double>(std::rand()) / RAND_MAX;
            return randomDouble;
        }
        
        // --- array_push() ---
        if (fname == "array_push") {
            if (node->children.size() != 2) {
                runtimeError(node.get(), "array_push() richiede 2 argomenti (array, value)");
                return 0;
            }
            
            // Primo arg deve essere identifier (nome array)
            if (node->children[0]->type != "Identifier") {
                runtimeError(node.get(), "array_push(): primo argomento deve essere nome array");
                return 0;
            }
            
            std::string arrName = node->children[0]->value;
            auto sv = currentScope().lookup(arrName);
            if (!sv) {
                runtimeError(node.get(), "Array '" + arrName + "' non definito");
                return 0;
            }
            
            if (!isType<ArrayValue>(sv->value)) {
                runtimeError(node.get(), "'" + arrName + "' non è un array");
                return 0;
            }
            
            if (!sv->isDynamic) {
                runtimeError(node.get(), "Array '" + arrName + "' non è dynamic");
                return 0;
            }
            
            Value newVal = eval(node->children[1]);
            as<ArrayValue>(sv->value).push_back(std::make_shared<Value>(newVal));
            return 0;
        }
        
        // --- array_pop() ---
        if (fname == "array_pop") {
            if (node->children.size() != 1) {
                runtimeError(node.get(), "array_pop() richiede 1 argomento (array)");
                return 0;
            }
            
            if (node->children[0]->type != "Identifier") {
                runtimeError(node.get(), "array_pop(): argomento deve essere nome array");
                return 0;
            }
            
            std::string arrName = node->children[0]->value;
            auto sv = currentScope().lookup(arrName);
            if (!sv) {
                runtimeError(node.get(), "Array '" + arrName + "' non definito");
                return 0;
            }
            
            if (!isType<ArrayValue>(sv->value)) {
                runtimeError(node.get(), "'" + arrName + "' non è un array");
                return 0;
            }
            
            if (!sv->isDynamic) {
                runtimeError(node.get(), "Array '" + arrName + "' non è dynamic");
                return 0;
            }
            
            auto& arr = as<ArrayValue>(sv->value);
            if (arr.empty()) {
                runtimeError(node.get(), "array_pop(): array vuoto");
                return 0;
            }
            
            Value ret = *arr.elements.back();
            arr.elements.pop_back();
            return ret;
        }
        
        // --- array_length() ---
        if (fname == "array_length") {
            if (node->children.size() != 1) {
                runtimeError(node.get(), "array_length() richiede 1 argomento");
                return 0;
            }
            Value arg = eval(node->children[0]);
            if (!isType<ArrayValue>(arg)) {
                runtimeError(node.get(), "array_length() supporta solo array");
                return 0;
            }
            return static_cast<int>(as<ArrayValue>(arg).size());
        }
        
        // --- array_first() ---
        if (fname == "array_first") {
            if (node->children.size() != 1) {
                runtimeError(node.get(), "array_first() richiede 1 argomento");
                return 0;
            }
            Value arg = eval(node->children[0]);
            if (!isType<ArrayValue>(arg)) {
                runtimeError(node.get(), "array_first() supporta solo array");
                return 0;
            }
            auto& arr = as<ArrayValue>(arg);
            if (arr.empty()) {
                runtimeError(node.get(), "array_first(): array vuoto");
                return 0;
            }
            return *arr.elements[0];
        }
        
        // --- array_last() ---
        if (fname == "array_last") {
            if (node->children.size() != 1) {
                runtimeError(node.get(), "array_last() richiede 1 argomento");
                return 0;
            }
            Value arg = eval(node->children[0]);
            if (!isType<ArrayValue>(arg)) {
                runtimeError(node.get(), "array_last() supporta solo array");
                return 0;
            }
            auto& arr = as<ArrayValue>(arg);
            if (arr.empty()) {
                runtimeError(node.get(), "array_last(): array vuoto");
                return 0;
            }
            return *arr.elements.back();
        }
        
        // --- toInt() ---
        if (fname == "toInt") {
            if (node->children.size() != 1) {
                runtimeError(node.get(), "toInt() richiede 1 argomento");
                return 0;
            }
            Value arg = eval(node->children[0]);
            if (isType<int>(arg)) return arg;
            if (isType<double>(arg)) return static_cast<int>(as<double>(arg));
            if (isType<std::string>(arg)) {
                try {
                    return std::stoi(as<std::string>(arg));
                } catch (...) {
                    runtimeError(node.get(), "toInt(): conversione fallita");
                    return 0;
                }
            }
            runtimeError(node.get(), "toInt() non supporta questo tipo");
            return 0;
        }
        
        // --- toDouble() ---
        if (fname == "toDouble") {
            if (node->children.size() != 1) {
                runtimeError(node.get(), "toDouble() richiede 1 argomento");
                return 0.0;
            }
            Value arg = eval(node->children[0]);
            if (isType<double>(arg)) return arg;
            if (isType<int>(arg)) return static_cast<double>(as<int>(arg));
            if (isType<std::string>(arg)) {
                try {
                    return std::stod(as<std::string>(arg));
                } catch (...) {
                    runtimeError(node.get(), "toDouble(): conversione fallita");
                    return 0.0;
                }
            }
            runtimeError(node.get(), "toDouble() non supporta questo tipo");
            return 0.0;
        }
        
        // --- typeOf() ---
        if (fname == "typeOf") {
            if (node->children.size() != 1) {
                runtimeError(node.get(), "typeOf() richiede 1 argomento");
                return "";
            }
            Value arg = eval(node->children[0]);
            return typeOfValue(arg);
        }
        
        // --- input() ---
        if (fname == "input") {
            std::string line;
            std::getline(std::cin, line);
            return line;
        }
        
        // --- range() ---
        if (fname == "range") {
            int start = 0, end = 0, step = 1;
            
            if (node->children.size() == 1) {
                // range(end)
                Value endVal = eval(node->children[0]);
                if (!isType<int>(endVal)) {
                    runtimeError(node.get(), "range(): argomento deve essere int");
                    return ArrayValue{};
                }
                end = as<int>(endVal);
            } else if (node->children.size() == 2) {
                // range(start, end)
                Value startVal = eval(node->children[0]);
                Value endVal = eval(node->children[1]);
                if (!isType<int>(startVal) || !isType<int>(endVal)) {
                    runtimeError(node.get(), "range(): argomenti devono essere int");
                    return ArrayValue{};
                }
                start = as<int>(startVal);
                end = as<int>(endVal);
            } else if (node->children.size() == 3) {
                // range(start, end, step)
                Value startVal = eval(node->children[0]);
                Value endVal = eval(node->children[1]);
                Value stepVal = eval(node->children[2]);
                if (!isType<int>(startVal) || !isType<int>(endVal) || !isType<int>(stepVal)) {
                    runtimeError(node.get(), "range(): argomenti devono essere int");
                    return ArrayValue{};
                }
                start = as<int>(startVal);
                end = as<int>(endVal);
                step = as<int>(stepVal);
                
                if (step == 0) {
                    runtimeError(node.get(), "range(): step non può essere 0");
                    return ArrayValue{};
                }
            } else {
                runtimeError(node.get(), "range(): richiede 1, 2 o 3 argomenti");
                return ArrayValue{};
            }
            
            ArrayValue result;
            
            if (step > 0) {
                for (int i = start; i < end; i += step) {
                    result.push_back(std::make_shared<Value>(i));
                }
            } else {
                for (int i = start; i > end; i += step) {
                    result.push_back(std::make_shared<Value>(i));
                }
            }
            
            return result;
        }
        
        // ============================================
        // USER FUNCTIONS (local + global)
        // ============================================
        
        // Prima cerca in funzioni locali (nested)
        auto localFunc = currentScope().lookupLocalFunction(fname);
        if (localFunc) {
            ArrayValue args;
            for (auto& ch : node->children)
                args.push_back(std::make_shared<Value>(eval(ch)));
            
            return callUserFunction(localFunc, args, node.get());
        }
        
        // Poi cerca in funzioni globali
        auto it = functions.find(fname);
        if (it == functions.end()) {
            runtimeError(node.get(), "Funzione '" + fname + "' non definita");
            return 0;
        }

        ArrayValue args;
        for (auto& ch : node->children)
            args.push_back(std::make_shared<Value>(eval(ch)));

        return callUserFunction(it->second, args, node.get());
    }

    // ============================================
    // CALLEXPR: Chiamata su espressione
    // Es: (doubler $ addFive)(10)
    // ============================================
    if (t == "CallExpr") {
        // Primo child è l'espressione da chiamare
        auto funcExpr = node->children[0];
        Value funcVal = eval(funcExpr);
        
        if (!isType<FunctionValue>(funcVal)) {
            runtimeError(node.get(), 
                "CallExpr: l'espressione non valuta a una funzione");
            return 0;
        }
        
        auto& fv = as<FunctionValue>(funcVal);
        
        // Valuta argomenti (dal secondo child in poi)
        ArrayValue args;
        for (size_t i = 1; i < node->children.size(); ++i) {
            args.push_back(std::make_shared<Value>(eval(node->children[i])));
        }
        
        // Controlla numero argomenti
        if (args.size() != fv.params.size()) {
            runtimeError(node.get(), 
                "CallExpr: numero argomenti errato (attesi " + 
                std::to_string(fv.params.size()) + ", trovati " + 
                std::to_string(args.size()) + ")");
            return 0;
        }
        
        // ============================================
        // FUNZIONE COMPOSTA: f $ g
        // ============================================
        if (!fv.composedFuncs.empty()) {
            // Esegui composizione: (f $ g)(x) = g(f(x))
            Value result = *args[0];  // Valore iniziale
            
            // Applica ogni funzione in sequenza
            for (auto& funcPtr : fv.composedFuncs) {
                const FunctionValue& func = *funcPtr;
                
                // Crea scope temporaneo
                pushScope();
                
                // Ripristina variabili catturate
                for (const auto& pair : func.capturedVars) {
                    defineVar(pair.first, pair.second, false, false);
                }
                
                // Bind parametro
                defineVar(func.params[0], result, false, false);
                
                // Esegui funzione
                result = eval(func.body);
                
                popScope();
            }
            
            return result;
        }
        
        // ============================================
        // FUNZIONE NORMALE
        // ============================================
        pushScope();
        
        // Ripristina variabili catturate
        for (const auto& pair : fv.capturedVars) {
            defineVar(pair.first, pair.second, false, false);
        }
        
        // Bind parametri
        for (size_t i = 0; i < fv.params.size(); ++i) {
            defineVar(fv.params[i], *args[i], false, false);
        }
        
        // Esegui body
        Value ret = eval(fv.body);
        
        popScope();
        
        return ret;
    }



    // -------- Program --------
    if (t == "Program") {
        Value last = 0;
        for (auto& st : node->children)
            last = eval(st);
        return last;
    }


    // -------- Body --------
    if (t == "Body") {
        Value last = 0;

        for (auto& st : node->children) {
            if (!st) continue;

            // --- Expression statement ---
            if (st->type == "ExprStmt") {
                last = eval(st->children[0]);
                continue;
            }

            // --- Echo ---
            if (st->type == "Echo") {
                Value v = eval(st->children[0]);
                printValue(v);
                std::cout << "\n";
                last = v;
                continue;
            }

            // --- Assign ---
            if (st->type == "Assign") {
                last = evalAssignment(st);
                continue;
            }

            // --- VarDecl ---
            if (st->type == "VarDecl") {
                std::string name = st->value;
                bool isDynamic = (st->extra.count("dynamic") &&
                                  st->extra.at("dynamic") == "true");
                bool isFixed = (st->extra.count("fixed") &&
                                st->extra.at("fixed") == "true");
                Value val = 0;
                if (!st->children.empty())
                    val = eval(st->children[0]);
                defineVar(name, val, isDynamic, isFixed);
                continue;
            }

            // ============================================
            // --- NESTED FUNCTION DEFINITION ---
            // ============================================
            if (st->type == "FunctionDef") {
                std::string funcName = st->value;
                
                // Define function in CURRENT scope (local, not global!)
                currentScope().defineLocalFunction(funcName, st);
                
                last = 0;
                continue;
            }

            // --- ArrayDecl ---
            if (st->type == "ArrayDecl") {
                std::string name = st->value;
                bool isDynamic = (st->extra.count("dynamic") &&
                                  st->extra.at("dynamic") == "true");
                bool isFixed = (st->extra.count("fixed") &&
                                st->extra.at("fixed") == "true");
                ArrayValue arr;

                if (st->extra.count("size")) {
                    int size = std::stoi(st->extra.at("size"));
                    arr = makeArrayOfSize(size);
                }

                if (!st->children.empty() &&
                    st->children[0] &&
                    st->children[0]->type == "ArrayInit") {
                    auto init = st->children[0];
                    arr.elements.clear();
                    for (auto& ch : init->children)
                        appendArrayInitExpr(this, arr, ch);
                }

                defineVar(name, arr, isDynamic, isFixed);
                continue;
            }

            // --- ArrayAssign ---
            if (st->type == "ArrayAssign") {
                auto acc = st->children[0];
                auto rhs = st->children[1];
                std::string name = acc->value;

                StoredVar* sv = currentScope().lookup(name);
                if (!sv) {
                    runtimeError(st.get(), "Array '" + name + "' non definito");
                    continue;
                }
                if (!sv->isDynamic) {
                    runtimeError(st.get(), "Array '" + name + "' è immutabile");
                    continue;
                }
                if (!isType<ArrayValue>(sv->value)) {
                    runtimeError(st.get(), "'" + name + "' non è un array");
                    continue;
                }

                Value idxV = eval(acc->children[0]);
                if (!isType<int>(idxV)) {
                    runtimeError(st.get(), "Indice array deve essere int");
                    continue;
                }
                int idx = as<int>(idxV);
                auto& arr = as<ArrayValue>(sv->value);
                int normIdx = normalizeIndex(idx, arr.size());
                if (normIdx < 0) {
                    runtimeError(st.get(), "Indice array fuori limite");
                    continue;
                }

                Value v = eval(rhs);
                if (!arr[(size_t)normIdx])
                    arr[(size_t)normIdx] = std::make_shared<Value>(v);
                else
                    *arr[(size_t)normIdx] = v;

                continue;
            }

            // --- FunctionDef ---
            if (st->type == "FunctionDef") {
                functions[st->value] = st;
                continue;
            }

            // --- While ---
            if (st->type == "While") {
                last = eval(st);
                continue;
            }

            // --- ForIn ---
            if (st->type == "ForIn") {
                last = eval(st);
                continue;
            }

            // --- Unknown ---
            runtimeError(st.get(), "Tipo statement non gestito in Body: " + st->type);
        }

        return last;
    }




    runtimeError(node.get(), "Nodo non gestito in eval(): " + node->type);
    return 0;
}


// =======================
// Operatori
// =======================

// =======================
// SEZIONE DA SOSTITUIRE in evalBinaryOp
// Cerca "1) Operatori aritmetici" e sostituisci TUTTO il blocco fino a "2) Confronti numerici"
// =======================

Value Interpreter::evalBinaryOp(const std::string& op,
                                Value left,
                                Value right,
                                const ASTNode* node)
{
    // ============================================================
    // 1) Operatori aritmetici + - * / % **
    // ============================================================
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "**") {

        bool li = isType<int>(left);
        bool ld = isType<double>(left);
        bool ri = isType<int>(right);
        bool rd = isType<double>(right);

        // ========== INT + INT ==========
        if (li && ri) {
            int L = as<int>(left);
            int R = as<int>(right);

            if (op == "+") return L + R;
            if (op == "-") return L - R;
            if (op == "*") return L * R;
            if (op == "/") {
                if (R == 0) {
                    runtimeError(node, "Divisione per zero");
                    return 0;
                }
                return L / R;
            }
            if (op == "%") {
                if (R == 0) {
                    runtimeError(node, "Modulo per zero");
                    return 0;
                }
                return L % R;
            }
            if (op == "**") {
                // Potenza: int ** int → double
                return std::pow(static_cast<double>(L), static_cast<double>(R));
            }
        }

        // ========== DOUBLE + DOUBLE ==========
        if (ld && rd) {
            double L = as<double>(left);
            double R = as<double>(right);

            if (op == "+") return L + R;
            if (op == "-") return L - R;
            if (op == "*") return L * R;
            if (op == "/") {
                if (R == 0.0) {
                    runtimeError(node, "Divisione per zero");
                    return 0.0;
                }
                return L / R;
            }
            if (op == "%") {
                runtimeError(node, "Modulo (%) non supportato per double, solo int");
                return 0.0;
            }
            if (op == "**") {
                // Potenza: double ** double → double
                return std::pow(L, R);
            }
        }

        // ========== INT + DOUBLE (promozione a double) ==========
        if ((li && rd) || (ld && ri)) {
            double L = li ? static_cast<double>(as<int>(left)) : as<double>(left);
            double R = ri ? static_cast<double>(as<int>(right)) : as<double>(right);

            if (op == "+") return L + R;
            if (op == "-") return L - R;
            if (op == "*") return L * R;
            if (op == "/") {
                if (R == 0.0) {
                    runtimeError(node, "Divisione per zero");
                    return 0.0;
                }
                return L / R;
            }
            if (op == "%") {
                runtimeError(node, "Modulo (%) richiede entrambi int");
                return 0.0;
            }
            if (op == "**") {
                return std::pow(L, R);
            }
        }

        runtimeError(node,
            "Operatore '" + op + "' non definito per i tipi forniti (" +
            typeOfValue(left) + " e " + typeOfValue(right) + ")");
        return 0;
    }
    // ============================================================
    // 2) Confronti numerici < <= > >=
    // ============================================================
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {

        double L, R;
        bool okL = false, okR = false;

        if (isType<int>(left))   { L = as<int>(left);   okL = true; }
        if (isType<double>(left)){ L = as<double>(left); okL = true; }

        if (isType<int>(right))   { R = as<int>(right);   okR = true; }
        if (isType<double>(right)){ R = as<double>(right); okR = true; }

        if (!okL) {
            runtimeError(node, "Confronto non definito per tipo sinistro " + typeOfValue(left));
            return 0;
        }
        if (!okR) {
            runtimeError(node, "Confronto non definito per tipo destro " + typeOfValue(right));
            return 0;
        }

        if (op == "<")  return (L <  R) ? 1 : 0;
        if (op == "<=") return (L <= R) ? 1 : 0;
        if (op == ">")  return (L >  R) ? 1 : 0;
        if (op == ">=") return (L >= R) ? 1 : 0;
    }

    // ============================================================
    // 3) Uguaglianza
    // ============================================================
    if (op == "==") return (toString(left) == toString(right)) ? 1 : 0;
    if (op == "!=") return (toString(left) != toString(right)) ? 1 : 0;

    // ============================================================
    // 4) Logici
    // ============================================================
    if (op == "and") return (isTruthy(left) && isTruthy(right)) ? 1 : 0;
    if (op == "or")  return (isTruthy(left) || isTruthy(right)) ? 1 : 0;

    // ============================================================
    // 5) OPERATORE $ — CONCATENAZIONE UNIVERSALE
    // ============================================================
    if (op == "$") {

        // -- STRING + STRING -------------------------------------
        if (isType<std::string>(left) && isType<std::string>(right)) {
            return as<std::string>(left) + as<std::string>(right);
        }

        // NOTA: Range è gestito in eval() prima di chiamare evalBinaryOp
        // quindi qui non dovremmo mai ricevere RangeInfo

        // -- ARRAY + ARRAY → concat immutabile
        if (isType<ArrayValue>(left) && isType<ArrayValue>(right)) {
            ArrayValue out;
            const auto& A = as<ArrayValue>(left);
            const auto& B = as<ArrayValue>(right);

            out.elements.reserve(A.size() + B.size());

            for (auto& e : A.elements)
                out.elements.push_back(std::make_shared<Value>(*e));

            for (auto& e : B.elements)
                out.elements.push_back(std::make_shared<Value>(*e));

            return out;
        }

        // NOTA: ARRAY + RANGE è gestito in eval() prima di chiamare evalBinaryOp

        // -- FUNCTION + FUNCTION → COMPOSIZIONE
        if (isType<FunctionValue>(left) && isType<FunctionValue>(right)) {
            // f $ g crea una nuova funzione composta: (f $ g)(x) = g(f(x))

            const auto& f = as<FunctionValue>(left);
            const auto& g = as<FunctionValue>(right);

            // Controllo numero parametri (per ora solo funzioni unarie)
            if (f.params.size() != 1) {
                runtimeError(node,
                    "Composizione richiede funzione con 1 parametro (prima funzione ha " +
                    std::to_string(f.params.size()) + " parametri)");
                return 0;
            }

            if (g.params.size() != 1) {
                runtimeError(node,
                    "Composizione richiede funzione con 1 parametro (seconda funzione ha " +
                    std::to_string(g.params.size()) + " parametri)");
                return 0;
            }

            // Crea nuova funzione composta
            FunctionValue composed;
            composed.params = f.params;  // Stessi parametri della prima funzione

            // Crea AST per: g(f(param))
            // Questo è complicato, per ora salviamo le due funzioni e gestiamo a runtime
            composed.extra["composed"] = "true";
            composed.extra["first"] = "f";
            composed.extra["second"] = "g";

            // Salviamo le funzioni originali nel closure
            // (Questo richiede una struttura dati più complessa)
            // Per ora, creiamo un wrapper

            // SOLUZIONE SEMPLICE: Salviamo f e g come variabili temporanee
            // e creiamo un body che le chiama

            // Alternativa migliore: salviamo i FunctionValue nel composed stesso
            composed.composedFuncs.push_back(std::make_shared<FunctionValue>(f));
            composed.composedFuncs.push_back(std::make_shared<FunctionValue>(g));

            return composed;
        }

        // -- INT + INT → ERRORE (conversione esplicita richiesta)
        if (isType<int>(left) && isType<int>(right)) {
            runtimeError(node,
                "Concatenazione '$' non supporta int direttamente.\n"
                "Usa conversione esplicita: str(123) $ str(456)");
            return 0;
        }

        // -- DOUBLE + DOUBLE → ERRORE (conversione esplicita richiesta)
        if (isType<double>(left) && isType<double>(right)) {
            runtimeError(node,
                "Concatenazione '$' non supporta double direttamente.\n"
                "Usa conversione esplicita: str(3.14) $ str(2.71)");
            return 0;
        }

        runtimeError(node,
            "Concatenazione '$' richiede tipi uguali e concatenabili (string, array, funzioni).\n"
            "Trovato: " + typeOfValue(left) + " e " + typeOfValue(right));
        return 0;
    }

    // ============================================================
    // 6) Operatore sconosciuto
    // ============================================================
    runtimeError(node, "Operatore binario non gestito: " + op);
    return 0;
}


Value Interpreter::evalUnaryOp(const std::string& op,
                               const Value& val,
                               const ASTNode* node)
{
    // -------------------------
    // Operatore unario "-"
    // -------------------------
    if (op == "-") {

        if (isType<int>(val))
            return -as<int>(const_cast<Value&>(val));

        if (isType<double>(val))
            return -as<double>(const_cast<Value&>(val));

        runtimeError(node,
            "Operatore unario '-' non definito per tipo " +
            typeOfValue(val));

        return 0;
    }

    // -------------------------
    // Operatore unario "!"
    // -------------------------
    if (op == "!") {
        // Mammuth: "!" applicato a qualunque valore → truthiness standard
        return isTruthy(val) ? 0 : 1;
    }

    // -------------------------
    // Operatore sconosciuto
    // -------------------------
    runtimeError(node,
        "Operatore unario non gestito: '" + op + "'");

    return 0;
}




// =======================
// CondChain / Elvis / Filter
// =======================

Value Interpreter::evalCondChain(const std::shared_ptr<ASTNode>& node) {

    // 🔥 Se la CondChain è incompleta e stiamo cercando di ottenere un valore → ERRORE
    if (node->condIncomplete) {
        runtimeError(node.get(), "CondChain senza fallback usata in un contesto che richiede un valore");
        return 0; // non raggiunto
    }

    size_t n = node->children.size();
    bool hasFallback = (node->extra.count("hasFallback") > 0 &&
                        node->extra.at("hasFallback") == "1");

    // limite: se c’è fallback l’ultimo è il fallback; se non c’è sono tutte SimpleCond
    size_t limit = hasFallback ? n - 1 : n;

    // 🌟 Valutazione delle condizioni una per una
    for (size_t i = 0; i < limit; ++i) {

        auto condNode = node->children[i];

        if (!condNode || condNode->type != "SimpleCond")
            continue;

        // SimpleCond ha 2 figli:
        // [0] = condizione
        // [1] = espressione di risultato
        auto condVal = eval(condNode->children[0]);

        if (isTruthy(condVal)) {
            return eval(condNode->children[1]);  // 🎯 primo true -> risultato
        }
    }

    // 🎯 Se c'è fallback → valutalo e restituisci
    if (hasFallback && n > 0) {
        auto fb = node->children[n - 1];
        return eval(fb);
    }

    // 🎯 Se NON c'è fallback, ma siamo qui → significa che:
    //   - la CondChain è stata considerata valida come statement isolato
    //   - e non deve produrre un valore
    //
    // Per specifica del linguaggio, una CondChain senza fallback
    // isolata restituisce sempre 0.
    return 0;
}


Value Interpreter::evalElvis(const std::shared_ptr<ASTNode>& node) {
    Value left = eval(node->children[0]);
    if (isTruthy(left)) return left;
    return eval(node->children[1]);
}

Value Interpreter::evalFilter(const std::shared_ptr<ASTNode>& node) {
    // Filter has 2 children:
    // [0] = left expression (array to filter)
    // [1] = right expression (condition with implicit 'x')

    if (node->children.size() < 2) {
        runtimeError(node.get(), "Filter (=>) richiede due espressioni: array => condizione");
        return 0;
    }

    // Evaluate left side (should be an array)
    Value leftVal = eval(node->children[0]);

    if (!isType<ArrayValue>(leftVal)) {
        runtimeError(node.get(), "Filter (=>) si applica solo ad array, ricevuto: " +
                    typeOfValue(leftVal));  // ← FIX 1: typeOfValue
        return 0;
    }

    auto& inputArray = as<ArrayValue>(leftVal);

    // Create result array
    ArrayValue resultArray;
    // FIX 2: Rimossa riga elementType - non esiste in ArrayValue!

    // Get the condition expression
    auto condExpr = node->children[1];

    // For each element in input array
    for (const auto& element : inputArray.elements) {
        // Push a temporary scope
        pushScope();

        // Define implicit variable 'x' with current element
        StoredVar sv;
        sv.value = *element;  // ← FIX 3: Dereferenzia!
        sv.isDynamic = false;
        sv.isFixed = true;  // x is immutable in filter context
        currentScope().define("x", sv);

        DEBUG_SCOPE_LOG("Filter: defineVar 'x' (implicit) with element value");

        // Evaluate the condition with 'x' bound to current element
        Value condResult = eval(condExpr);

        // Pop the temporary scope
        popScope();

        // If condition is truthy, include element in result
        if (isTruthy(condResult)) {
            resultArray.elements.push_back(element);
        }
    }

    return resultArray;
}

// =======================
// Funzioni utente
// =======================

Value Interpreter::callUserFunction(const std::shared_ptr<ASTNode>& funcNode,
                                    const ArrayValue& args,
                                    const ASTNode* callSite)
{
    size_t paramCount = 0;
    while (paramCount < funcNode->children.size() &&
           funcNode->children[paramCount]->type == "Param") {
        ++paramCount;
    }

    if (args.size() != paramCount) {
        runtimeError(callSite, "Numero argomenti errato per funzione '" +
                               funcNode->value + "'");
        return 0;
    }

    if (paramCount >= funcNode->children.size() ||
        funcNode->children[paramCount]->type != "Body") {
        runtimeError(funcNode.get(), "FunctionDef senza Body");
        return 0;
    }

    auto body = funcNode->children[paramCount];

    pushScope();

    for (size_t i = 0; i < paramCount; ++i) {
        auto p = funcNode->children[i];
        std::string pname = p->value;

        if (!args[i]) {
            defineVar(pname, 0, true);
        } else {
            defineVar(pname, *args[i], true);
        }
    }

    Value ret = eval(body);

    popScope();

    // Se funzione ritorna zero, ignora ret e ritorna sempre 0
    std::string retType = funcNode->extra.count("returnType")
                          ? funcNode->extra.at("returnType")
                          : "";
    if (retType == "zero") {
        return 0;
    }

    return ret;
}

// ============================================================
// evalAssignment - Gestisce assegnamento variabili/array
// ============================================================
Value Interpreter::evalAssignment(const std::shared_ptr<ASTNode>& node) {
    if (!node || node->children.size() < 2) {
        runtimeError(node.get(), "Nodo Assign malformato");
        return 0;
    }

    auto target = node->children[0];
    auto valueExpr = node->children[1];

    // Caso 1: Assegnamento variabile semplice
    if (target->type == "Identifier") {
        std::string varName = target->value;
        Value newVal = eval(valueExpr);
        setVar(varName, newVal);
        return newVal;
    }

    // Caso 2: Assegnamento array element arr[idx] = val
    if (target->type == "ArrayAccess") {
        if (target->children.size() < 2) {
            runtimeError(target.get(), "ArrayAccess malformato");
            return 0;
        }

        std::string arrName = target->value;
        auto idxNode = target->children[0];

        // Valuta indice
        Value idxVal = eval(idxNode);
        if (!isType<int>(idxVal)) {
            runtimeError(idxNode.get(), "Indice array deve essere int");
            return 0;
        }
        int idx = as<int>(idxVal);

        // Lookup array
        auto sv = currentScope().lookup(arrName);
        if (!sv) {
            runtimeError(target.get(), "Array '" + arrName + "' non definito");
            return 0;
        }

        if (!isType<ArrayValue>(sv->value)) {
            runtimeError(target.get(), "'" + arrName + "' non è un array");
            return 0;
        }

        // Controlla se è dynamic
        if (!sv->isDynamic) {
            runtimeError(target.get(), "Array '" + arrName + "' non è dynamic, non può essere modificato");
            return 0;
        }

        auto& arr = as<ArrayValue>(sv->value);

        // Normalizza indice
        int normIdx = idx;
        if (idx < 0) {
            normIdx = static_cast<int>(arr.size()) + idx;
        }

        if (normIdx < 0 || normIdx >= static_cast<int>(arr.size())) {
            runtimeError(target.get(), "Indice " + std::to_string(idx) + " fuori range");
            return 0;
        }

        // Valuta e assegna
        Value newVal = eval(valueExpr);
        *arr[normIdx] = newVal;
        return newVal;
    }

    runtimeError(node.get(), "Target di assegnamento non riconosciuto");
    return 0;
}

