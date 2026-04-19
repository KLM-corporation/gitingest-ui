#include "SystemUtils.h"
#include <windows.h>
#include <shobjidl.h>
#include <wincrypt.h>
#include <shellapi.h>
#include <array>
#include <iostream>
#include <vector>
#include <functional>

std::string SystemUtils::SelectDirectory() {
    std::string path = "";
    
    // Initialisation du thread COM (nécessaire pour IFileOpenDialog)
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileOpenDialog* pFileOpen;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
        
        if (SUCCEEDED(hr)) {
            DWORD dwOptions;
            if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions))) {
                // Mode de sélection de dossiers uniquement
                pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
            }
            
            // Affichage de la boîte de dialogue
            if (SUCCEEDED(pFileOpen->Show(NULL))) {
                IShellItem* pItem;
                if (SUCCEEDED(pFileOpen->GetResult(&pItem))) {
                    PWSTR pszFilePath;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                        // Conversion du chemin (chaîne large vers UTF-8 pour C++)
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                        path.resize(size_needed - 1); // Sans le null-terminator
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &path[0], size_needed, NULL, NULL);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return path;
}

std::string SystemUtils::SaveFileDialog() {
    std::string path = "";
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileSaveDialog* pFileSave;
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
        if (SUCCEEDED(hr)) {
            // Configuration de l'extension par defaut
            COMDLG_FILTERSPEC rgSpec[] = { { L"Fichier Texte (*.txt)", L"*.txt" }, { L"Tous les fichiers (*.*)", L"*.*" } };
            pFileSave->SetFileTypes(2, rgSpec);
            pFileSave->SetDefaultExtension(L"txt");
            pFileSave->SetFileName(L"digest.txt");
            
            if (SUCCEEDED(pFileSave->Show(NULL))) {
                IShellItem* pItem;
                if (SUCCEEDED(pFileSave->GetResult(&pItem))) {
                    PWSTR pszFilePath;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                        path.resize(size_needed - 1);
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &path[0], size_needed, NULL, NULL);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileSave->Release();
        }
        CoUninitialize();
    }
    return path;
}

bool SystemUtils::ExecuteCommand(const std::string& command, std::function<void(const std::string& out)> callback) {
    // Version Windows sécurisée avec CreateProcessW
    // pApplicationName à NULL pour laisser Windows chercher dans le PATH
    
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;

    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) return false;
    if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) return false;

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOW siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOW));
    siStartInfo.cb = sizeof(STARTUPINFOW);
    siStartInfo.hStdError = hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Conversion de la commande en WideChar pour CreateProcessW
    int wlen = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, NULL, 0);
    std::vector<wchar_t> wcmd(wlen);
    MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, wcmd.data(), wlen);

    if (!CreateProcessW(NULL, wcmd.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &siStartInfo, &piProcInfo)) {
        CloseHandle(hChildStd_OUT_Rd);
        CloseHandle(hChildStd_OUT_Wr);
        return false;
    }

    CloseHandle(hChildStd_OUT_Wr); // Fermer le côté écriture dans le parent

    char buffer[4096];
    DWORD dwRead;
    while (ReadFile(hChildStd_OUT_Rd, buffer, sizeof(buffer) - 1, &dwRead, NULL) && dwRead > 0) {
        buffer[dwRead] = '\0';
        if (callback) callback(std::string(buffer));
    }

    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(piProcInfo.hProcess, &exitCode);

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
    CloseHandle(hChildStd_OUT_Rd);

    if (exitCode != 0 && callback) {
        callback("\n[Processus terminé avec le code d'erreur : " + std::to_string(exitCode) + "]\n");
    }

    return true;
}

