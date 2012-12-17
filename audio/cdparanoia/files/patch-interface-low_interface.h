Index: interface/low_interface.h
===================================================================
RCS file: /home/cvs/cdparanoia/interface/low_interface.h,v
retrieving revision 1.1.1.1
retrieving revision 1.3
diff -u -r1.1.1.1 -r1.3
--- interface/low_interface.h	2003/01/05 09:46:26	1.1.1.1
+++ interface/low_interface.h	2003/01/06 21:26:23	1.3
@@ -26,6 +26,8 @@
 #include <sys/time.h>
 #include <sys/types.h>
 
+#if defined(__linux__)
+
 #include <linux/major.h>
 #include <linux/version.h>
 
@@ -54,7 +56,6 @@
 #include <linux/cdrom.h>
 #include <linux/major.h>
 
-#include "cdda_interface.h"
 
 #ifndef SG_EMULATED_HOST
 /* old kernel version; the check for the ioctl is still runtime, this
@@ -64,6 +65,19 @@
 #define SG_GET_TRANSFORM 0x2205
 #endif
 
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+
+#include <sys/cdio.h>
+#include <sys/cdrio.h>
+
+#include <cam/scsi/scsi_message.h>
+#include <camlib.h>
+
+#endif
+
+#include "cdda_interface.h"
+
+
 #ifndef SG_IO
 /* old kernel version; the usage is all runtime-safe, this is just to
    build */
@@ -98,19 +112,30 @@
 #endif
 
 struct cdda_private_data {
+#if defined(__linux__)
   struct sg_header *sg_hd;
-  unsigned char *sg_buffer; /* points into sg_hd */
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+  struct cam_device *dev;
+#endif
+  unsigned char *sg_buffer; /* on Linux points into sg_hd */
   clockid_t clock;
   int last_milliseconds;
+#if defined (__FreeBSD__) || defined(__FreeBSD_kernel__)
+  union ccb *ccb;
+#endif
 };
 
 #define MAX_RETRIES 8
 #define MAX_BIG_BUFF_SIZE 65536
 #define MIN_BIG_BUFF_SIZE 4096
-#define SG_OFF sizeof(struct sg_header)
 
 extern int  cooked_init_drive (cdrom_drive *d);
+#if defined(__linux__)
 extern unsigned char *scsi_inquiry (cdrom_drive *d);
+#define SG_OFF sizeof(struct sg_header)
+#else
+#define SG_OFF (0)
+#endif
 extern int  scsi_init_drive (cdrom_drive *d);
 #ifdef CDDA_TEST
 extern int  test_init_drive (cdrom_drive *d);
