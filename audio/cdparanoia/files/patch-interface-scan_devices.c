--- interface/scan_devices.c.orig	Mon Mar 26 07:44:01 2001
+++ interface/scan_devices.c	Thu Jan  5 22:27:44 2006
@@ -1,6 +1,8 @@
 /******************************************************************
  * CopyPolicy: GNU Public License 2 applies
  * Copyright (C) 1998-2008 Monty xiphmont@mit.edu
+ * FreeBSD porting (c) 2003
+ * 	Simon 'corecode' Schubert <corecode@corecode.ath.cx>
  * 
  * Autoscan for or verify presence of a cdrom device
  * 
@@ -24,6 +24,8 @@
 
 #define MAX_DEV_LEN 20 /* Safe because strings only come from below */
 /* must be absolute paths! */
+
+#if defined(__linux__)
 static char *scsi_cdrom_prefixes[]={
   "/dev/scd",
   "/dev/sr",
@@ -52,6 +54,17 @@
   "/dev/cm206cd",
   "/dev/gscd",
   "/dev/optcd",NULL};
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+static char *cdrom_devices[] = {
+	"/dev/cd?",
+	"/dev/acd?",
+	"/dev/wcd?",
+	"/dev/mcd?",
+	"/dev/cd?c",
+	"/dev/acd?c",
+	"/dev/wcd?c",
+	"/dev/mcd?c", NULL};
+#endif
 
 /* Functions here look for a cdrom drive; full init of a drive type
    happens in interface.c */
@@ -78,10 +91,12 @@
 	if((d=cdda_identify(buffer,messagedest,messages)))
 	  return(d);
 	idmessage(messagedest,messages,"",NULL);
+#if defined(__linux__)
 	buffer[pos-(cdrom_devices[i])]=j+97;
 	if((d=cdda_identify(buffer,messagedest,messages)))
 	  return(d);
 	idmessage(messagedest,messages,"",NULL);
+#endif
       }
     }else{
       /* Name.  Go for it. */
@@ -145,6 +160,7 @@
 
 }
 
+#if defined(__linux__)
 cdrom_drive *cdda_identify_cooked(const char *dev, int messagedest,
 				  char **messages){
 
@@ -179,6 +179,12 @@
   case IDE1_MAJOR:
   case IDE2_MAJOR:
   case IDE3_MAJOR:
+  case IDE4_MAJOR:
+  case IDE5_MAJOR:
+  case IDE6_MAJOR:
+  case IDE7_MAJOR:
+  case IDE8_MAJOR:
+  case IDE9_MAJOR:
     /* Yay, ATAPI... */
     /* Ping for CDROM-ness */
     
@@ -270,11 +276,11 @@
   d->interface=COOKED_IOCTL;
   d->bigendianp=-1; /* We don't know yet... */
   d->nsectors=-1;
-  d->private=calloc(1,sizeof(*d->private));
+  d->private_data=calloc(1,sizeof(*d->private_data));
   {
     /* goddamnit */
     struct timespec tv;
-    d->private->clock=(clock_gettime(CLOCK_MONOTONIC,&tv)<0?CLOCK_REALTIME:CLOCK_MONOTONIC);
+    d->private_data->clock=(clock_gettime(CLOCK_MONOTONIC,&tv)<0?CLOCK_REALTIME:CLOCK_MONOTONIC);
   }
   idmessage(messagedest,messages,"\t\tCDROM sensed: %s\n",description);
   return(d);
@@ -280,6 +302,61 @@
   return(d);
 }
 
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+cdrom_drive *
+cdda_identify_cooked(const char *dev, int messagedest, char **messages)
+{
+	cdrom_drive *d;
+	struct stat st;
+
+	if (stat(dev, &st)) {
+		idperror(messagedest, messages, "\t\tCould not stat %s", dev);
+		return NULL;
+	}
+
+	if (!S_ISCHR(st.st_mode)) {
+		idmessage(messagedest, messages, "\t\t%s is no block device", dev);
+		return NULL;
+	}
+
+	if ((d = calloc(1, sizeof(*d))) == NULL) {
+		idperror(messagedest, messages, "\t\tCould not allocate memory", NULL);
+		return NULL;
+	}
+	d->ioctl_fd = -1;
+
+	if ((d->ioctl_fd = open(dev, O_RDONLY)) == -1) {
+		idperror(messagedest, messages, "\t\tCould not open %s", dev);
+		goto cdda_identify_cooked_fail;
+	}
+
+	if (ioctl_ping_cdrom(d->ioctl_fd)) {
+		idmessage(messagedest, messages, "\t\tDevice %s is not a CDROM", dev);
+		goto cdda_identify_cooked_fail;
+	}
+
+	d->cdda_device_name = copystring(dev);
+	d->drive_model = copystring("Generic cooked ioctl CDROM");
+	d->interface = COOKED_IOCTL;
+	d->bigendianp = -1;
+	d->nsectors = -1;
+
+	idmessage(messagedest, messages, "\t\tCDROM sensed: %s\n", d->drive_model);
+
+	return d;
+
+cdda_identify_cooked_fail:
+	if (d != NULL) {
+		if (d->ioctl_fd != -1)
+			close(d->ioctl_fd);
+		free(d);
+	}
+	return NULL;
+}
+#endif
+
+
+#if defined(__linux__)
 struct  sg_id {
   long    l1; /* target | lun << 8 | channel << 16 | low_ino << 24 */
   long    l2; /* Unique id */
@@ -406,6 +483,7 @@
   if(dev!=-1)close(dev);
   return(NULL);
 }
