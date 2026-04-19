#include "AppUI.h"
#include "SystemUtils.h"
#include "NativeIngester.h"
#include <windows.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <filesystem>
#include <shlobj.h>
#include "I18n.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace fs = std::filesystem;

static std::string s_LogBuffer;
static std::mutex s_LogMutex;

static void StandardOutputCallback(const std::string& out) {
    std::lock_guard<std::mutex> lock(s_LogMutex);
    s_LogBuffer += out;
}

void AppUI::Init() {
    InitI18n();
    
    char pathBuf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, pathBuf))) {
        std::string dir = std::string(pathBuf) + "\\GitingestUI";
        CreateDirectoryA(dir.c_str(), NULL);
        m_ConfigPath = dir + "\\gitingest_config.ini";
    } else {
        m_ConfigPath = "gitingest_config.ini";
    }
    
    // Language will be set after LoadSettings() applies m_LangFrench
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.ItemSpacing = ImVec2(12, 10);
    style.WindowPadding = ImVec2(20, 20);

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg]         = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    c[ImGuiCol_Border]           = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.14f, 0.14f, 0.18f, 1.0f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.18f, 0.18f, 0.24f, 1.0f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.20f, 0.20f, 0.28f, 1.0f);
    c[ImGuiCol_TitleBg]          = ImVec4(0.07f, 0.07f, 0.09f, 1.0f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.07f, 0.07f, 0.09f, 1.0f);
    c[ImGuiCol_ScrollbarBg]      = ImVec4(0.07f, 0.07f, 0.09f, 1.0f);
    c[ImGuiCol_ScrollbarGrab]    = ImVec4(0.30f, 0.30f, 0.38f, 1.0f);
    c[ImGuiCol_CheckMark]        = ImVec4(0.38f, 0.60f, 1.00f, 1.0f);
    c[ImGuiCol_SliderGrab]       = ImVec4(0.38f, 0.60f, 1.00f, 1.0f);
    c[ImGuiCol_Button]           = ImVec4(0.22f, 0.42f, 0.88f, 1.0f);
    c[ImGuiCol_Text]             = ImVec4(0.92f, 0.92f, 0.95f, 1.0f);
    c[ImGuiCol_TextDisabled]     = ImVec4(0.45f, 0.45f, 0.50f, 1.0f);

    if (!LoadSettings()) {
        m_ShowWizard = true;
        m_WizardStep = 0;
    }
    // Apply saved language preference
    SetLanguage(m_LangFrench);

    // Load Logo Texture
    int width, height, channels;
    unsigned char* data = stbi_load("gitingest_logo.png", &width, &height, &channels, 4);
    if (!data) {
        // Try absolute path from exe dir
        char exePath[512] = {};
        GetModuleFileNameA(NULL, exePath, 512);
        std::string p(exePath);
        p = p.substr(0, p.find_last_of("\\/")) + "\\gitingest_logo.png";
        data = stbi_load(p.c_str(), &width, &height, &channels, 4);
    }
    
    if (data) {
        glGenTextures(1, &m_LogoTexture);
        glBindTexture(GL_TEXTURE_2D, m_LogoTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
}

void AppUI::Shutdown() {
    m_IsCancelled = true;
    if (m_WorkerThread.joinable()) {
        m_WorkerThread.join();
    }
    CleanupTempDir();
}

void AppUI::CleanupTempDir() {
    try {
        if (fs::exists(".gitingest_temp")) {
            fs::remove_all(".gitingest_temp");
        }
    } catch (...) {}
}

bool AppUI::LoadSettings() {
    // Migration logic : if old config.dat exists, inform user and remove it
    if (fs::exists("config.dat")) {
        fs::remove("config.dat");
        m_MigrationNotice = true;
    }

    std::ifstream ifs(m_ConfigPath);
    if (ifs) {
        std::string line;
        while (std::getline(ifs, line)) {
            size_t sep = line.find('=');
            if (sep == std::string::npos) continue;
            std::string key = line.substr(0, sep);
            std::string val = line.substr(sep + 1);

            if (key == "InputPath") snprintf(m_InputPath, sizeof(m_InputPath), "%s", val.c_str());
            else if (key == "Excludes") snprintf(m_Excludes, sizeof(m_Excludes), "%s", val.c_str());
            else if (key == "Includes") snprintf(m_Includes, sizeof(m_Includes), "%s", val.c_str());
            else if (key == "MaxSize") m_MaxSize = std::stoi(val);
            else if (key == "AddLineNumbers") m_AddLineNumbers = (val == "1");
            else if (key == "IgnoreGitignore") m_IgnoreGitignore = (val == "1");
            else if (key == "IgnoreMaxSizeLimit") m_IgnoreMaxSizeLimit = (val == "1");
            else if (key == "LangFrench") m_LangFrench = (val == "1");
            else if (key == "IsPrivateRepo") m_IsPrivateRepo = (val == "1");
            else if (key == "GitHubToken") {
                // Base64 decode first, then DPAPI decrypt
                std::string rawBlob = SystemUtils::Base64Decode(val);
                std::string decrypted = SystemUtils::DecryptString(rawBlob);
                snprintf(m_GitHubToken, sizeof(m_GitHubToken), "%s", decrypted.c_str());
            }
        }
        return true;
    }
    return false;
}

void AppUI::SaveSettings() {
    std::ofstream ofs(m_ConfigPath);
    if (ofs) {
        ofs << "InputPath=" << m_InputPath << "\n";
        ofs << "Excludes=" << m_Excludes << "\n";
        ofs << "Includes=" << m_Includes << "\n";
        ofs << "MaxSize=" << m_MaxSize << "\n";
        ofs << "AddLineNumbers=" << (m_AddLineNumbers ? "1" : "0") << "\n";
        ofs << "IgnoreGitignore=" << (m_IgnoreGitignore ? "1" : "0") << "\n";
        ofs << "IgnoreMaxSizeLimit=" << (m_IgnoreMaxSizeLimit ? "1" : "0") << "\n";
        ofs << "LangFrench=" << (m_LangFrench ? "1" : "0") << "\n";
        ofs << "IsPrivateRepo=" << (m_IsPrivateRepo ? "1" : "0") << "\n";
        
        // DPAPI encrypt, then Base64 encode for safe text storage
        std::string encryptedToken = SystemUtils::EncryptString(m_GitHubToken);
        std::string b64Token = SystemUtils::Base64Encode(encryptedToken);
        ofs << "GitHubToken=" << b64Token << "\n";
    }
}

void AppUI::Render() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGuiWindowFlags wflags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("GitingestUI##main", nullptr, wflags);
    
    // Synchronize background logs to UI console
    {
        std::lock_guard<std::mutex> lock(s_LogMutex);
        if (!s_LogBuffer.empty()) {
            std::string filtered = SystemUtils::MaskSensitive(s_LogBuffer, m_GitHubToken);
            m_ConsoleOutput += filtered;
            s_LogBuffer.clear();
        }
    }
    
    if (m_MigrationNotice) {
        ImGui::SetNextItemWidth(viewport->WorkSize.x);
        ImGui::TextColored(ImVec4(1, 1, 0, 1), tr("Paramètres mis à jour pour la nouvelle version (Format sécurisé)."));
        if (ImGui::Button(tr("OK, j'ai compris"))) m_MigrationNotice = false;
        ImGui::Separator();
    }
    
    if (m_ShowWizard) RenderWizard();
    
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float totalHeight = ImGui::GetContentRegionAvail().y;
    float leftPanelW = totalWidth * 0.35f;
    float rightPanelW = totalWidth - leftPanelW - ImGui::GetStyle().ItemSpacing.x;
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.13f, 1.0f));
    ImGui::BeginChild("##left", ImVec2(leftPanelW, totalHeight), true);
    
    ImGui::Spacing();
    if (m_LogoTexture) {
        float logoSize = 100.0f;
        ImGui::SetCursorPosX((leftPanelW - logoSize) * 0.5f);
        ImGui::Image((void*)(intptr_t)m_LogoTexture, ImVec2(logoSize, logoSize));
    }
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.38f, 0.65f, 1.0f, 1.0f));
    float titleW = ImGui::CalcTextSize("GITINGEST").x;
    ImGui::SetCursorPosX((leftPanelW - titleW) * 0.5f);
    ImGui::Text("GITINGEST");
    ImGui::PopStyleColor();
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1.0f));
    const char* subTitle = tr("Ingesteur de code local");
    float subW = ImGui::CalcTextSize(subTitle).x;
    ImGui::SetCursorPosX((leftPanelW - subW) * 0.5f);
    ImGui::Text("%s", subTitle);
    ImGui::PopStyleColor();
    
    // Language toggle button (Flag)
    {
        const char* flagLabel = m_LangFrench ? "EN" : "FR";
        ImVec4 flagColor = m_LangFrench ? ImVec4(0.2f, 0.3f, 0.7f, 1.0f) : ImVec4(0.2f, 0.3f, 0.7f, 1.0f);
        float flagBtnW = ImGui::CalcTextSize("EN").x + 24.0f;
        ImGui::SetCursorPosX(leftPanelW - flagBtnW - ImGui::GetStyle().WindowPadding.x);
        ImGui::PushStyleColor(ImGuiCol_Button, flagColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.4f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.25f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        if (ImGui::Button(flagLabel, ImVec2(flagBtnW, ImGui::GetFrameHeight()))) {
            m_LangFrench = !m_LangFrench;
            SetLanguage(m_LangFrench);
            SaveSettings();
        }
        ImGui::PopStyleColor(4);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", m_LangFrench ? "Switch to English" : "Passer en Francais");
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.65f, 0.75f, 1.0f));
    ImGui::Text("SOURCE");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    
    float browseW = ImGui::CalcTextSize(tr("Parcourir")).x + 20.0f;
    float scanW = ImGui::CalcTextSize(tr("Scanner")).x + 20.0f;
    float buttonTotalW = browseW + scanW + ImGui::GetStyle().ItemSpacing.x;
    float inputW = leftPanelW - ImGui::GetStyle().WindowPadding.x * 2 - buttonTotalW - ImGui::GetStyle().ItemSpacing.x;
    
    ImGui::SetNextItemWidth(inputW);
    ImGui::InputText("##src", m_InputPath, sizeof(m_InputPath));
    if (!ImGui::IsItemActive() && m_InputPath[0] == '\0') {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 rmin = ImGui::GetItemRectMin();
        dl->AddText(ImVec2(rmin.x + 5, rmin.y + 5), IM_COL32(100, 100, 120, 200), tr("C:\\mon\\projet"));
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("Parcourir"), ImVec2(browseW, 0))) {
        std::string path = SystemUtils::SelectDirectory();
        if (!path.empty()) {
            snprintf(m_InputPath, sizeof(m_InputPath), "%s", path.c_str());
            m_TreeValid = false; // Reset tree on path change
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("Scanner"), ImVec2(scanW, 0))) {
        if (m_InputPath[0] != '\0') {
            m_ProjectTree = NativeIngester::ScanDirectory(m_InputPath, m_IgnoreGitignore);

            m_TreeValid = true;
            if (!m_IgnoreGitignore) m_GitignoreVec = NativeIngester::ParseGitignore(m_InputPath);
            else m_GitignoreVec.clear();
            RefreshFilters();
        }
    }

    ImGui::Spacing();
    ImGui::Checkbox(tr("Dépôt Privé (GitHub)"), &m_IsPrivateRepo);
    if (m_IsPrivateRepo) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(leftPanelW - ImGui::GetCursorPosX() - 20);
        ImGui::InputTextWithHint("##token", "GitHub PAT", m_GitHubToken, sizeof(m_GitHubToken), ImGuiInputTextFlags_Password);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.65f, 1.0f, 1.0f));
        ImGui::Text("   Get your token");
        if (ImGui::IsItemClicked()) SystemUtils::OpenURL("https://github.com/settings/tokens");
        ImGui::PopStyleColor();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.65f, 0.75f, 1.0f));
    ImGui::Text("%s", tr("FILTRES"));
    ImGui::PopStyleColor();
    float fullW = leftPanelW - ImGui::GetStyle().WindowPadding.x * 2;
    ImGui::SetNextItemWidth(fullW);
    if (ImGui::InputText("##excl", m_Excludes, sizeof(m_Excludes))) RefreshFilters();
    ImGui::SetNextItemWidth(fullW);
    if (ImGui::InputText("##incl", m_Includes, sizeof(m_Includes))) RefreshFilters();
    
    ImGui::Spacing();
    ImGui::Separator();
    
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.65f, 0.75f, 1.0f));
    ImGui::Text("%s", tr("OPTIONS"));
    ImGui::PopStyleColor();
    float sliderW = fullW - 130.0f;
    ImGui::PushItemWidth(sliderW);
    if (m_IgnoreMaxSizeLimit) {
        ImGui::BeginDisabled();
        int d = 0; ImGui::SliderInt("##ms", &d, 0, 0, tr("ILLIMITE"));
        ImGui::EndDisabled();
    } else {
        ImGui::SliderInt("##ms", &m_MaxSize, 1, 500, "%d MB");
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Checkbox(tr("Sans limite"), &m_IgnoreMaxSizeLimit);
    
    ImGui::Checkbox(tr("Numerotation des lignes"), &m_AddLineNumbers);
    ImGui::Checkbox(tr("Ignorer .gitignore"), &m_IgnoreGitignore);
    
    ImGui::Spacing();
    ImGui::Separator();
    
    ImGui::Spacing();
    if (m_IsRunning) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button(tr("ANNULER L'ANALYSE"), ImVec2(fullW, 48))) m_IsCancelled = true;
        ImGui::PopStyleColor();
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
        ImGui::SetCursorPosX((fullW - ImGui::CalcTextSize(tr("Analyse en cours...")).x) * 0.5f);
        ImGui::Text("%s", tr("Analyse en cours..."));
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.25f, 1.0f));
        if (ImGui::Button(tr("Lancer l'analyse"), ImVec2(fullW, 48))) RunGitingest();
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(); // End left panel child bg
    
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.13f, 1.0f));
    ImGui::BeginChild("##right", ImVec2(rightPanelW, totalHeight), true);
    
    float actionBtnW = ImGui::CalcTextSize(tr("Enregistrer")).x + 30.0f;
    float actionBtnH = ImGui::GetFontSize() * 2.0f;
    if (m_TreeValid || !m_ConsoleOutput.empty()) {
        if (!m_ConsoleOutput.empty()) {
             if (ImGui::Button(tr("Copier"), ImVec2(actionBtnW, actionBtnH))) {
                 std::lock_guard<std::mutex> lock(m_DigestMutex);
                 ImGui::SetClipboardText(m_LastDigest.c_str());
             }
             ImGui::SameLine();
             if (ImGui::Button(tr("Enregistrer"), ImVec2(actionBtnW, actionBtnH))) {
                 std::string p = SystemUtils::SaveFileDialog();
                 if (!p.empty()) { 
                     std::ofstream os(p, std::ios::binary); 
                     std::lock_guard<std::mutex> lock(m_DigestMutex);
                     os << m_LastDigest; 
                 }
             }
             ImGui::SameLine();
             if (ImGui::Button(tr("Effacer"), ImVec2(actionBtnW, actionBtnH))) { 
                 std::lock_guard<std::mutex> lock(m_DigestMutex);
                 m_ConsoleOutput.clear(); 
                 m_LastDigest.clear(); 
             }
        }
        
        if (m_TreeValid) {
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
            ImGui::Text("DIRECTORY STRUCTURE (Interactive - Click to Toggle)");
            ImGui::PopStyleColor();
            
            ImGui::BeginChild("##tree_frame", ImVec2(0, 300), true);
            std::vector<bool> last_entries;
            RenderVisualTree(m_ProjectTree, last_entries, m_ExclVec, m_InclVec);
            ImGui::EndChild();
        }
        
        if (!m_ConsoleOutput.empty()) {
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.07f, 1.0f));
            ImGui::BeginChild("##console", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(m_ConsoleOutput.c_str());
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    } else {
        ImGui::SetCursorPosY(totalHeight * 0.5f);
        ImGui::SetCursorPosX(20);
        ImGui::Text("%s", tr("Choisissez un projet et cliquez sur 'Scanner' pour commencer..."));
    }
    ImGui::EndChild(); // End right panel
    ImGui::PopStyleColor(); // End right panel color
    ImGui::End(); // End main window
}

