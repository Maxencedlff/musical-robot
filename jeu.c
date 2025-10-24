#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <string.h>  // Pour strtok, strcpy, strlen, strdup
#include <ctype.h>   // Pour isspace, iswspace
#include <time.h>    // Pour la génération aléatoire
#include <strings.h> // Pour strcasecmp (obsolète maintenant)
#include <stdbool.h> // Pour le type 'bool'
#include <locale.h>  // Pour setlocale
#include <wchar.h>   // Pour les caractères larges (wchar_t)
#include <wctype.h>  // Pour towlower, iswspace

// --- Mes identifiants de BDD ---
#define DB_HOST "localhost"
#define DB_USER "root"
#define DB_PASS ""
#define DB_NAME "kaamelott_db"

// --- Structure pour le jeu ---
typedef struct
{
    char personnage[100]; // Nom extrait
    char citation[1024];  // Citation complète
} CitationJeu;

// --- Fonctions utilitaires ---

void clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int compterMots(const char *phrase)
{
    int compte = 0;
    int dansUnMot = 0;
    while (*phrase)
    {
        if (isspace(*phrase))
        {
            dansUnMot = 0;
        }
        else if (dansUnMot == 0)
        {
            dansUnMot = 1;
            compte++;
        }
        phrase++;
    }
    return compte;
}

void extraireNomPersonnage(const char *source, char *destination)
{
    char temp[256];
    strncpy(temp, source, 255);
    temp[255] = '\0';
    char *token = strtok(temp, ",(");
    if (token != NULL)
    {
        strncpy(destination, token, 99);
        destination[99] = '\0';
    }
    else
    {
        strncpy(destination, source, 99);
        destination[99] = '\0';
    }
}

char *trimWhitespace(char *str)
{
    char *end;
    while (isspace((unsigned char)*str))
        str++;
    if (*str == 0)
        return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
    return str;
}

void normaliserChaineW(wchar_t *str)
{
    while (*str)
    {
        wchar_t c = towlower(*str);
        switch (c)
        {
        case L'é':
        case L'è':
        case L'ê':
        case L'ë':
            *str = L'e';
            break;
        case L'à':
        case L'â':
        case L'ä':
            *str = L'a';
            break;
        case L'ù':
        case L'û':
        case L'ü':
            *str = L'u';
            break;
        case L'ç':
            *str = L'c';
            break;
        case L'î':
        case L'ï':
            *str = L'i';
            break;
        case L'ô':
        case L'ö':
            *str = L'o';
            break;
        default:
            *str = c;
            break;
        }
        str++;
    }
}

