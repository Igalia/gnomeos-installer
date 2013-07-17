#include "mainwindow.h"
#include <stdio.h>
#include <stdint.h>
#include <libudev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MARGIN_DEFAULT 20

#define IMAGE_SIZE_FILE "/tmp/image_size"
#define PROGRESS_FILE "/tmp/progress"
#define MOUNT_PATH "/mnt"
#define IMAGE_PATH MOUNT_PATH"/image/image.gz"
#define SOURCE_PARTITION_LABEL "GNOMEOS"

struct _MainWindow {
    GtkWindow parent;
    GtkWidget *header;
    GtkWidget *header_previous;
    GtkWidget *header_next;
    GtkWidget *header_label;
    GtkWidget *footer;
    GtkWidget *footer_label;
    GtkWidget *label1;
    GtkWidget *label2;
    GtkWidget *disks;
    GtkWidget *progress;
    gchar *destination;
    uint64_t image_size;
    gint width;
};

struct _MainWindowClass {
    GtkWindowClass parent;
};

struct hard_disk {
    gchar *device;
    gchar *model;
    uint64_t size;
};

G_DEFINE_TYPE(MainWindow, main_window, GTK_TYPE_WINDOW)

static void create_header(MainWindow *window);
static void create_footer(MainWindow *window);
static void create_step1(MainWindow *window);
static void create_step2(MainWindow *window);
static void create_step3(MainWindow *window);
static void create_step4(MainWindow *window);
static void set_step1(MainWindow *window);
static void set_step2(MainWindow *window);
static void set_step3(MainWindow *window);
static void set_step4(MainWindow *window);
static void finish(MainWindow *window);
static void install(MainWindow *window);
static GList* get_hard_disks();
static void free_hard_disk(gpointer data);
static void mount_source_device();
static void umount_source_device();
static uint64_t get_image_size(MainWindow *window);
static void disk_toggled(GtkToggleButton *toggle, gpointer data);

/*************************************************************************************/
static void main_window_finalize(GObject *gObject)
{
    MainWindow *widget = MAIN_WINDOW(gObject);

    g_free(widget->destination);

    G_OBJECT_CLASS(main_window_parent_class)->finalize(gObject);
}

static void main_window_init(MainWindow *window)
{
    GdkScreen *screen;
    GtkWidget *vbox, *vbox2;
    GtkWidget *align;

    window->image_size = 0;
    window->destination = NULL;

    screen = gtk_window_get_screen(GTK_WINDOW(window));
    gtk_widget_set_size_request(GTK_WIDGET(window),
                                gdk_screen_get_width(screen),
                                gdk_screen_get_height(screen));
    window->width = gdk_screen_get_width(screen);

    vbox = gtk_vbox_new(FALSE, MARGIN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);

    create_header(window);
    gtk_box_pack_start(GTK_BOX(vbox), window->header, FALSE, FALSE, 0);
    gtk_widget_show_all(window->header);

    align = gtk_alignment_new(0.5, 0.5, 0.8, 0);
    gtk_box_pack_start(GTK_BOX(vbox), align, TRUE, TRUE, 0);
    gtk_widget_show(align);

    vbox2 = gtk_vbox_new(FALSE, MARGIN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(align), vbox2);
    gtk_widget_show(vbox2);

    window->label1 = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox2), window->label1, FALSE, FALSE, 0);

    window->label2 = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox2), window->label2, FALSE, FALSE, 0);

    window->disks = gtk_hbox_new(TRUE, MARGIN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(vbox2), window->disks, FALSE, FALSE, 0);

    window->progress = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox2), window->progress, FALSE, FALSE, 0);

    create_footer(window);
    gtk_box_pack_start(GTK_BOX(vbox), window->footer, FALSE, FALSE, 0);
    gtk_widget_show_all(window->footer);

    set_step1(window);
}

static void main_window_realize(GtkWidget* widget)
{
    GdkCursor *cur;

    GTK_WIDGET_CLASS(main_window_parent_class)->realize(widget);

    cur = gdk_cursor_new(GDK_LEFT_PTR);
    gdk_window_set_cursor (widget->window, cur); 
    gdk_cursor_unref(cur);
}

static void main_window_class_init(MainWindowClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    GtkWidgetClass *gtkwidgetClass = GTK_WIDGET_CLASS(klass);

    gobjectClass->finalize = main_window_finalize;
    gtkwidgetClass->realize = main_window_realize;
}

