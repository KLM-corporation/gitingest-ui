# Script de Préparation Packaging GitingestUI
# Ce script rassemble les fichiers nécessaires dans le dossier PackageRoot

$ErrorActionPreference = "Stop"

# Configuration des chemins
$SolutionDir = Get-Location
$BuildDir = Join-Path $SolutionDir "build\Release"
$PackageRoot = Join-Path $SolutionDir "PackageRoot"
$AssetDir = Join-Path $PackageRoot "Assets"

Write-Host "--- Préparation du Package MSIX GitingestUI ---" -ForegroundColor Cyan

# 1. Vérifier que la compilation Release est faite
if (-not (Test-Path -Path (Join-Path $BuildDir "GitingestUI.exe"))) {
    Write-Error "Erreur : GitingestUI.exe introuvable dans build\Release. Compilez en mode Release d'abord."
}

# 2. Nettoyer et Créer le dossier PackageRoot
if (Test-Path -Path $PackageRoot) {
    Write-Host "Nettoyage de l'ancien dossier PackageRoot..."
    Remove-Item -Path $PackageRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $PackageRoot | Out-Null
New-Item -ItemType Directory -Path $AssetDir | Out-Null

Write-Host "Dossiers créés : $PackageRoot"

# 3. Copier les Binaires et Dépendances
Write-Host "Copie des binaires et de python_embed..."
Copy-Item -Path (Join-Path $BuildDir "GitingestUI.exe") -Destination $PackageRoot
Copy-Item -Path (Join-Path $BuildDir "python_embed") -Destination $PackageRoot -Recurse
Copy-Item -Path (Join-Path $BuildDir "gitingest_logo.png") -Destination $PackageRoot

# 4. Préparer le Manifeste
Write-Host "Préparation du manifeste AppxManifest.xml..."
if (Test-Path -Path (Join-Path $SolutionDir "AppxManifest.xml.template")) {
    Copy-Item -Path (Join-Path $SolutionDir "AppxManifest.xml.template") -Destination (Join-Path $PackageRoot "AppxManifest.xml")
    Write-Host "Manifeste copié (N'oubliez pas de remplacer les placeholders INSERT_...)." -ForegroundColor Yellow
}

# 5. Préparer les Assets (Mocker des icônes temporaires à partir du logo principal)
Write-Host "Génération des assets visuels (Placeholders)..."
$MainLogo = Join-Path $BuildDir "gitingest_logo.png"
$AssetsList = @(
    "StoreLogo.png",
    "Square150x150Logo.png", 
    "Square44x44Logo.png", 
    "Wide310x150Logo.png", 
    "SplashScreen.png"
)

foreach ($asset in $AssetsList) {
    Copy-Item -Path $MainLogo -Destination (Join-Path $AssetDir $asset)
}

Write-Host "--- TERMINÉ ---" -ForegroundColor Green
Write-Host "Votre dossier 'PackageRoot' est prêt."
Write-Host "Action suivante : Ouvrez AppxManifest.xml dans le dossier PackageRoot pour remplir vos identifiants."
Write-Host "Pour créer le MSIX final, utilisez la commande :"
Write-Host "MakeAppx pack /d `"$PackageRoot`" /p `"GitingestUI.msix`"" -ForegroundColor Cyan
