#include <gtk/gtk.h>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *custom;
    GtkWidget *align;
    GdkCursor *cur;

    gtk_init(&argc, &argv);

    window = main_window_new();

    gtk_widget_show(window);

    gtk_main ();

    return 0;
}
