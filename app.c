
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ncurses.h>
#include <pthread.h>
#include <time.h>


// Clé d'API google traduction
#define API_KEY "AIzaSyBGfF7FmIbaQ-N44u3rn3pAqdTH1WI3nM4"

// des variables globales pour la gestion de la traduction
signed int delimitationIndex = 0;  // cette variable garde la position du dernier délimitateur dans le texte entré par l'utilisteur
signed int previousLen = 0;   // cette variable garde l'ancienne longueur du texte entré par l'utilisateur
signed int nbrMots = 0;


char response[2048]; // cette chaine de caractères va contenir la traduction du texte entré par l'uitlisateur
char textcorrected[2048];   // cette chaine de caractères va contenir le texte entré par l'utilisateur après sa correction


// ces deux variables vont contenir les langues source et destination de la traduction
char langdest[3] = "fr\0"; // initialement initalisées : anglais vers français
char langsrc[3] = "en\0";



// Ce tableau contient tous les codes des langues acceptées par l'API de google traduction
const char *codeLG[] = {
    "af", "sq", "de", "am", "en", "ar", "hy", "az", "eu", "bn",
    "be", "my", "bs", "bg", "ca", "ceb", "ny", "zh-CN", "zh-TW", "si",
    "ko", "co", "ht", "hr", "da", "es", "eo", "et", "fi", "fr",
    "fy", "gl", "cy", "ka", "gu", "el", "ha", "haw", "iw", "hi",
    "hmn", "hu", "ig", "id", "ga", "is", "it", "ja", "jw", "kn",
    "kk", "km", "ky", "ku", "lo", "la", "lv", "lt", "lb", "mk",
    "ms", "ml", "mg", "mt", "mi", "mr", "mn", "nl", "ne", "no",
    "uz", "ps", "pa", "fa", "pl", "pt", "ro", "ru", "sm", "sr",
    "st", "sn", "sd", "sk", "sl", "so", "su", "sv", "sw", "tg",
    "tl", "ta", "cs", "te", "th", "tr", "uk", "ur", "vi", "xh",
    "yi", "yo", "zu"
};



// cette fonction ajoute les langues supportées par google traduction au comboboxes de l'application gtk
void ajouter_langues_a_combobox(GtkComboBox *combobox) {
// Liste des langues supportées par Google Traduction
    const gchar *langues[] = {
        "Afrikaans", "Albanais", "Allemand", "Amharique", "Anglais", "Arabe", "Arménien", "Azéri", "Basque", "Bengali",
        "Biélorusse", "Birman", "Bosniaque", "Bulgare", "Catalan", "Cebuano", "Chichewa", "Chinois (simplifié)", "Chinois (traditionnel)", "Cinghalais",
        "Coréen", "Corse", "Créole haïtien", "Croate", "Danois", "Espagnol", "Espéranto", "Estonien", "Finnois", "Français",
        "Frison", "Galicien", "Gallois", "Géorgien", "Goudjrati", "Grec", "Haoussa", "Hawaïen", "Hébreu", "Hindi",
        "Hmong", "Hongrois", "Igbo", "Indonésien", "Irlandais", "Islandais", "Italien", "Japonais", "Javanais", "Kannada",
        "Kazakh", "Khmer", "Kirghize", "Kurde", "Laotien", "Latin", "Letton", "Lituanien", "Luxembourgeois", "Macédonien",
        "Malaisien", "Malayalam", "Malgache", "Maltais", "Maori", "Marathi", "Mongol", "Néerlandais", "Népalais", "Norvégien",
        "Ouzbek", "Pachtô", "Pendjabi", "Persan", "Polonais", "Portugais", "Roumain", "Russe", "Samoan", "Serbe",
        "Sesotho", "Shona", "Sindhi", "Slovaque", "Slovène", "Somali", "Soundanais", "Suédois", "Swahili", "Tadjik",
        "Tagalog", "Tamoul", "Tchèque", "Télougou", "Thaï", "Turc", "Ukrainien", "Urdu", "Vietnamien", "Xhosa",
        "Yiddish", "Yoruba", "Zoulou"
    };

    // Nombre total de langues
    gint nb_langues = G_N_ELEMENTS(langues);

    // Ajout des langues à la combobox
    for (int i = 0; i < nb_langues; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), langues[i]);
    }
}






