--- interface/cooked_interface.c.orig	Wed Apr 19 15:41:04 2000
+++ interface/cooked_interface.c	Fri Nov  7 17:16:03 2003
@@ -1,6 +1,8 @@
 /******************************************************************
  * CopyPolicy: GNU Lesser General Public License 2.1 applies
  * Copyright (C) Monty xiphmont@mit.edu
+ * FreeBSD porting (c) 2003
+ * 	Simon 'corecode' Schubert <corecode@corecode.ath.cx>
  *
  * CDROM code specific to the cooked ioctl interface
  *
@@ -13,13 +13,13 @@
 static int timed_ioctl(cdrom_drive *d, int fd, int command, void *arg){
   struct timespec tv1;
   struct timespec tv2;
-  int ret1=clock_gettime(d->private->clock,&tv1);
+  int ret1=clock_gettime(d->private_data->clock,&tv1);
   int ret2=ioctl(fd, command,arg);
-  int ret3=clock_gettime(d->private->clock,&tv2);
+  int ret3=clock_gettime(d->private_data->clock,&tv2);
   if(ret1<0 || ret3<0){
-    d->private->last_milliseconds=-1;
+    d->private_data->last_milliseconds=-1;
   }else{
-    d->private->last_milliseconds = (tv2.tv_sec-tv1.tv_sec)*1000. + (tv2.tv_nsec-tv1.tv_nsec)/1000000.;
+    d->private_data->last_milliseconds = (tv2.tv_sec-tv1.tv_sec)*1000. + (tv2.tv_nsec-tv1.tv_nsec)/1000000.;
   }
   return ret2;
 }
@@ -24,6 +26,7 @@
   return ret2;
 }
 
+#if defined(__linux__)
 static int cooked_readtoc (cdrom_drive *d){
   int i;
   int tracks;
@@ -157,6 +160,140 @@
   return ret;
 }
 
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+static int
+cooked_readtoc(cdrom_drive *d)
+{
+	int i;
+	struct ioc_toc_header hdr;
+	struct ioc_read_toc_single_entry entry;
+
+	if (ioctl(d->ioctl_fd, CDIOREADTOCHEADER, &hdr) == -1) {
+		int ret;
+
+		if (errno == EPERM) {
+			ret = -102;
+			cderror(d, "102: ");
+		} else {
+			ret = -4;
+			cderror(d, "004: Unable to read table of contents header: ");
+		}
+		cderror(d, strerror(errno));
+		cderror(d, "\n");
+		return ret;
+	}
+
+	entry.address_format = CD_LBA_FORMAT;
+	for (i = hdr.starting_track; i <= hdr.ending_track; ++i) {
+		entry.track = i;
+
+		if (ioctl(d->ioctl_fd, CDIOREADTOCENTRY, &entry) == -1) {
+			cderror(d, "005: Unable to read table of contents entry\n");
+			return -5;
+		}
+
+		d->disc_toc[i - hdr.starting_track].bFlags = entry.entry.control;
+		d->disc_toc[i - hdr.starting_track].bTrack = entry.entry.track;
+		d->disc_toc[i - hdr.starting_track].dwStartSector = be32_to_cpu(entry.entry.addr.lba);
+	}
+
+	entry.track = 0xaa;	/* leadout */
+
+	if (ioctl(d->ioctl_fd, CDIOREADTOCENTRY, &entry) == -1) {
+		cderror(d, "005: Unable to read table of contents entry\n");
+		return -5;
+	}
+
+	d->disc_toc[i - hdr.starting_track].bFlags = entry.entry.control;
+	d->disc_toc[i - hdr.starting_track].bTrack = entry.entry.track;
+	d->disc_toc[i - hdr.starting_track].dwStartSector = be32_to_cpu(entry.entry.addr.lba);
+
+	d->cd_extra = FixupTOC(d, hdr.ending_track - hdr.starting_track + 2);	/* with TOC */
+
+	return hdr.ending_track - hdr.starting_track + 1;
+}
+
+static int
+cooked_setspeed(cdrom_drive *d, int speed)
+{
+#ifdef CDRIOCREADSPEED
+	speed *= 177;
+	return ioctl(d->ioctl_fd, CDRIOCREADSPEED, &speed);
+#else
+	return -1;
+#endif
+}
+
+
+static long
+cooked_read(cdrom_drive *d, void *p, long begin, long sectors)
+{
+	int retry_count = 0;
+#ifndef CDIOCREADAUDIO
+	int bsize = CD_FRAMESIZE_RAW;
+#else
+	struct ioc_read_audio arg;
+
+	if (sectors > d->nsectors)
+		sectors = d->nsectors;
+
+	arg.address_format = CD_LBA_FORMAT;
+	arg.address.lba = begin;
+	arg.buffer = p;
+#endif
+
+#ifndef CDIOCREADAUDIO
+	if (ioctl(d->ioctl_fd, CDRIOCSETBLOCKSIZE, &bsize) == -1)
+		return -7;
+#endif
+	for (;;) {
+#ifndef CDIOCREADAUDIO
+		if (pread(d->ioctl_fd, p, sectors*bsize, begin*bsize) != sectors*bsize) {
+#else
+		arg.nframes = sectors;
+		if (ioctl(d->ioctl_fd, CDIOCREADAUDIO, &arg) == -1) {
+#endif
+			if (!d->error_retry)
+				return -7;
+
+			switch (errno) {
+			case ENOMEM:
+				if (sectors == 1) {
+					cderror(d, "300: Kernel memory error\n");
+					return -300;
+				}
+				/* FALLTHROUGH */
+			default:
+				if (sectors == 1) {
+					if (retry_count > MAX_RETRIES - 1) {
+						char b[256];
+						snprintf(b, sizeof(b),
+						    "010: Unable to access sector %ld; "
+						    "skipping...\n", begin);
+						cderror(d, b);
+						return -10;
+					}
+					break;
+				}
+			}
+
+			if (retry_count > 4 && sectors > 1)
+				sectors = sectors * 3 / 4;
+
+			++retry_count;
+
+			if (retry_count > MAX_RETRIES) {
+				cderror(d, "007: Unknown, unrecoverable error reading data\n");
+				return -7;
+			}
+		} else
+			break;
+	}
+
+	return sectors;
+}
+#endif
+
 /* hook */
 static int Dummy (cdrom_drive *d,int Switch){
   return(0);
@@ -221,6 +358,7 @@
 int cooked_init_drive (cdrom_drive *d){
   int ret;
 
+#if defined(__linux__)
   switch(d->drive_type){
   case MATSUSHITA_CDROM_MAJOR:	/* sbpcd 1 */
   case MATSUSHITA_CDROM2_MAJOR:	/* sbpcd 2 */
@@ -271,6 +409,9 @@
   default:
     d->nsectors=40; 
   }
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+  d->nsectors = 26;		/* FreeBSD only support 64K I/O transfer size */
+#endif
   d->enable_cdda = Dummy;
   d->read_audio = cooked_read;
   d->read_toc = cooked_readtoc;
