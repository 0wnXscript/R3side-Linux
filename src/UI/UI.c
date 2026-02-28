#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UI.h"
#include "Injector.h"

static GtkWidget *main_window;
static GtkWidget *text_view;
static GtkTextBuffer *text_buffer;

static void on_execute_clicked(GtkButton *button, gpointer user_data)
{
    GtkTextIter start, end;

    gtk_text_buffer_get_start_iter(text_buffer, &start);
    gtk_text_buffer_get_end_iter(text_buffer, &end);

    gchar *text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);

    g_print("Editor content:\n%s\n", text);
    g_free(text);
}

static void on_clear_clicked(GtkButton *button, gpointer user_data)
{
    gtk_text_buffer_set_text(text_buffer, "", -1);
}

static void on_attach_clicked(GtkButton *button, gpointer user_data)
{
    const char *target_name = "sober";
    const char *lib_path    = "./Lib.so";

    pid_t pid = find_process_by_name(target_name);
    if (pid <= 0) {
        g_printerr("No process found matching '%s'\n", target_name);
        return;
    }

    printf("[+] Injecting into PID %d...\n", (int)pid);

    if (injector_attach(pid, lib_path) == 0) {
        g_print("[+] Injection succeeded\n");
    } else {
        g_printerr("[-] Injection failed\n");
    }
}

void create_main_window(GtkApplication *app)
{
    main_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(main_window), "R3side");

    gtk_window_set_resizable(GTK_WINDOW(main_window), FALSE);

    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 500);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_window_set_child(GTK_WINDOW(main_window), vbox);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    text_view = gtk_text_view_new();
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), text_view);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(vbox), button_box);

    GtkWidget *btn_execute = gtk_button_new_with_label("Execute");
    GtkWidget *btn_clear   = gtk_button_new_with_label("Clear");
    GtkWidget *btn_attach  = gtk_button_new_with_label("Attach");

    gtk_widget_set_size_request(btn_execute, 120, 40);
    gtk_widget_set_size_request(btn_clear,   120, 40);
    gtk_widget_set_size_request(btn_attach,  120, 40);

    gtk_box_append(GTK_BOX(button_box), btn_execute);
    gtk_box_append(GTK_BOX(button_box), btn_clear);
    gtk_box_append(GTK_BOX(button_box), btn_attach);

    g_signal_connect(btn_execute, "clicked",
                     G_CALLBACK(on_execute_clicked), NULL);

    g_signal_connect(btn_clear, "clicked",
                     G_CALLBACK(on_clear_clicked), NULL);

    g_signal_connect(btn_attach, "clicked",
                     G_CALLBACK(on_attach_clicked), NULL);

    gtk_window_present(GTK_WINDOW(main_window));
}