static void
quit_cb (GtkWindow *window)
{
  gtk_window_close (window);
}

void decode_html_entities(char *str) {
    char *p = str;

    while (*str) {
        if (strncmp(str, "&#39;", 5) == 0) {
            *p++ = '\'';
            str += 5;
        } else {
            *p++ = *str++;
        }
    }

    *p = '\0';
}


// cette fonction gère le signal de la fermeture de la fenetre gtk, en effet, elle conserve la dernière traduction effectué dans le fichier recovery, afin de la charger lors de la prochaine utilisation

static void on_window_closed(GtkWidget *widget, gpointer data) {
    

    FILE *fichier = fopen("recovery.txt", "r+");
    fseek(fichier, 0, SEEK_END);
    if (fichier != NULL) {

        // ici on écrit le texte corrigé, ensuite le text traduit

        if(strlen(textcorrected)>0) fprintf(fichier,"%s", textcorrected);
        if(strlen(response)>0) fprintf(fichier,"\n%s\n", response);


    
        // cette partie du code efface les deux première ligne du fichier (un texte corrigé+sa traduction) si le nombre de traduction conservé atteint 10 (20 ligne de texte + traduction), donc on conserve toujours 10 traduction, et on efface a chaque fois l'ancienne

        rewind(fichier);
    
        int nbLignes = 0;
        char ligne[2048];
        while (fgets(ligne, 2048, fichier) != NULL) {
            nbLignes++;
        }

        // Retour au début du fichier
        rewind(fichier);

        if (nbLignes >= 22) { 
            // Ignorer les deux premières lignes
            
            fgets(ligne, 2048, fichier);
            fgets(ligne, 2048, fichier);

            int pos = 0;
            long actualpos;
            while (fgets(ligne, 2048, fichier) != NULL) {

                actualpos = ftell(fichier);
                fseek(fichier, pos, SEEK_SET);
                fputs(ligne, fichier);
                pos = ftell(fichier);
                fseek(fichier, actualpos, SEEK_SET);
            }

            // Tronquer le fichier à la taille appropriée
            ftruncate(fileno(fichier), pos);
        }


        fclose(fichier);


    } 
    else {

        g_print("Erreur lors de l'ouverture du fichier.\n");
    }
}


// Les deux caractère ' et " pose problème lors de la traduction, cette fonction sert a formater le texte avant de le traduire
void formaterString(char str[]) {
    
    for(int i =0; i<strlen(str); i++){
        
        if (str[i] == '\'' || str[i] == '\"') {
            str[i] = ' ';
        }
    }
}


// cette fonction fait l'extraction du texte traduit (resultat de la réponse de l'api) du format json
void text_extraction(char* jsonResponse, char* translation){

        char *start = strstr(jsonResponse, "\"translatedText\":");

        int i= start-jsonResponse + strlen("\"translatedText\":")+2;
        int j =0;
        while(jsonResponse[i]!='"'){     // un problème si le texte entré contient le caractère "
            translation[j] = jsonResponse[i];
            i++;
            j++;
        }        
        translation[j] = '\0';



}




// c'est la fonction principale de traduction, elle prend un texte en entrée, crée deux processus, le premier contacte l'api pour faire la traduction, elle transmet le résultat de la traduction a l'autre processus pour extraire la traduction de la format json, et la mettre dans la variable translation

void translate_word(char *text, char* translation, char* source_lang, char* target_lang) {

    
    int pipefd[2];
    char response[2048]; 

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

        formaterString(text); 

        sprintf(curl_command, "curl -s -X POST -H \"Content-Type: application/json\" -d "
                           "'{ \"q\": \"%s\", \"source\": \"%s\", \"target\": \"%s\" }' "
                           "'https://translation.googleapis.com/language/translate/v2?key=" API_KEY "'",
                            text, source_lang, target_lang);
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

        text_extraction(response, translation);


        // Attendre la fin du processus enfant
        wait(NULL);

    }
}