static void create_header(MainWindow *window)
{
    GtkWidget *align;
    GdkColor color;
    GtkWidget *hbox;

    window->header = gtk_event_box_new();
    gtk_widget_set_size_request(window->header, 0, 50);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(window->header), TRUE);
    gdk_color_parse("#888888", &color);
    gtk_widget_modify_bg(window->header, GTK_STATE_NORMAL, &color);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window->header), hbox);

    align = gtk_alignment_new(0.5, 0.5, 1, 0);
    gtk_widget_set_size_request(align, 100, 30);
    gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, FALSE, MARGIN_DEFAULT);
    window->header_previous = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(align), window->header_previous);

    align = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), align, TRUE, FALSE, MARGIN_DEFAULT);
    window->header_label = gtk_label_new("");
    gtk_container_add(GTK_CONTAINER(align), window->header_label);

    align = gtk_alignment_new(0.5, 0.5, 1, 0);
    gtk_widget_set_size_request(align, 100, 30);
    gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, FALSE, MARGIN_DEFAULT);
    window->header_next = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(align), window->header_next);

    gtk_widget_show_all(window->header);
}

static void create_footer(MainWindow *window)
{
    window->footer = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_widget_set_size_request(window->footer, 0, 50);

    window->footer_label = gtk_label_new("");
    gtk_container_add(GTK_CONTAINER(window->footer), window->footer_label);

    gtk_widget_show_all(window->footer);
}

static void set_step1(MainWindow *window)
{
    gtk_widget_hide(window->header_previous);
    g_signal_handlers_disconnect_by_data(G_OBJECT(window->header_previous), window);

    gtk_label_set_text(GTK_LABEL(window->header_label), "GnomeOS Installer");

    gtk_button_set_label(GTK_BUTTON(window->header_next), "Start");
    gtk_widget_show(window->header_next);
    g_signal_handlers_disconnect_by_data(G_OBJECT(window->header_next), window);
    g_signal_connect_swapped(G_OBJECT(window->header_next), "clicked", G_CALLBACK(set_step2), window);

    gtk_label_set_text(GTK_LABEL(window->label1), "Welcome to the GnomeOS installer");
    gtk_widget_show(window->label1);
    gtk_label_set_text(GTK_LABEL(window->label2), "Press start to begin with the installation");
    gtk_widget_show(window->label2);
    gtk_widget_hide(window->disks);
    gtk_widget_hide(window->progress);

    gtk_label_set_text(GTK_LABEL(window->footer_label), "1/4");
 }

static void set_step2(MainWindow *window)
{
    gtk_widget_hide(window->header_previous);
    g_signal_handlers_disconnect_by_data(G_OBJECT(window->header_previous), window);

    gtk_label_set_text(GTK_LABEL(window->header_label), "Installation size");

    gtk_widget_hide(window->header_next);
    g_signal_handlers_disconnect_by_data(G_OBJECT(window->header_next), window);

    gtk_label_set_text(GTK_LABEL(window->label1), "Calculating space needed for the installation");
    gtk_widget_show(window->label1);
    gtk_widget_hide(window->label2);
    gtk_widget_hide(window->disks);
    gtk_widget_show(window->progress);

    gtk_label_set_text(GTK_LABEL(window->footer_label), "2/4");

    mount_source_device();
    window->image_size = get_image_size(window);
    set_step3(window);
 }