void AppUI::RenderWizard() {
    ImGui::OpenPopup(tr("Bienvenue sur GitingestUI"));
    ImVec2 c = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(c, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 480));

    if (ImGui::BeginPopupModal(tr("Bienvenue sur GitingestUI"), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (m_WizardStep == 0) {
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "%s", tr("Bienvenue !"));
            ImGui::TextWrapped("%s", tr("Transformez vos dépôts Git en texte pour LLM."));
        } else if (m_WizardStep == 1) {
            ImGui::Text("%s", tr("Dossier de travail habituel"));
            ImGui::InputText("##w_p", m_InputPath, sizeof(m_InputPath));
        } else if (m_WizardStep == 2) {
            ImGui::Text("%s", tr("GitHub Token (Optionnel)"));
            ImGui::Checkbox(tr("Repos privés"), &m_IsPrivateRepo);
            if (m_IsPrivateRepo) ImGui::InputText("##w_t", m_GitHubToken, sizeof(m_GitHubToken), ImGuiInputTextFlags_Password);
        } else if (m_WizardStep == 3) {
            ImGui::Text("%s", tr("Prêt !"));
        }
        
        ImGui::SetCursorPosY(430);
        if (m_WizardStep > 0) { if (ImGui::Button(tr("Précédent"), ImVec2(100, 0))) m_WizardStep--; }
        ImGui::SameLine(480);
        if (m_WizardStep < 3) { if (ImGui::Button(tr("Suivant"), ImVec2(100, 0))) m_WizardStep++; }
        else { if (ImGui::Button(tr("C'est parti !"), ImVec2(100, 0))) { SaveSettings(); m_ShowWizard = false; ImGui::CloseCurrentPopup(); } }
        ImGui::EndPopup();
    }
}