// La routine d'un threas de traduction
void* threadRoutineTranslate(void* arg){

    char *string = (char *)arg;


    translate_word(string, string, langsrc, langdest);

    pthread_exit(NULL);
}

// La routine d'un threas de correction de texte
void* threadRoutineCorrection(void* arg){

    char *string = (char *)arg;


    translate_word(string, string, langdest, langsrc);

    pthread_exit(NULL);
}


//cette fonction utilise le multithreaing pour effectuer la traduction d'un texte en utilisant la fonction translate_word,
//elle est utilisé si l'utilisateur efface un caractère du texte existant, ou bien si il colle dirrectement un texte dans l'entrée, dans ces deux cas, on crée autant de thread  que de phrase dans le texte (une phrase est une chaine de caractère entre deux délimitateurs", ? ! ...")
void advancedTranslator(char **strings, char* text, int len, char* result){

    char destination[2048];



    if(len <= previousLen-1){  // si l'utilisateur efface un caractère


            strcpy(destination, text);
    
            strcpy(strings[0], destination);

            int j, i =0, cpt=0;
            while(destination[i]!='\0'){
                
                if(destination[i]==',' || destination[i]=='.' ||destination[i]=='!' ||destination[i]=='?' ||destination[i]==':' || destination[i]=='?' || destination[i]==';'){

                    for(int j=0; j<=i; j++){
                        strings[cpt][j] = destination[j];
                    }
                    strings[cpt][i+1] = '\0';
                    j = i+1;

                    while(destination[j]!='\0'){
                        destination[j-i-1] = destination[j];
                        j++;
                    }
                    destination[j-i-1] = '\0';


                    cpt++;

                    i = -1;
                }
                 
                i++;
            }
            strcpy(strings[cpt], destination);
            previousLen = len;


        
            pthread_t* tabThreads =  malloc((cpt+1) * sizeof(pthread_t));

            for (int i = 0; i <= cpt; i++) {

                pthread_create(&tabThreads[i], NULL, threadRoutineTranslate, (void *)strings[i]);
            }
            
            for (int i = 0; i <= cpt; i++) {

                pthread_join(tabThreads[i],NULL);
            }


            strcpy(result, "");


            for (int i = 0; i <= cpt; i++) {
                
               strcat(result, strings[i]);

            }


            delimitationIndex = len;
        
            for (int i = 0; i < cpt; i++) {
                free(strings[i]);
            }
            free(strings);
            free(tabThreads);
    }  
    
    else{ // si l'utilisateur colle directement un texte dans l'entrée
            
            previousLen = delimitationIndex;

    
            strcpy(destination, text+delimitationIndex);
    
            strcpy(strings[0], destination);

            int j, i =0, cpt=0;
            while(destination[i]!='\0'){
                
                if(destination[i]==',' || destination[i]=='.' ||destination[i]=='!' ||destination[i]=='?' ||destination[i]==':' || destination[i]=='?' || destination[i]==';'){

                    for(int j=0; j<=i; j++){
                        strings[cpt][j] = destination[j];
                    }
                    strings[cpt][i+1] = '\0';
                    j = i+1;

                    while(destination[j]!='\0'){
                        destination[j-i-1] = destination[j];
                        j++;
                    }
                    destination[j-i-1] = '\0';


                    cpt++;

                    i = -1;
                }
                 
                i++;
            }
            strcpy(strings[cpt], destination);

            for (int i = 0; i <= cpt; i++){


                previousLen= previousLen + strlen(strings[i]);
            } 

         
            pthread_t* tabThreads =  malloc((cpt+1) * sizeof(pthread_t));

            for (int i = 0; i <= cpt; i++) {

                pthread_create(&tabThreads[i], NULL, threadRoutineTranslate, (void *)strings[i]);
            }
            
            for (int i = 0; i <= cpt; i++) {

                pthread_join(tabThreads[i],NULL);
            }


            for (int i = 0; i <= cpt; i++) {
                
               strcat(result, strings[i]);

            }


            delimitationIndex = len;
        

            free(tabThreads);


    }



}



