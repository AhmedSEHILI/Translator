#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ncurses.h>

#define API_KEY "AIzaSyBGfF7FmIbaQ-N44u3rn3pAqdTH1WI3nM4"

void translate_word(const char *word) {
    int pipefd[2];
    char response[2048]; // Augmentez la taille si nécessaire
    char translation[2048];

    // Création du pipe pour la communication entre le parent et l'enfant
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Création d'un processus enfant
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Processus enfant
        // Fermer l'extrémité de lecture inutilisée du pipe
        close(pipefd[0]);

        // Redirection de la sortie standard vers le pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        // Construire la commande curl avec le mot saisi
        char curl_command[1024];
        sprintf(curl_command, "curl -s -X POST -H \"Content-Type: application/json\" -d "
                               "'{ \"q\": \"%s\", \"source\": \"en\", \"target\": \"fr\" }' "
                               "'https://translation.googleapis.com/language/translate/v2?key=" API_KEY "'", word);
        
        // Exécuter la commande curl pour traduire le mot
        execlp("sh", "sh", "-c", curl_command, NULL);
        
        // En cas d'échec de l'exécution de curl
        perror("execlp");
        exit(EXIT_FAILURE);
    } else { // Processus parent
        // Fermer l'extrémité d'écriture inutilisée du pipe
        close(pipefd[1]);

        // Lecture de la réponse JSON à partir du pipe
        read(pipefd[0], response, sizeof(response));

        // Fermeture de l'extrémité de lecture du pipe
        close(pipefd[0]);


        // extraction de la traduction
        char *start = strstr(response, "\"translatedText\":");

        int i= start-response + strlen("\"translatedText\":")+2;
        int j =0;
        while(response[i]!='"'){
            translation[j] = response[i];
            i++;
            j++;
        }        
        translation[j] = '\0';
        printw("%s ", translation);
        refresh();

        // Attendre la fin du processus enfant
        // la terminaison du fils me semble correct ici
        wait(NULL);

    }
}

int main(void) {
    char input[1024];

    // Initialisation de ncurses
    initscr();
    cbreak(); // Permet une saisie en temps réel sans mise en mémoire tampon
    noecho(); // Désactive l'affichage des caractères saisis

    printw("Entrez du texte (appuyez sur 'q' pour quitter) :\n");
    refresh();
    
    //nettoyage du buffer en entrée
    memset(input, 0, sizeof(input));

    // Lecture de caractères jusqu'à ce que l'utilisateur appuie sur 'q'
    while (1) {

        char c = getch(); // Récupère un caractère sans attendre la touche "Entrée"

        // Vérifier si l'utilisateur a appuyé sur 'q' pour quitter
        if (c == 'q') {
            printw("\nVous avez appuyé sur 'q'. Sortie...\n");
            break;
        }

        // Ajouter le caractère à l'entrée utilisateur
        strncat(input, &c, 1);

        // on trasuit lorsque on rencontre une virgule, un point, ou d'autre signe de ponctuation qui indique un fin de contexte
        if (c == ',' || c == '\n' || c == '.' || c == '?' || c == '!') {

            // Remplacer le dernier caractère ('\n') par un terminateur de chaîne
            input[strlen(input) - 1] = '\0';
            
            // Traduire le mot et afficher la traduction
            translate_word(input);

            // Effacer le contenu de l'entrée utilisateur pour le prochain mot
            memset(input, 0, sizeof(input));
        }
    }

    // Nettoyage de ncurses
    endwin();

    return 0;
}