void AppUI::RunGitingest() {
    CleanupTempDir();
    SaveSettings();
    m_IsRunning = true;
    m_IsCancelled = false;
    m_ConsoleOutput.clear();
    s_LogBuffer.clear();
    
    if (m_WorkerThread.joinable()) {
        m_WorkerThread.join();
    }
    
    std::string path = m_InputPath;
    std::string ex = m_Excludes;
    std::string inc = m_Includes;
    int ms = m_MaxSize;
    bool ig = m_IgnoreGitignore;
    bool an = m_AddLineNumbers;
    bool nl = m_IgnoreMaxSizeLimit;
    bool pr = m_IsPrivateRepo;
    std::string tk = m_GitHubToken;

    std::set<std::string> excludedPaths;
    if (m_TreeValid) {
        CollectExcludedPaths(m_ProjectTree, excludedPaths);
    }
    
    m_WorkerThread = std::thread([this, path, ex, inc, ms, ig, an, nl, pr, tk, excludedPaths]() {
        std::string targetDir = path;
        if (path.find("http") == 0) {
            // SECURITY: Validate URL to prevent command injection
            if (!SystemUtils::IsValidGitUrl(path)) {
                { std::lock_guard<std::mutex> l(s_LogMutex); s_LogBuffer += "Error: Invalid or potentially dangerous URL.\n"; }
                m_IsRunning = false;
                return;
            }
            
            std::string url = path;
            targetDir = ".gitingest_temp";
            
            if (pr && !tk.empty()) {
                // SECURITY: Use git credential-store with a temp file
                // instead of embedding token in URL (visible in process list)
                char exePathBuf[MAX_PATH] = {};
                GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
                std::string exeDir(exePathBuf);
                exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
                std::string credFile = exeDir + "\\.gitcredtmp";
                
                // Write credentials in git-credential-store format
                {
                    std::ofstream credOfs(credFile, std::ios::binary);
                    if (credOfs) {
                        // Extract hostname from URL for credential matching
                        std::string host = "github.com";
                        size_t hostStart = url.find("://");
                        if (hostStart != std::string::npos) {
                            hostStart += 3;
                            size_t hostEnd = url.find('/', hostStart);
                            if (hostEnd != std::string::npos)
                                host = url.substr(hostStart, hostEnd - hostStart);
                            else
                                host = url.substr(hostStart);
                        }
                        credOfs << "https://x-access-token:" << tk << "@" << host << std::endl;
                    }
                }
                
                // Convert backslashes to forward slashes for git
                std::string credFileGit = credFile;
                for (char& c : credFileGit) { if (c == '\\') c = '/'; }
                
                { std::lock_guard<std::mutex> l(s_LogMutex); 
                  s_LogBuffer += std::string(tr("➔ Commande : git clone [PRIVATE REPO] ")) + targetDir + "\n"; }
                
                std::string cloneCmd = "git -c credential.helper=\"store --file=" + credFileGit + "\" clone " + url + " " + targetDir;
                SystemUtils::ExecuteCommand(cloneCmd, StandardOutputCallback);
                
                // Secure cleanup: delete temp credential file immediately
                remove(credFile.c_str());
            } else {
                { std::lock_guard<std::mutex> l(s_LogMutex); 
                  s_LogBuffer += std::string(tr("➔ Commande : git clone ")) + url + " " + targetDir + "\n"; }
                SystemUtils::ExecuteCommand("git clone " + url + " " + targetDir, StandardOutputCallback);
            }
        }
        IngestOptions opt;
        opt.sourcePath = targetDir;
        opt.customExcludes = ex;
        opt.customIncludes = inc;
        opt.maxSizeMb = ms;
        opt.ignoreGitignore = ig;
        opt.addLineNumbers = an;
        opt.ignoreMaxSizeLimit = nl;
        opt.excludedPaths = excludedPaths;
        
        std::string localDigest = NativeIngester::IngestLocalDirectory(opt, StandardOutputCallback, [this]() { return m_IsCancelled.load(); });
        
        if (!m_IsCancelled) {
            int tok = SystemUtils::CountTokensTiktoken(localDigest);
            std::lock_guard<std::mutex> l(s_LogMutex); 
            s_LogBuffer += std::string(tr("\n➔ Estimation : ~")) + std::to_string(tok) + tr(" tokens Tiktoken\n\n");
        }
        
        { std::lock_guard<std::mutex> l(s_LogMutex); 
          s_LogBuffer += localDigest;
        }
        
        { std::lock_guard<std::mutex> l(m_DigestMutex);
          m_LastDigest = localDigest;
        }
        
        m_IsRunning = false;
    });
}