// cette fonction distingue les deux cas possible de traduction, si l'utilisateur entre le texte caractère par caractère, et s'il efface ou colle directement un texte dans l'entrée
// dans le premier cas, la traduction est faite a chaque fois qu'on rencontre un délimitateur de fin de phrase (contexte), ou si le nombre de mot atteint 5 sans sans avoir rencontrer un délimitateur
// dans le deuxième cas, on appelle la fonction advancedTranslator qui fait le travail en multithreading
void TranslatorCorrector(char* text, int len){


    char destination[2048];
    char translation[2048];
    char correction[2048];

    if(text[len-1] == ' ') nbrMots = nbrMots+1;

    
    if(len == previousLen+1){


        if(text[len-1]==',' || text[len-1]=='.' ||text[len-1]=='!' ||text[len-1]=='?' ||text[len-1]==':' || text[len-1]=='?' ||text[len-1]==';'){

            strcpy(destination, text+delimitationIndex);
            translate_word(destination, translation, langsrc, langdest);
            strcat(response, translation);


            strcpy(correction, translation);
            strcpy(translation, "");
            translate_word(correction, translation, langdest, langsrc);
            strcat(textcorrected, translation);

            delimitationIndex = len;
        }
        else if (nbrMots == 5){
        
            strcpy(destination, text+delimitationIndex);
            translate_word(destination, translation, langsrc, langdest);
            strcat(response, translation);

            strcpy(correction, translation);
            strcpy(translation, "");
            translate_word(correction, translation, langdest, langsrc);
            strcat(textcorrected, translation);
            
            delimitationIndex = len;
            nbrMots = 0;
        }


        previousLen++;
        
    }
    
    /************** Si l'utilisateur colle directement un texte, ou s'il efface un car ********************/
    
    else{

        char **stringsTranslator = malloc(1024 * sizeof(char*));
        for (int i = 0; i < 1024; i++) stringsTranslator[i] = malloc(2048 * sizeof(char));


        advancedTranslator(stringsTranslator, text, len, response);

        translate_word(response, textcorrected, langdest, langsrc);




    }

}







//cette fonction est appelée a chaque fois qu'un signal de changement de le buffer d'entrée est reçu, et elle fait la traduction
void on_entry_changed_translate(GtkTextBuffer *buffer, gpointer data) {

    GtkWidget *txtview = GTK_WIDGET(data);
    GtkTextBuffer *buffertxt = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtview));

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);

    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    int len = strlen(text);



    TranslatorCorrector(text, len);


    decode_html_entities(response);
    gtk_text_buffer_set_text(buffertxt, response, -1);
    
}   

//cette fonction est appelée a chaque fois qu'un signal de changement de le buffer d'entrée est reçu, et elle fait la correction du texte
void on_entry_changed_correct(GtkTextBuffer *buffer, gpointer data) {

    GtkWidget *txtview = GTK_WIDGET(data);
    GtkTextBuffer *buffertxt = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtview));

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);

    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    int len = strlen(text);



    decode_html_entities(textcorrected);
    gtk_text_buffer_set_text(buffertxt, textcorrected, -1);


}   



//cette fonction est appelée a chaque fois qu'un signal de changement dans le combo box de la langue source est reçu, c'est a dire a chaque fois que la langue source est changée.

void on_combosrc_changed(GtkComboBox *combobox, gpointer user_data) {


    gint active_index = gtk_combo_box_get_active(combobox);
    
    // ici on accepte pas que les langues source et destination soient les même car l'api ne l'accepte pas
    if(strcmp(langdest, codeLG[active_index])){
      strcpy(langsrc, codeLG[active_index]);
    }

}

