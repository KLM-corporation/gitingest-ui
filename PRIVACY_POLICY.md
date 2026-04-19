# Politique de Confidentialité — GitingestUI

**Dernière mise à jour : 02 Avril 2026**

Cette Politique de Confidentialité décrit comment l'application **GitingestUI** ("l'Application") gère vos informations.

## 1. Collecte de Données
L'Application **ne collecte pas**, ne transmet pas et ne stocke pas vos données personnelles sur des serveurs externes. Toutes les opérations d'ingestion de code sont effectuées **localement** sur votre ordinateur.

## 2. Clés API et Tokens GitHub
Si vous utilisez un Token GitHub Personal Access Token (PAT) pour accéder à des dépôts privés, celui-ci est stocké de manière sécurisée sur votre machine locale via l'API **DPAPI (Data Protection API)** de Windows. Ce token n'est jamais transmis à d'autres services que l'API officielle de GitHub via une connexion sécurisée (HTTPS).

## 3. Fichiers Temporaires
L'Application utilise un dossier caché `.gitingest_temp` pour cloner temporairement les dépôts distants. Ce dossier est automatiquement supprimé à la fermeture de l'application ou lors d'une nouvelle ingestion.

## 4. Services Tiers
L'Application utilise des outils tiers locaux (Python embed, Tiktoken) qui fonctionnent entièrement hors-ligne une fois installés. Aucun traçage ou télémétrie n'est inclus.

## 5. Contact
Pour toute question concernant cette politique, vous pouvez contacter le développeur via le dépôt officiel du projet.