void AppUI::RenderVisualTree(FileNode& node, std::vector<bool>& last_entries, const std::vector<std::string>& excl, const std::vector<std::string>& incl) {
    ImGui::PushID(node.relativePath.c_str());

    // 1. DESSINER LES LIGNES DU TREE (│, ├──, └──)
    std::string linePrefix = "";
    if (!last_entries.empty()) {
        for (size_t i = 0; i < last_entries.size() - 1; ++i) {
            linePrefix += (last_entries[i] ? "    " : "│   ");
        }
        linePrefix += (last_entries.back() ? "└── " : "├── ");
    }
    
    ImGui::TextDisabled("%s", linePrefix.c_str());
    ImGui::SameLine(0, 0);

    // 2. DESSINER LE NOM DU NOEUD (INTERACTIF)
    bool isTreeExcluded = (node.state == SelectionState::Unselected);
    
    // On verifie si le filtre manuel (m_Excludes / m_Includes) match aussi
    // IsExcluded prend normalement un fs::path. On lui passe path relatif.
    bool isFilterExcluded = NativeIngester::IsExcluded(node.relativePath, excl, m_GitignoreVec);
    // TODO: In the future, check m_InclVec if not empty.
    
    bool isVisuallyExcluded = isTreeExcluded || isFilterExcluded;
    
    if (isVisuallyExcluded) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    else if (node.isDirectory) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
    
    std::string displayName = node.name + (node.isDirectory ? "/" : "");
    ImGui::Selectable(displayName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(ImGui::CalcTextSize(displayName.c_str()).x, 0));
    
    if (ImGui::IsItemClicked()) {
        ToggleNodeRecursive(node, isTreeExcluded); // On toggle l'état interne uniquement
        UpdateParentStates(m_ProjectTree);
    }

    // STRIKE-THROUGH LOGIC
    if (isVisuallyExcluded) {
        ImVec2 p_min = ImGui::GetItemRectMin();
        ImVec2 p_max = ImGui::GetItemRectMax();
        float mid_y = p_min.y + (p_max.y - p_min.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(p_min.x, mid_y), ImVec2(p_max.x, mid_y), IM_COL32(100, 100, 100, 255), 1.5f);
        ImGui::PopStyleColor();
    } else if (node.isDirectory) {
        ImGui::PopStyleColor();
    }

    // 3. RECURSION
    if (node.isDirectory && !node.children.empty()) {
        for (size_t i = 0; i < node.children.size(); ++i) {
            last_entries.push_back(i == node.children.size() - 1);
            RenderVisualTree(node.children[i], last_entries, excl, incl);
            last_entries.pop_back();
        }
    }

    ImGui::PopID();
}

