#pragma once

// Initialise le système de traduction (anglais par défaut).
void InitI18n();

// Change la langue active. true = Français, false = Anglais.
void SetLanguage(bool french);

// Retourne true si la langue active est le français.
bool IsFrench();

// Retourne la traduction de la chaîne donnée en fonction de la langue active.
const char* tr(const char* key);