static void set_step3(MainWindow *window)
{
    int i;
    GList *devices;
    GtkWidget *radio;
    gboolean limit_size = FALSE;

    // create an invisible radio button to hold th button group
    radio = gtk_radio_button_new(NULL);
    gtk_box_pack_start(GTK_BOX(window->disks), radio, FALSE, FALSE, 0);

    devices = get_hard_disks();

    // don't let the buttons grow too much
    if (((window->width * 0.8) / g_list_length(devices)) > 200)
        limit_size = TRUE;

    for (i = 0; i < g_list_length(devices); i++) {
        struct hard_disk *disk;
        GtkWidget *b, *l, *box, *align;
        gchar *label;

        disk = (struct hard_disk*)g_list_nth_data(devices, i);
        b = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(radio));
        align = gtk_alignment_new(0.5, 0.5, 0, 0);
        gtk_container_add(GTK_CONTAINER(b), align);
        box = gtk_vbox_new(FALSE, 4);
        gtk_container_add(GTK_CONTAINER(align), box);
        l = gtk_label_new(disk->model);
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        l = gtk_label_new(disk->device);
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        label = g_strdup_printf("%llu GB", disk->size >> 20);
        l = gtk_label_new(label);
        gtk_box_pack_start(GTK_BOX(box), l, FALSE, FALSE, 0);
        g_free(label);

        gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(b), FALSE);
        gtk_widget_set_name(b, disk->device);
        gtk_widget_show_all(b);
        g_signal_connect(b, "toggled", G_CALLBACK(disk_toggled), window);
        if (limit_size) {
            gtk_widget_set_size_request(b, 150, 150);
            gtk_box_pack_start(GTK_BOX(window->disks), b, FALSE, FALSE, 0);
        } else {
            gtk_widget_set_size_request(b, 0, 150);
            gtk_box_pack_start(GTK_BOX(window->disks), b, TRUE, TRUE, 0);
        }
        // disable disks with not enough space
        if (disk->size < (window->image_size+1))
            gtk_widget_set_sensitive(b, FALSE);
    }
    g_list_free_full(devices, free_hard_disk);

    gtk_widget_hide(window->header_previous);
    g_signal_handlers_disconnect_by_data(G_OBJECT(window->header_previous), window);

    gtk_label_set_text(GTK_LABEL(window->header_label), "Drive select");

    gtk_button_set_label(GTK_BUTTON(window->header_next), "Install");
    gtk_widget_show(window->header_next);
    gtk_widget_set_sensitive(window->header_next, FALSE);
    g_signal_handlers_disconnect_by_data(G_OBJECT(window->header_next), window);
    g_signal_connect_swapped(G_OBJECT(window->header_next), "clicked", G_CALLBACK(set_step4), window);

    gtk_label_set_text(GTK_LABEL(window->label1), "Select the destination hard disk");
    gtk_widget_show(window->label1);
    gtk_widget_hide(window->label2);
    gtk_widget_show(window->disks);
    gtk_widget_hide(window->progress);

    gtk_label_set_text(GTK_LABEL(window->footer_label), "3/4");
}

static void set_step4(MainWindow *window)
{
    gtk_widget_hide(window->header_previous);
    g_signal_handlers_disconnect_by_data(G_OBJECT(window->header_previous), window);

    gtk_label_set_text(GTK_LABEL(window->header_label), "Installing");

    gtk_button_set_label(GTK_BUTTON(window->header_next), "Reboot");
    gtk_widget_hide(window->header_next);
    g_signal_handlers_disconnect_by_data(G_OBJECT(window->header_next), window);
    g_signal_connect_swapped(G_OBJECT(window->header_next), "clicked", G_CALLBACK(finish), window);

    gtk_label_set_text(GTK_LABEL(window->label1), "Copying files");
    gtk_widget_show(window->label1);
    gtk_widget_hide(window->label2);
    gtk_widget_hide(window->disks);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(window->progress), 0);
    gtk_widget_show(window->progress);

    gtk_label_set_text(GTK_LABEL(window->footer_label), "4/4");

    while (gtk_events_pending ())
        gtk_main_iteration ();

    install(window);
 }

static void finish(MainWindow *window)
{
    umount_source_device();
    system("/sbin/reboot");
}

static void install(MainWindow *window)
{
    gchar *command;
    gchar buf[256];
    gchar **split;
    uint64_t kbytes;
    FILE *f = NULL;

    command = g_strdup_printf("rm -f %s", PROGRESS_FILE);
    system(command);
    g_free(command);
    command = g_strdup_printf("zcat %s | dd of=%s bs=4M conv=noerror oflag=dsync 2> %s &",
                              IMAGE_PATH, window->destination, PROGRESS_FILE);
    system(command);
    g_free(command);
    command = g_strdup_printf("kill -USR1 $(pidof dd)");
    system(command);
    while ((f = fopen(PROGRESS_FILE, "r")) == NULL) {
        sleep(2);
    }

    kbytes = 0;

    // we need to get sure that we read 3 lines each time or the parse will start
    // to fail, so we use whiles until we get each line
    while (1) {
        //ensure we read the first line
        while (fgets(buf, 255, f) == NULL) {
            sleep(1);
        }

        //ensure we read the the second line
        while (fgets(buf, 255, f) == NULL) {
            sleep(1);
        }

        //ensure we read the third line
        while (fgets(buf, 255, f) == NULL) {
            sleep(1);
        }
        //split the line to get the 1st substring with the bytes
        split = g_strsplit(buf, " ", 2);
        sscanf(split[0], "%llu", &kbytes);
        kbytes = kbytes >> 10;
        g_strfreev(split);

        if (kbytes < window->image_size) {
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(window->progress), ((kbytes*1.0)/window->image_size));
            while (gtk_events_pending ())
                gtk_main_iteration ();
            system(command);
        } else {
            sleep(2);
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(window->progress), 1.0);
            gtk_label_set_text(GTK_LABEL(window->label1), "Installation finished. Press reboot to restart the system");
            break;
        }
        sleep(2);
    }

    g_free(command);
    fclose(f);
    gtk_widget_show(window->header_next);
}