+#endif
 
 void strscat(char *a,char *b,int n){
   int i;
@@ -417,6 +495,7 @@
   strcat(a," ");
 }
 
+#if defined(__linux__)
 /* At this point, we're going to punt compatability before SG2, and
    allow only SG2 and SG3 */
 static int verify_SG_version(cdrom_drive *d,int messagedest,
@@ -680,15 +759,15 @@
   d->bigendianp=-1; /* We don't know yet... */
   d->nsectors=-1;
   d->messagedest = messagedest;
-  d->private=calloc(1,sizeof(*d->private));
+  d->private_data=calloc(1,sizeof(*d->private_data));
   {
     /* goddamnit */
     struct timespec tv;
-    d->private->clock=(clock_gettime(CLOCK_MONOTONIC,&tv)<0?CLOCK_REALTIME:CLOCK_MONOTONIC);
+    d->private_data->clock=(clock_gettime(CLOCK_MONOTONIC,&tv)<0?CLOCK_REALTIME:CLOCK_MONOTONIC);
   }
   if(use_sgio){
     d->interface=SGIO_SCSI;
-    d->private->sg_buffer=(unsigned char *)(d->private->sg_hd=malloc(MAX_BIG_BUFF_SIZE));
+    d->private_data->sg_buffer=(unsigned char *)(d->private_data->sg_hd=malloc(MAX_BIG_BUFF_SIZE));
     g_fd=d->cdda_fd=dup(d->ioctl_fd);
   }else{
     version=verify_SG_version(d,messagedest,messages);
@@ -702,8 +781,8 @@
     }
 
     /* malloc our big buffer for scsi commands */
-    d->private->sg_hd=malloc(MAX_BIG_BUFF_SIZE);
-    d->private->sg_buffer=((unsigned char *)d->private->sg_hd)+SG_OFF;
+    d->private_data->sg_hd=malloc(MAX_BIG_BUFF_SIZE);
+    d->private_data->sg_buffer=((unsigned char *)d->private_data->sg_hd)+SG_OFF;
   }
 
   {
@@ -778,9 +857,9 @@
   if(i_fd!=-1)close(i_fd);
   if(g_fd!=-1)close(g_fd);
   if(d){
-    if(d->private){
-      if(d->private->sg_hd)free(d->private->sg_hd);
-      free(d->private);
+    if(d->private_data){
+      if(d->private_data->sg_hd)free(d->private_data->sg_hd);
+      free(d->private_data);
     }
     free(d);
   }
@@ -786,6 +865,96 @@
   }
   return(NULL);
 }
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+
+cdrom_drive *cdda_identify_scsi(const char *dummy,
+    const char *device,
+    int messagedest,
+    char **messages)
+{
+	char *devname;
+	cdrom_drive *d = NULL;
+
+	if (device == NULL) {
+		idperror(messagedest, messages, "\t\tNo device specified", NULL);
+		return NULL;
+	}
+
+	if ((devname = test_resolve_symlink(device, messagedest, messages)) == NULL)
+		return NULL;
+
+	if ((d = calloc(1, sizeof(*d))) == NULL) {
+		idperror(messagedest, messages, "\t\tCould not allocate memory", NULL);
+		free(devname);
+		return NULL;
+	}
+
+	if ((d->private_data = calloc(1, sizeof(*(d->private_data)))) == NULL) {
+		idperror(messagedest, messages, "\t\tCould not allocate memory", NULL);
+		free(d);
+		free(devname);
+		return NULL;
+	}
+
+	if ((d->private_data->dev = cam_open_device(devname, O_RDWR)) == NULL) {
+		idperror(messagedest, messages, "\t\tCould not open SCSI device: %s", cam_errbuf);
+		goto cdda_identify_scsi_fail;
+	}
+
+	if ((d->private_data->ccb = cam_getccb(d->private_data->dev)) == NULL) {
+		idperror(messagedest, messages, "\t\tCould not allocate ccb", NULL);
+		goto cdda_identify_scsi_fail;
+	}
+
+	if (strncmp(d->private_data->dev->inq_data.vendor, "TOSHIBA", 7) == 0 &&
+	    strncmp(d->private_data->dev->inq_data.product, "CD_ROM", 6) == 0 &&
+	    SID_TYPE(&d->private_data->dev->inq_data) == T_DIRECT) {
+		d->private_data->dev->inq_data.device = T_CDROM;
+		d->private_data->dev->inq_data.dev_qual2 |= 0x80;
+	}
+
+	if (SID_TYPE(&d->private_data->dev->inq_data) != T_CDROM &&
+	    SID_TYPE(&d->private_data->dev->inq_data) != T_WORM) {
+		idmessage(messagedest, messages,
+		    "\t\tDevice is neither a CDROM nor a WORM device\n", NULL);
+		goto cdda_identify_scsi_fail;
+	}
+
+	d->cdda_device_name = copystring(devname);
+	d->ioctl_fd = -1;
+	d->bigendianp = -1;
+	d->nsectors = -1;
+	d->lun = d->private_data->dev->target_lun;
+	d->interface = GENERIC_SCSI;
+
+	if ((d->private_data->sg_buffer = malloc(MAX_BIG_BUFF_SIZE)) == NULL) {
+		idperror(messagedest, messages, "Could not allocate buffer memory", NULL);
+		goto cdda_identify_scsi_fail;
+	}
+
+	if ((d->drive_model = calloc(36,1)) == NULL) {
+	}
+
+	strscat(d->drive_model, d->private_data->dev->inq_data.vendor, SID_VENDOR_SIZE);
+	strscat(d->drive_model, d->private_data->dev->inq_data.product, SID_PRODUCT_SIZE);
+	strscat(d->drive_model, d->private_data->dev->inq_data.revision, SID_REVISION_SIZE);
+
+	idmessage(messagedest, messages, "\nCDROM model sensed: %s", d->drive_model);
+
+	return d;
+
+cdda_identify_scsi_fail:
+	free(devname);
+	if (d) {
+		if (d->private_data->ccb)
+			cam_freeccb(d->private_data->ccb);
+		if (d->private_data->dev)
+			cam_close_device(d->private_data->dev);
+		free(d);
+	}
+	return NULL;
+}
+#endif
 
 #ifdef CDDA_TEST
 
@@ -827,7 +996,7 @@
   d->interface=TEST_INTERFACE;
   d->bigendianp=-1; /* We don't know yet... */
   d->nsectors=-1;
-  d->private=calloc(1,sizeof(*d->private));
+  d->private_data=calloc(1,sizeof(*d->private_data));
   d->drive_model=copystring("File based test interface");
   idmessage(messagedest,messages,"\t\tCDROM sensed: %s\n",d->drive_model);
   
