// gcc -o main app.c `pkg-config --cflags --libs gtk4`
// gtk4-builder-tool simplify --3to4 --replace ./myapp.ui


#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ncurses.h>
#include<pthread.h>

#define API_KEY "AIzaSyBGfF7FmIbaQ-N44u3rn3pAqdTH1WI3nM4"
signed int delimitationIndex = 0;
char response[2048];

static void
print_hello (GtkWidget *widget,
             gpointer   data)
{


   g_print("Hello World\n");

   GtkWidget *txtview = GTK_WIDGET(data);

   
   GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtview));

   gtk_text_buffer_set_text(buffer, "eee", -1);
   
}

static void
quit_cb (GtkWindow *window)
{
  gtk_window_close (window);
}


void translate_word(const char *word, char* translation) {

    
    int pipefd[2];
    char response[2048]; // Augmentez la taille si nécessaire
  //  char translation[2048];

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
      //  g_print("%s ", translation); 


        // Attendre la fin du processus enfant
        // la terminaison du fils me semble correct ici
        wait(NULL);

    }
}

void on_entry_changed(GtkEntry *entry, gpointer data) {

   


    char destination[2048];
    
    char translation[2048];


    GtkWidget *txtview = GTK_WIDGET(data);
    GtkTextBuffer *buffertxt = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txtview));
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(entry);
    const char* text = gtk_entry_buffer_get_text(buffer);
    guint len = gtk_entry_buffer_get_length(buffer);

    //strcpy(response, "");


    if(text[len-1] == ','){

        strcpy(destination, text+delimitationIndex);
        translate_word(destination, translation);
        strcat(response, translation);
        delimitationIndex = len;
    }

    int find = 0;

    if(delimitationIndex>len-1){ // chercher le dernier délimitateur
        for(int i=len-1; i>=0; i--){
            if(text[i] == ','){
         
                strcpy(destination, text+i+1);
                translate_word(destination, translation);
                strcat(response, translation);
               // g_print("%s\n", response);
                find = 1;
                break;
            }
        }
        if(!find){ // pas de delimitateur
                translate_word(text, response);
        }   
    }

    //strcpy(destination, text+delimitationIndex+1);
   // translate_word(destination, response);

    g_print("%s\n", response);

    


    //g_print("%s\n",destination);

}   


static void
activate (GtkApplication *app,
          gpointer        user_data)
{
    strcpy(response, "");
  
  /* Construct a GtkBuilder instance and load our UI description */
  GtkBuilder *builder = gtk_builder_new();
  gtk_builder_add_from_file (builder, "myapp.ui", NULL);


  /* Connect signal handlers to the constructed widgets. */
  GObject *window = gtk_builder_get_object (builder, "window");
  gtk_window_set_application (GTK_WINDOW (window), app);

  GObject *button = gtk_builder_get_object(builder, "button");
    


  GObject *entry = gtk_builder_get_object(builder, "entry");

  GObject *txtview = gtk_builder_get_object(builder, "txtview");

   

  g_signal_connect(button, "clicked", G_CALLBACK(print_hello), txtview);

   //  gtk_text_buffer_set_text(buffer, "ddd", -1);

  g_signal_connect(entry, "changed", G_CALLBACK(on_entry_changed), txtview);


    


 // GObject *button = gtk_builder_get_object (builder, "button1");

 /* GObject *button = gtk_builder_get_object (builder, "button1");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  button = gtk_builder_get_object (builder, "button2");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  button = gtk_builder_get_object (builder, "quit");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (quit_cb), window);




*/
  gtk_widget_show(GTK_WIDGET (window));

  /* We do not need the builder any more */
  g_object_unref (builder);
}

int
main (int   argc,
      char *argv[])
{
#ifdef GTK_SRCDIR
  g_chdir (GTK_SRCDIR);
#endif

  GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