//cette fonction est appelée a chaque fois qu'un signal de changement dans le combo box de la langue destination est reçu, c'est a dire a chaque fois que la langue destination est changée.

void on_combodest_changed(GtkComboBox *combobox, gpointer user_data) {

    gint active_index = gtk_combo_box_get_active(combobox);

    // ici on accepte pas que les langues source et destination soient les même car l'api ne l'accepte pas
    if(strcmp(langsrc, codeLG[active_index])){
      strcpy(langdest, codeLG[active_index]);
    }

}

//fonction principale de l'application gtk, elle active dès le début de programme
static void activate (GtkApplication *app, gpointer user_data){

    strcpy(response, "");
    strcpy(textcorrected, "");

    GtkCssProvider *provider;
    GdkDisplay *display;

    provider = gtk_css_provider_new ();
    display = gdk_display_get_default ();

    gtk_style_context_add_provider_for_display (display, GTK_STYLE_PROVIDER (provider),          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_css_provider_load_from_path(provider, "styles.css");

    GtkStyleContext *context;

  
  // certaint widgets de l'interface sont importés directement a partir du fichier myapp.ui déja conçu
  GtkBuilder *builder = gtk_builder_new();
  gtk_builder_add_from_file (builder, "myapp.ui", NULL);


  // importation de la fenetre principale de l'application
  GObject *window = gtk_builder_get_object (builder, "window");
  gtk_window_set_application (GTK_WINDOW (window), app);
    


  // importation du texte view qui va contenir la traduction
  GObject *txtview = gtk_builder_get_object(builder, "txtview");

  // importation du texte view qui va contenir le texte corrigé
  GObject *corrected = gtk_builder_get_object(builder, "corrected");

  // importation du texte view qui va contenir le texte entré par l'utilisateur
  GObject *enterhere = gtk_builder_get_object(builder, "enterhere");
  

  // récupération du buffer du texte view d'entrée
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(enterhere));

  //interdire l'écriture dans les deux autres texte views
  gtk_text_view_set_editable(GTK_TEXT_VIEW(txtview), FALSE);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(corrected), FALSE);
 

  // permettre le saut de ligne dans les textes view
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(enterhere), GTK_WRAP_CHAR);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txtview), GTK_WRAP_CHAR);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(corrected), GTK_WRAP_CHAR);


  // connection avec les fonctions correspondantes, a chaque fois le texte view de l'entrée est changé
  g_signal_connect(buffer, "changed", G_CALLBACK(on_entry_changed_translate), txtview); 
  g_signal_connect(buffer, "changed", G_CALLBACK(on_entry_changed_correct), corrected);



  // importation des combo boxes qui vont contenir les langues disponibles
  GtkComboBox *combosrc = GTK_COMBO_BOX(gtk_builder_get_object(builder, "combosrc"));
  GtkComboBox *combodest = GTK_COMBO_BOX(gtk_builder_get_object(builder, "combodest"));


 // ajout des langues disponibles au combos boxes
  ajouter_langues_a_combobox(combosrc);
  ajouter_langues_a_combobox(combodest);
  
  // mettre par défaut la traduction de l'anglais vers le français
  gtk_combo_box_set_active(combosrc, 4);
  gtk_combo_box_set_active(combodest, 29);

  // connection avec les fonctions correspondantes, a chaque fois les combo boxes des langues sont changées
  g_signal_connect(combosrc, "changed", G_CALLBACK(on_combosrc_changed), NULL);
  g_signal_connect(combodest, "changed", G_CALLBACK(on_combodest_changed), NULL);
    

  // importation du stack qui va contenir deux page, la première pour la traduction et la deuxième pour l'historique
  GtkStack *stack = GTK_STACK(gtk_builder_get_object(builder, "stack"));

  //titre de la page de l'historique
  GtkWidget *label = gtk_label_new("Historique de la traduction");




    // Créer un conteneur GtkScrolledWindow pour l'historique
    GtkWidget *label_scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(label_scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(label_scrolled_window), label);





    // Créer un conteneur GtkBox pour le GtkTextView
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20); // Vertical box


    //créer autant de lables qu'on a de texte a affichier dans l'historique
    GtkWidget* labels[20];

    // création et setup d'une grid
    GtkWidget *grid = gtk_grid_new();


    gtk_grid_set_column_spacing(GTK_GRID(grid), 90);


    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);

   

    gtk_widget_set_size_request (grid, 20, 40);


    GtkWidget* labCorrected = gtk_label_new("Corrected text");
    GtkWidget* labTranslated = gtk_label_new("Translated text");

    gtk_widget_set_name(labCorrected, "labCorrected");
    gtk_widget_set_name(labTranslated, "labTranslated");

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    int i=2;


