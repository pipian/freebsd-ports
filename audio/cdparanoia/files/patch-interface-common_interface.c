Index: interface/common_interface.c
===================================================================
RCS file: /home/cvs/cdparanoia/interface/common_interface.c,v
retrieving revision 1.1.1.1
retrieving revision 1.5
diff -u -r1.1.1.1 -r1.5
--- interface/common_interface.c	2003/01/05 09:46:26	1.1.1.1
+++ interface/common_interface.c	2003/01/06 21:39:53	1.5
@@ -13,15 +13,22 @@
 #include "utils.h"
 #include "smallft.h"
 
+#if defined(__linux__)
 #include <linux/hdreg.h>
+#endif
 
 /* Test for presence of a cdrom by pinging with the 'CDROMVOLREAD' ioctl() */
 /* Also test using CDROM_GET_CAPABILITY (if available) as some newer DVDROMs will
    reject CDROMVOLREAD ioctl for god-knows-what reason */
 int ioctl_ping_cdrom(int fd){
+#if defined(__linux__)
   struct cdrom_volctrl volctl;
   if (ioctl(fd, CDROMVOLREAD, &volctl) &&
       ioctl(fd, CDROM_GET_CAPABILITY, NULL)<0)
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+  struct ioc_vol volctl;
+  if (ioctl(fd, CDIOCGETVOL, &volctl))
+#endif
     return(1); /* failure */
 
   return(0);
@@ -29,6 +36,7 @@
 }
 
 
+#if defined(__linux__)
 /* Use the ioctl thingy above ping the cdrom; this will get model info */
 char *atapi_drive_info(int fd){
   /* Work around the fact that the struct grew without warning in
@@ -49,6 +57,7 @@
   free(id);
   return(ret);
 }
+#endif
 
 int data_bigendianp(cdrom_drive *d){
   float lsb_votes=0;
@@ -174,7 +183,9 @@
    knows the leasoud/leadin size. */
 
 int FixupTOC(cdrom_drive *d,int tracks){
+#if defined(__linux__)
   struct cdrom_multisession ms_str;
+#endif
   int j;
   
   /* First off, make sure the 'starting sector' is >=0 */
@@ -211,6 +222,8 @@
   /* For a scsi device, the ioctl must go to the specialized SCSI
      CDROM device, not the generic device. */
 
+  /* XXX */
+#if defined(__linux__)
   if (d->ioctl_fd != -1) {
     int result;
 
@@ -235,6 +248,61 @@
       return 1;
     }
   }
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+  {
+    bzero(&d->private_data->ccb->csio, sizeof(d->private_data->ccb->csio));
+
+    unsigned char cmd[] = {
+      0x43,  /* READ TOC/PMA/ATIP */
+      0x00,  /* LBA format */
+      0x01,  /* Session Info */
+      0x00, 0x00, 0x00, 0x00,
+      0x12, 0x00,  /* Multisession data fits in 12 bytes. */
+      0x00
+    };
+    size_t cmd_len = sizeof(cmd);
+    size_t read_len = 12;
+    size_t write_len = 0;
+    memcpy(d->private_data->ccb->csio.cdb_io.cdb_bytes, cmd, cmd_len);
+
+    cam_fill_csio(&d->private_data->ccb->csio,
+        /* retries */ 0,
+        /* cbfcnp */ NULL,
+        /* flags */ CAM_DEV_QFRZDIS | (write_len ? CAM_DIR_OUT : CAM_DIR_IN),
+        /* tag_action */ MSG_SIMPLE_Q_TAG,
+        /* data_ptr */ d->private_data->sg_buffer,
+        /* dxfer_len */ write_len ? write_len : read_len,
+        /* sense_len */ SSD_FULL_SIZE,
+        /* cdb_len */ cmd_len,
+        /* timeout */ 60000);	/* XXX */
+
+    int result = cam_send_ccb(d->private_data->dev, d->private_data->ccb);
+
+    if (result != 0) return -1;
+
+    unsigned char *bp = d->private_data->sg_buffer + 8;
+    unsigned int lba = ((bp[0] << 24) |
+			(bp[1] << 16) |
+			(bp[2] << 8)  | 
+			(bp[3]));
+    if (lba > 100) {
+
+      /* This is an odd little piece of code --Monty */
+
+      /* believe the multisession offset :-) */
+      /* adjust end of last audio track to be in the first session */
+      for (j = tracks-1; j >= 0; j--) {
+	if (j > 0 && !IS_AUDIO(d,j) && IS_AUDIO(d,j-1)) {
+	  if ((d->disc_toc[j].dwStartSector > lba - 11400) &&
+	      (lba - 11400 > d->disc_toc[j-1].dwStartSector))
+	    d->disc_toc[j].dwStartSector = lba - 11400;
+	  break;
+	}
+      }
+      return 1;
+    }
+  }
+#endif
   return 0;
 }
