#include "I18n.h"
#include <unordered_map>
#include <string>

// Anglais par défaut
static bool s_IsFrench = false;

// Dictionnaire des traductions (Français -> Anglais)
static std::unordered_map<std::string, std::string> s_Translations = {
    {"Paramètres mis à jour pour la nouvelle version (Format sécurisé).", "Settings updated for the new version (Secure format)."},
    {"OK, j'ai compris", "OK, got it"},
    {"Ingesteur de code local", "Local Code Ingester"},
    {"C:\\mon\\projet", "C:\\my\\project"},
    {"Parcourir", "Browse"},
    {"Scanner", "Scan"},
    {"Dépôt Privé (GitHub)", "Private Repository (GitHub)"},
    {"FILTRES", "FILTERS"},
    {"ILLIMITE", "UNLIMITED"},
    {"Sans limite", "Unlimited"},
    {"Numerotation des lignes", "Line numbering"},
    {"Ignorer .gitignore", "Ignore .gitignore"},
    {"ANNULER L'ANALYSE", "CANCEL ANALYSIS"},
    {"Analyse en cours...", "Analysis in progress..."},
    {"Lancer l'analyse", "Start analysis"},
    {"Copier", "Copy"},
    {"Enregistrer", "Save"},
    {"Effacer", "Clear"},
    {"Choisissez un projet et cliquez sur 'Scanner' pour commencer...", "Choose a project and click 'Scan' to start..."},
    
    // Wizard Setup
    {"Bienvenue sur GitingestUI", "Welcome to GitingestUI"},
    {"Bienvenue !", "Welcome!"},
    {"Transformez vos dépôts Git en texte pour LLM.", "Turn your Git repositories into text for LLMs."},
    {"Dossier de travail habituel", "Usual working folder"},
    {"GitHub Token (Optionnel)", "GitHub Token (Optional)"},
    {"Repos privés", "Private repos"},
    {"Prêt !", "Ready!"},
    {"Précédent", "Previous"},
    {"Suivant", "Next"},
    {"C'est parti !", "Let's go!"},
    
    // Logs and execution output
    {"\n➔ Estimation : ~", "\n➔ Estimate: ~"},
    {" tokens Tiktoken\n\n", " Tiktoken tokens\n\n"},
    {"➔ Commande : git clone [PRIVATE REPO] ", "➔ Command: git clone [PRIVATE REPO] "},
    {"➔ Commande : git clone ", "➔ Command: git clone "},

    // Options section
    {"OPTIONS", "OPTIONS"}
};

void InitI18n() {
    // Anglais par défaut, la langue sera chargée depuis les settings
    s_IsFrench = false;
}

void SetLanguage(bool french) {
    s_IsFrench = french;
}

bool IsFrench() {
    return s_IsFrench;
}

const char* tr(const char* key) {
    if (s_IsFrench) {
        return key;
    }
    
    auto it = s_Translations.find(key);
    if (it != s_Translations.end()) {
        return it->second.c_str();
    }
    
    return key; // Fallback to key if missing
}