// Dans cette partie on ouvre le fichier recovery (sinon on le crée s'il n'existe pas) et on affiche son contenu comme historique
    FILE *file = fopen("recovery.txt", "r");
    if (file != NULL) {

        if(((read = getline(&line, &len, file)) != -1 && len!=0)){

                    labels[0] = gtk_label_new(line);
                    
                
                    gtk_grid_attach(GTK_GRID(grid), labels[0], 0, 1, 1, 1);

                    gtk_label_set_wrap(GTK_LABEL(labels[0]), TRUE);
                    gtk_label_set_justify(GTK_LABEL(labels[0]), GTK_JUSTIFY_LEFT);
                    gtk_widget_set_size_request(labels[0], 200, 50);

                    read = getline(&line, &len, file);
                    
                    labels[1] = gtk_label_new(line);
           

                    gtk_grid_attach(GTK_GRID(grid), labels[1], 1, 1, 1, 1);

                    gtk_label_set_wrap(GTK_LABEL(labels[1]), TRUE);
                    gtk_label_set_justify(GTK_LABEL(labels[1]), GTK_JUSTIFY_LEFT);
                    gtk_widget_set_size_request(labels[1], 200, 50);
            
        


            // Lire chaque ligne du fichier
            while ((read = getline(&line, &len, file)) != -1 && len!=0) {


                    labels[i] = gtk_label_new(line);
                    
                    gtk_grid_attach_next_to(GTK_GRID(grid), labels[i], labels[i-2], GTK_POS_TOP, 1, 1);


                    gtk_label_set_wrap(GTK_LABEL(labels[i]), TRUE);
                    gtk_label_set_justify(GTK_LABEL(labels[i]), GTK_JUSTIFY_LEFT);
                    gtk_widget_set_size_request(labels[i], 100, 50);
                    i++;


            }


            gtk_grid_attach_next_to(GTK_GRID(grid), labCorrected, labels[i-2], GTK_POS_TOP, 1, 1);
            gtk_grid_attach_next_to(GTK_GRID(grid), labTranslated, labels[i-1], GTK_POS_TOP, 1, 1);

            // Libérer la mémoire allouée pour la ligne
            free(line);
            // Fermer le fichier
            fclose(file);

        }
     } else {
         // on crée le fichier s'il n'existe pas        
         file= fopen("recovery.txt", "w");

     }
    



    gtk_box_append(GTK_BOX(box), grid);
    

    GtkWidget *box_scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(box_scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(box_scrolled_window), box);

    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(box_scrolled_window),  800);





    // Créer un conteneur GtkBox pour les deux conteneurs
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); // Vertical box
    gtk_box_append(GTK_BOX(main_box), label_scrolled_window);
    gtk_box_append(GTK_BOX(main_box), box_scrolled_window);

    gtk_stack_add_titled(stack, main_box, "page2", "Historique");



    // call back de l'évènement de fin de programme (quitter l'application)
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_closed), NULL);



    //affichage de la fenetre principale
    gtk_widget_show(GTK_WIDGET (window));

    // libération du builder
    g_object_unref (builder);
}



int main (int   argc, char *argv[]){
#ifdef GTK_SRCDIR
  g_chdir (GTK_SRCDIR);
#endif

  GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}

