#pragma once
#include <string>
#include <atomic>
#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <set>
#include "NativeIngester.h"

class AppUI {
public:
    void Init();
    void Render();
    void Shutdown();
    
private:
    void CleanupTempDir();
    char m_InputPath[512] = "";
    char m_Excludes[256] = "";
    int m_MaxSize = 10; // MegaOctets
    bool m_IncludePatterns = false;
    char m_Includes[256] = "";
    
    std::atomic<bool> m_IsRunning{false};
    std::atomic<bool> m_IsCancelled{false};
    std::string m_ConsoleOutput;
    std::string m_LastDigest;
    std::mutex m_DigestMutex;
    std::thread m_WorkerThread;
    std::string m_ConfigPath;
    unsigned int m_LogoTexture = 0;

    // Live Filters Cache
    std::vector<std::string> m_ExclVec;
    std::vector<std::string> m_InclVec;
    std::vector<std::string> m_GitignoreVec;
    void RefreshFilters();

    // Project Tree Selection
    FileNode m_ProjectTree;
    bool m_TreeValid = false;
    void RenderVisualTree(FileNode& node, std::vector<bool>& last_entries, const std::vector<std::string>& excl, const std::vector<std::string>& incl);
    void ToggleNodeRecursive(FileNode& node, bool selected);
    void UpdateParentStates(FileNode& node);
    void CollectExcludedPaths(const FileNode& node, std::set<std::string>& excluded);
    
    bool m_IgnoreGitignore = false;
    bool m_AddLineNumbers = false;
    bool m_IgnoreMaxSizeLimit = false;
    
    // Dépôt privé
    bool m_IsPrivateRepo = false;
    char m_GitHubToken[256] = "";
    
    // Persistance
    bool LoadSettings();
    void SaveSettings();
    
    // Wizard d'Onboarding (Optionnel)
    bool m_ShowWizard = false;
    int m_WizardStep = 0;
    bool m_MigrationNotice = false;
    bool m_LangFrench = false; // false = English (default), true = French
    void RenderWizard();
    
    // Fonction privée pour executer la commande dans un thread 
    void RunGitingest();
};