// --- NOUVELLE FONCTION pour qsort ---
// Fonction de comparaison pour trier la liste des personnages
int compareStrings(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

// --- NOUVELLE FONCTION: AFFICHER LES RÈGLES ---
void afficherRegles()
{
    clearScreen();
    printf("=========================================================\n");
    printf("                        RÈGLES DU JEU\n");
    printf("=========================================================\n\n");
    printf("1. Une citation de Kaamelott va s'afficher.\n");
    printf("2. Vous avez 3 vies pour deviner quel personnage l'a dite.\n");
    printf("3. L'orthographe, les accents et les majuscules ne comptent pas.\n");
    printf("   (Ex: 'Léodagan', 'leodagan' ou 'LEODAGAN' sont acceptés).\n");
    printf("4. Les espaces au début ou à la fin sont ignorés.\n");
    printf("\n--- SYSTÈME DE POINTS (Étape 5) ---\n");
    printf(" * Bonne réponse du premier coup : +10 points\n");
    printf(" * Au deuxième essai             : +5 points\n");
    printf(" * Au troisième essai            : +2 points\n");
    printf("\n--- AIDE EN JEU ---\n");
    printf(" * Pendant le jeu, tapez '?' ou 'liste' pour afficher\n");
    printf("   la liste de tous les personnages.\n");
    printf("\nAppuyez sur Entrée pour revenir au menu...");

    // Attend que l'utilisateur appuie sur Entrée
    char buffer[10];
    fgets(buffer, 9, stdin);
}

// --- NOUVELLE FONCTION: AFFICHER LES PERSONNAGES ---
void afficherPersonnages(char **liste_personnages, int nb_personnages)
{
    clearScreen();
    printf("=========================================================\n");
    printf("             LISTE DES PERSONNAGES UNIQUES (%d)\n", nb_personnages);
    printf("=========================================================\n\n");

    // Affiche les personnages sur 3 colonnes
    int par_colonne = (nb_personnages + 2) / 3;
    for (int i = 0; i < par_colonne; i++)
    {
        // Colonne 1
        if (i < nb_personnages)
        {
            printf("%-25s", liste_personnages[i]);
        }
        // Colonne 2
        if (i + par_colonne < nb_personnages)
        {
            printf("%-25s", liste_personnages[i + par_colonne]);
        }
        // Colonne 3
        if (i + (par_colonne * 2) < nb_personnages)
        {
            printf("%-25s", liste_personnages[i + (par_colonne * 2)]);
        }
        printf("\n");
    }

    printf("\nAppuyez sur Entrée pour revenir...");
    char buffer[10];
    fgets(buffer, 9, stdin);
}

// --- NOUVELLE FONCTION: BOUCLE DE JEU ---
// (Contient l'ancienne boucle de jeu principale)
// --- MODIFIÉE POUR ACCEPTER LA LISTE DES PERSONNAGES ---
void lancerJeu(CitationJeu *citations_chargees, int nb_citations, char **liste_personnages, int nb_personnages)
{
    int score_total = 0;
    bool continuer_jouer = true;
    char buffer_reponse[256];
    wchar_t w_reponse_utilisateur[256];
    wchar_t w_reponse_correcte[256];

    while (continuer_jouer)
    {
        clearScreen();
        int index_aleatoire = rand() % nb_citations;
        CitationJeu *citation_actuelle = &citations_chargees[index_aleatoire];
        int vies = 3;
        bool reponse_trouvee = false;

        printf("Score actuel: %d\n", score_total);
        printf("-------------------------------------------------------\n\n");
        printf("  %s\n\n", citation_actuelle->citation);
        printf("-------------------------------------------------------\n");

        while (vies > 0 && !reponse_trouvee)
        {
            // --- MODIFIÉ: Ajout de l'indice '?' ---
            printf("Qui a dit ça ? (Vies: %d | '?' pour la liste) > ", vies);

            if (fgets(buffer_reponse, 255, stdin) == NULL)
            {
                continuer_jouer = false;
                break;
            }
            buffer_reponse[strcspn(buffer_reponse, "\n")] = 0;

            char *reponse_utilisateur_trimmee = trimWhitespace(buffer_reponse);

            // --- NOUVEAU: Vérification de la demande d'aide ---
            if (strcmp(reponse_utilisateur_trimmee, "?") == 0 ||
                strcmp(reponse_utilisateur_trimmee, "liste") == 0 ||
                strcmp(reponse_utilisateur_trimmee, "aide") == 0)
            {
                // 1. Afficher la liste des personnages
                afficherPersonnages(liste_personnages, nb_personnages);

                // 2. Redessiner l'écran de jeu actuel
                clearScreen();
                printf("Score actuel: %d\n", score_total);
                printf("-------------------------------------------------------\n\n");
                printf("  %s\n\n", citation_actuelle->citation);
                printf("-------------------------------------------------------\n");

                // 3. Recommencer la boucle sans perdre de vie
                continue;
            }
            // --- FIN DE LA VÉRIFICATION ---

            mbstowcs(w_reponse_utilisateur, reponse_utilisateur_trimmee, 255);
            mbstowcs(w_reponse_correcte, citation_actuelle->personnage, 255);

            normaliserChaineW(w_reponse_utilisateur);
            normaliserChaineW(w_reponse_correcte);

            if (wcscmp(w_reponse_utilisateur, w_reponse_correcte) == 0)
            {
                reponse_trouvee = true;
                printf("\nBonne réponse ! C'était bien %s.\n", citation_actuelle->personnage);
                if (vies == 3)
                {
                    printf("+10 points !\n");
                    score_total += 10;
                }
                else if (vies == 2)
                {
                    printf("+5 points.\n");
                    score_total += 5;
                }
                else
                {
                    printf("+2 points.\n");
                    score_total += 2;
                }
            }
            else
            {
                vies--;
                printf("\nMauvaise réponse !\n");
            }
        }

        if (!reponse_trouvee)
        {
            printf("\nVous n'avez plus de vies. La réponse était : %s\n", citation_actuelle->personnage);
        }

        printf("\nAppuyez sur Entrée pour la citation suivante (ou 'q' pour quitter) > ");
        if (fgets(buffer_reponse, 255, stdin) == NULL)
        {
            continuer_jouer = false;
        }
        if (buffer_reponse[0] == 'q' || buffer_reponse[0] == 'Q')
        {
            continuer_jouer = false;
        }
    }

    // FIN DE LA PARTIE
    clearScreen();
    printf("=========================================================\n");
    printf("                PARTIE TERMINÉE\n");
    printf("               Votre score final est de : %d\n", score_total);
    printf("=========================================================\n");
    printf("Merci d'avoir joué !\n");
    printf("\nAppuyez sur Entrée pour revenir au menu...");
    char buffer[10];
    fgets(buffer, 9, stdin);
}

// --- Fonction Principale ---
int main()
{
    // Configuration des caractères spéciaux
    setlocale(LC_ALL, ".UTF-8");
    system("chcp 65001 > nul");
    srand(time(NULL));

    MYSQL *con = mysql_init(NULL);
    CitationJeu *citations_chargees = NULL;
    int nb_citations = 0;

    // --- NOUVEAU: Pour la liste des personnages uniques ---
    char *liste_personnages[500];               // Max 500 personnages uniques
    wchar_t w_liste_personnages_norm[500][100]; // Pour la vérification des doublons
    int nb_personnages_uniques = 0;
    // ---------------------------------------------------

    if (con == NULL)
    {
        fprintf(stderr, "Erreur mysql_init()\n");
        exit(1);
    }

    // ÉTAPE 3.1: CONNEXION
    if (mysql_real_connect(con, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0) == NULL)
    {
        fprintf(stderr, "Erreur de connexion à la BDD: %s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }
    if (mysql_set_character_set(con, "utf8mb4"))
    {
        fprintf(stderr, "Erreur lors du réglage du charset en utf8mb4: %s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }
    printf("Connexion réussie à la base de données '%s' !\n", DB_NAME);

    // ÉTAPE 3.2: CHARGEMENT DES CITATIONS
    if (mysql_query(con, "SELECT `character`, `quote` FROM citations"))
    {
        fprintf(stderr, "Erreur de requête SQL: %s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }

    MYSQL_RES *result = mysql_store_result(con);
    if (result == NULL)
    {
        fprintf(stderr, "Erreur mysql_store_result: %s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }

    MYSQL_ROW row;

    // PREMIÈRE PASSE (juste pour compter)
    printf("Comptage des citations valides (>= 5 mots)...\n");
    while ((row = mysql_fetch_row(result)))
    {
        if (compterMots(row[1]) >= 5)
        {
            nb_citations++;
        }
    }
    printf("%d citations valides trouvées.\n", nb_citations);

    if (nb_citations == 0)
    {
        printf("Aucune citation valide...\n");
        exit(0);
    }

    // ALLOCATION MÉMOIRE
    printf("Allocation de la mémoire pour %d citations...\n", nb_citations);
    citations_chargees = (CitationJeu *)malloc(nb_citations * sizeof(CitationJeu));
    if (citations_chargees == NULL)
    {
        fprintf(stderr, "Erreur critique: Échec de l'allocation mémoire !\n");
        exit(1);
    }

    // DEUXIÈME PASSE (pour copier)
    mysql_data_seek(result, 0);
    int index_citations = 0;
    printf("Chargement, copie et extraction des personnages uniques...\n");
    while ((row = mysql_fetch_row(result)))
    {
        if (compterMots(row[1]) >= 5)
        {
            char nom_propre[100];
            extraireNomPersonnage(row[0], nom_propre);
            char *nom_propre_trimme = trimWhitespace(nom_propre);

            // Copie de la citation
            strcpy(citations_chargees[index_citations].personnage, nom_propre_trimme);
            strcpy(citations_chargees[index_citations].citation, row[1]);
            index_citations++;

            // --- NOUVEAU: Logique pour les personnages uniques ---
            wchar_t w_nom_normalise[100];
            mbstowcs(w_nom_normalise, nom_propre_trimme, 99);
            normaliserChaineW(w_nom_normalise);

            bool est_deja_liste = false;
            for (int i = 0; i < nb_personnages_uniques; i++)
            {
                if (wcscmp(w_nom_normalise, w_liste_personnages_norm[i]) == 0)
                {
                    est_deja_liste = true;
                    break;
                }
            }

            if (!est_deja_liste && nb_personnages_uniques < 500)
            {
                // strdup alloue de la mémoire et copie la chaîne
                liste_personnages[nb_personnages_uniques] = strdup(nom_propre_trimme);
                wcscpy(w_liste_personnages_norm[nb_personnages_uniques], w_nom_normalise);
                nb_personnages_uniques++;
            }
            // --- Fin de la logique unique ---
        }
    }
    printf("Chargement terminé. %d citations copiées, %d personnages uniques trouvés.\n", index_citations, nb_personnages_uniques);
    mysql_free_result(result);
    mysql_close(con);

    // --- NOUVEAU: Tri de la liste des personnages ---
    printf("Tri de la liste des personnages...\n");
    qsort(liste_personnages, nb_personnages_uniques, sizeof(char *), compareStrings);

    // --- NOUVEAU: MENU PRINCIPAL ---
    char buffer_menu[10];
    while (true)
    {
        clearScreen();
        printf("=========================================================\n");
        printf("               BIENVENUE DANS LE JEU KAAMELOTT\n");
        printf("            (Basé sur %d citations chargées)\n", nb_citations);
        printf("=========================================================\n\n");
        printf("  1. Jouer\n");
        printf("  2. Afficher les règles\n");
        printf("  3. Afficher la liste des personnages (%d)\n", nb_personnages_uniques);
        printf("  4. Quitter\n");

        // --- MODIFICATION ICI ---
        printf("\nVotre choix (1-4) > "); // Plus explicite

        if (fgets(buffer_menu, 9, stdin) == NULL)
        {
            break;
        };

        switch (atoi(buffer_menu))
        {
        case 1:
            // --- MODIFIÉ: On passe la liste des personnages au jeu ---
            lancerJeu(citations_chargees, nb_citations, liste_personnages, nb_personnages_uniques);
            break;
        case 2:
            afficherRegles();
            break;
        case 3:
            afficherPersonnages(liste_personnages, nb_personnages_uniques);
            break;
        case 4:
            goto cleanup; // Saute à la section de nettoyage
        default:
            printf("\nChoix invalide. Tapez 1, 2, 3 ou 4. Appuyez sur Entrée pour réessayer...");
            // On utilise fgets pour vider le buffer au cas où l'utilisateur
            // aurait tapé plus de 9 caractères
            fgets(buffer_menu, 9, stdin);
        }
    }

cleanup:
    // --- FIN DU JEU (Section de nettoyage) ---
    clearScreen();
    printf("Libération de la mémoire...\n");

    // Libère la mémoire de la liste des personnages (strdup)
    for (int i = 0; i < nb_personnages_uniques; i++)
    {
        free(liste_personnages[i]);
    }

    // Libère la mémoire des citations (malloc)
    free(citations_chargees);

    printf("Au revoir !\n");
    return 0;
}