static GList* get_hard_disks()
{
    int i;
    GList *devices = NULL;
    struct udev *udev;
    struct udev_enumerate *udev_enum;
    struct udev_list_entry *udev_le_first, *udev_le;
    struct udev_device *udev_dev;
    const char *path;
    struct hard_disk *disk;

    udev = udev_new();
    udev_enum = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(udev_enum, "block");
    udev_enumerate_add_match_property(udev_enum, "ID_TYPE", "disk");
    udev_enumerate_add_match_sysattr(udev_enum, "removable", "0");
    udev_enumerate_scan_devices(udev_enum);

    udev_le_first = udev_enumerate_get_list_entry(udev_enum);
    udev_list_entry_foreach(udev_le, udev_le_first) {
        path = udev_list_entry_get_name(udev_le);
        udev_dev = udev_device_new_from_syspath(udev, path);

        disk = g_malloc(sizeof(struct hard_disk));
        disk->model = g_strdup(udev_device_get_property_value(udev_dev, "ID_MODEL"));
        disk->device = g_strdup(udev_device_get_property_value(udev_dev, "DEVNAME"));
        // convert size from 512 bytes blocks to kbytes
        disk->size = atoi(udev_device_get_sysattr_value(udev_dev, "size")) >> 1;
        devices = g_list_append(devices, disk);
    }

    udev_enumerate_unref(udev_enum);
    udev_unref(udev);

    return devices;
}

static void free_hard_disk(gpointer data)
{
    struct hard_disk *disk = (struct hard_disk*)data;
    g_free(disk->device);
    g_free(disk->model);
    g_free(disk);
}

static void mount_source_device()
{
    int i;
    struct udev *udev;
    struct udev_enumerate *udev_enum;
    struct udev_list_entry *udev_le;
    struct udev_device *udev_dev;
    const char *path;
    gchar *device = NULL;
    gchar *command;

    udev = udev_new();
    udev_enum = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(udev_enum, "block");
    udev_enumerate_add_match_property(udev_enum, "ID_FS_LABEL", SOURCE_PARTITION_LABEL);
    udev_enumerate_add_match_sysattr(udev_enum, "partition", "1");
    udev_enumerate_scan_devices(udev_enum);

    // We get the first partition only. If there are no partitions,
    // fall back to the cdrom device so it works in VirtualBox
    udev_le = udev_enumerate_get_list_entry(udev_enum);

    if (udev_le) {
        path = udev_list_entry_get_name(udev_le);
        udev_dev = udev_device_new_from_syspath(udev, path);
        device = g_strdup(udev_device_get_property_value(udev_dev, "DEVNAME"));
        udev_device_unref(udev_dev);
    } else {
        device = g_strdup("/dev/sr0");
    }

    udev_enumerate_unref(udev_enum);
    udev_unref(udev);

    command = g_strdup_printf("mount -t iso9660 %s %s", device, MOUNT_PATH);
    system(command);
    g_free(command);
    g_free(device);
}

static void umount_source_device()
{
    gchar *command;

    command = g_strdup_printf("umount %s", MOUNT_PATH);
    system(command);
    g_free(command);
}

static uint64_t get_image_size(MainWindow *window)
{
    FILE *f;
    uint64_t size;
    gchar *command;
    struct stat buf;
    int fsize;

    
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(window->progress));
    command = g_strdup_printf("rm -f %s", IMAGE_SIZE_FILE);
    g_free(command);
    command = g_strdup_printf("zcat %s | wc -c > %s &", IMAGE_PATH, IMAGE_SIZE_FILE);
    system(command);
    g_free(command);

    do {
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(window->progress));
        while (gtk_events_pending ())
            gtk_main_iteration ();
        sleep(1);
        stat(IMAGE_SIZE_FILE, &buf);
    } while (buf.st_size == 0);

    f = fopen(IMAGE_SIZE_FILE, "r");
    fscanf(f, "%llu", &size);
    fclose(f);

    // size to kb
    size = size >> 10;
    return size;
}

static void disk_toggled(GtkToggleButton *toggle, gpointer data)
{
    MainWindow *window = (MainWindow*)data;

    if (gtk_toggle_button_get_active(toggle)) {
        g_free(window->destination);
        window->destination = g_strdup(gtk_widget_get_name(GTK_WIDGET(toggle)));
        gtk_widget_set_sensitive(window->header_next, TRUE);
    }
}

/************************ Public API ************************************/
GtkWidget *main_window_new()
{
    return GTK_WIDGET(g_object_new(MAIN_TYPE_WINDOW, NULL));
}
