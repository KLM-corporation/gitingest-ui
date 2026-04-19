#pragma once
#include <string>
#include <functional>

namespace SystemUtils {
    // Affiche un selecteur de dossier Windows et retourne le chemin
    std::string SelectDirectory();
    
    // Affiche une boîte de dialogue "Enregistrer sous" pour créer un fichier texte
    std::string SaveFileDialog();
    
    // Compte le vrai nombre de tokens via tiktoken (Python)
    // Retourne -1 si tiktoken/Python n'est pas disponible
    int CountTokensTiktoken(const std::string& text);
    
    
    // Execute une commande dans un processus séparé et lit la sortie
    // La fonction de callback est appelée en temps réel avec les logs
    bool ExecuteCommand(const std::string& command, std::function<void(const std::string& out)> callback);
    
    // Windows Data Protection API (DPAPI)
    // Sécurise une chaîne pour qu'elle ne soit lisible que par l'utilisateur propriétaire
    std::string EncryptString(const std::string& input);
    std::string DecryptString(const std::string& input);
    
    // Ouvre une URL dans le navigateur par défaut
    void OpenURL(const std::string& url);

    // Masque les tokens sensibles dans une chaîne
    std::string MaskSensitive(const std::string& input, const std::string& token);
    
    // Encodage/Décodage Base64 (pour stocker les blobs DPAPI en texte dans l'INI)
    std::string Base64Encode(const std::string& input);
    std::string Base64Decode(const std::string& input);
    
    // Vérifie qu'une URL git est sûre (pas d'injection de commande)
    bool IsValidGitUrl(const std::string& url);
}