void AppUI::ToggleNodeRecursive(FileNode& node, bool selected) {
    node.state = selected ? SelectionState::Selected : SelectionState::Unselected;
    for (auto& child : node.children) {
        ToggleNodeRecursive(child, selected);
    }
}

void AppUI::UpdateParentStates(FileNode& node) {
    if (!node.isDirectory || node.children.empty()) return;

    for (auto& child : node.children) {
        UpdateParentStates(child);
    }

    bool allSelected = true;
    bool allUnselected = true;
    bool anyPartial = false;

    for (const auto& child : node.children) {
        if (child.state == SelectionState::Selected) allUnselected = false;
        else if (child.state == SelectionState::Unselected) allSelected = false;
        else {
            allSelected = false;
            allUnselected = false;
            anyPartial = true;
        }
    }

    if (allSelected) node.state = SelectionState::Selected;
    else if (allUnselected) node.state = SelectionState::Unselected;
    else node.state = SelectionState::Partial;
}

void AppUI::CollectExcludedPaths(const FileNode& node, std::set<std::string>& excluded) {
    if (node.state == SelectionState::Unselected) {
        excluded.insert(node.relativePath);
        return; // No need to check children if the whole folder is out
    }
    for (const auto& child : node.children) {
        CollectExcludedPaths(child, excluded);
    }
}

void AppUI::RefreshFilters() {
    m_ExclVec = NativeIngester::SplitComma(m_Excludes);
    m_InclVec = NativeIngester::SplitComma(m_Includes);
}