int SystemUtils::CountTokensTiktoken(const std::string& text) {
    // Trouver le chemin de l'executable courant pour localiser python_embed/
    char exePath[MAX_PATH] = {};
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) return -1;
    
    std::string exeDir(exePath);
    size_t lastSlash = exeDir.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        exeDir = exeDir.substr(0, lastSlash);

    std::string embPython = exeDir + "\\python_embed\\python.exe";

    // Verifier que le Python embeddable existe
    DWORD attrs = GetFileAttributesA(embPython.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return -1;
    }

    // 1. Ecriture du texte dans un fichier temporaire
    std::string dataPath = exeDir + "\\.gitingest_tok_data.tmp";
    {
        FILE* f = fopen(dataPath.c_str(), "wb");
        if (!f) return -1;
        fwrite(text.data(), 1, text.size(), f);
        fclose(f);
    }

    // 2. Ecriture du script Python dans un fichier temporaire
    // On utilise r"" pour eviter les problemes de backslashes Windows
    std::string scriptPath = exeDir + "\\.gitingest_tok_script.py";
    {
        FILE* f = fopen(scriptPath.c_str(), "w");
        if (!f) {
            remove(dataPath.c_str());
            return -1;
        }
        fprintf(f, 
            "import sys\n"
            "try:\n"
            "    import tiktoken\n"
            "    enc = tiktoken.get_encoding('cl100k_base')\n"
            "    with open(r'%s', 'r', encoding='utf-8', errors='ignore') as f:\n"
            "        data = f.read()\n"
            "    print(len(enc.encode(data, disallowed_special=())))\n"
            "except Exception:\n"
            "    sys.exit(1)\n", 
            dataPath.c_str());
        fclose(f);
    }

    // 3. Execution du script
    std::string cmd = "\"" + embPython + "\" \"" + scriptPath + "\"";
    
    int result = -1;
    std::string fullOutput;
    ExecuteCommand(cmd, [&fullOutput](const std::string& out) {
        fullOutput += out;
    });

    if (!fullOutput.empty()) {
        result = atoi(fullOutput.c_str());
    }

    // Nettoyage
    remove(dataPath.c_str());
    remove(scriptPath.c_str());

    return (result > 0) ? result : -1;
}

std::string SystemUtils::EncryptString(const std::string& input) {
    if (input.empty()) return "";

    DATA_BLOB dataIn;
    dataIn.pbData = (BYTE*)input.data();
    dataIn.cbData = (DWORD)input.size();

    DATA_BLOB dataOut;
    if (CryptProtectData(&dataIn, L"GitingestUI Token", NULL, NULL, NULL, 0, &dataOut)) {
        std::string result((char*)dataOut.pbData, dataOut.cbData);
        LocalFree(dataOut.pbData);
        return result;
    }
    return "";
}

std::string SystemUtils::DecryptString(const std::string& input) {
    if (input.empty()) return "";

    DATA_BLOB dataIn;
    dataIn.pbData = (BYTE*)input.data();
    dataIn.cbData = (DWORD)input.size();

    DATA_BLOB dataOut;
    if (CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut)) {
        std::string result((char*)dataOut.pbData, dataOut.cbData);
        LocalFree(dataOut.pbData);
        return result;
    }
    return "";
}

void SystemUtils::OpenURL(const std::string& url) {
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

std::string SystemUtils::MaskSensitive(const std::string& input, const std::string& token) {
    if (token.empty() || input.empty()) return input;
    std::string result = input;
    size_t pos = 0;
    while ((pos = result.find(token, pos)) != std::string::npos) {
        result.replace(pos, token.length(), "***");
        pos += 3;
    }
    return result;
}

std::string SystemUtils::Base64Encode(const std::string& input) {
    if (input.empty()) return "";
    DWORD len = 0;
    CryptBinaryToStringA((const BYTE*)input.data(), (DWORD)input.size(),
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &len);
    if (len == 0) return "";
    std::string result(len, '\0');
    CryptBinaryToStringA((const BYTE*)input.data(), (DWORD)input.size(),
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &result[0], &len);
    result.resize(len);
    while (!result.empty() && (result.back() <= ' ' || result.back() == '\0'))
        result.pop_back();
    return result;
}

std::string SystemUtils::Base64Decode(const std::string& input) {
    if (input.empty()) return "";
    DWORD len = 0;
    CryptStringToBinaryA(input.c_str(), 0, CRYPT_STRING_BASE64, NULL, &len, NULL, NULL);
    if (len == 0) return "";
    std::string result(len, '\0');
    CryptStringToBinaryA(input.c_str(), 0, CRYPT_STRING_BASE64, (BYTE*)&result[0], &len, NULL, NULL);
    result.resize(len);
    return result;
}

bool SystemUtils::IsValidGitUrl(const std::string& url) {
    // Must start with a valid protocol
    if (url.find("https://") != 0 && url.find("http://") != 0 && url.find("git://") != 0)
        return false;
    // Block shell metacharacters that could allow command injection
    const std::string forbidden = "&|;`$(){}[]!><\"'\\";
    for (char c : url) {
        if (forbidden.find(c) != std::string::npos)
            return false;
    }
    return true;
}
