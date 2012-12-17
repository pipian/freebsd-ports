--- interface/scsi_interface.c.orig	2001-03-23 17:15:46.000000000 -0800
+++ interface/scsi_interface.c	2011-10-17 21:33:00.000000000 -0700
@@ -3,6 +3,8 @@
  * Original interface.c Copyright (C) 1994-1997 
  *            Eissfeldt heiko@colossus.escape.de
  * Current incarnation Copyright (C) 1998-2008 Monty xiphmont@mit.edu
+ * FreeBSD porting (c) 2003
+ * 	Simon 'corecode' Schubert <corecode@corecode.ath.cx>
  * 
  * Generic SCSI interface specific code.
  *
@@ -12,6 +12,11 @@
 #include "common_interface.h"
 #include "utils.h"
 #include <time.h>
+
+#ifndef SG_MAX_SENSE
+#define SG_MAX_SENSE 16
+#endif
+
 static int timed_ioctl(cdrom_drive *d, int fd, int command, void *arg){
   struct timespec tv1;
   struct timespec tv2;
@@ -15,13 +20,13 @@
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
@@ -36,6 +41,7 @@
   int table, reserved, cur, err;
   char buffer[256];
 
+#if defined(__linux__)
   /* SG_SET_RESERVED_SIZE doesn't actually allocate or reserve anything.
    * what it _does_ do is give you an error if you ask for a value
    * larger than q->max_sectors (the length of the device's bio request
@@ -59,6 +65,7 @@
 	  "table entry size: %d bytes\n\t"
 	  "maximum theoretical transfer: %d sectors\n",
 	  table, reserved, table*(reserved/CD_FRAMESIZE_RAW));
+
   cdmessage(d,buffer);
 
   cur=reserved; /* only use one entry for now */
@@ -90,9 +97,19 @@
 
   if(cur==0) exit(1);
 
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+  d->nsectors = 26;            /* FreeBSD only supports 64K I/O transfer size */
+  d->bigbuff = d->nsectors * CD_FRAMESIZE_RAW;
+
+  sprintf(buffer,"\tSetting default read size to %d sectors (%d bytes).\n\n",
+      d->nsectors,d->nsectors*CD_FRAMESIZE_RAW);
+#endif
+
   cdmessage(d,buffer);
 }
 
+
+#if defined(__linux__)
 static void clear_garbage(cdrom_drive *d){
   fd_set fdset;
   struct timeval tv;
@@ -96,7 +111,7 @@
 static void clear_garbage(cdrom_drive *d){
   fd_set fdset;
   struct timeval tv;
-  struct sg_header *sg_hd=d->private->sg_hd;
+  struct sg_header *sg_hd=d->private_data->sg_hd;
   int flag=0;
 
   /* clear out any possibly preexisting garbage */
@@ -123,6 +140,7 @@
     flag=1;
   }
 }
+#endif
 
 static int check_sbp_error(const unsigned char status,
 			   const unsigned char *sbp) {
@@ -142,7 +160,11 @@
     case 1:
       break;
     case 2:
+#ifdef ENOMEDIUM
       errno = ENOMEDIUM;
+#else
+      errno = EIO;
+#endif
       return(TR_NOTREADY);
     case 3: 
       if ((ASC==0x0C) & (ASCQ==0x09)) {
@@ -172,6 +194,7 @@
   return 0;
 }
 
+#ifdef __linux__
 /* process a complete scsi command. */
 static int sg2_handle_scsi_cmd(cdrom_drive *d,
 			       unsigned char *cmd,
@@ -185,7 +208,7 @@
   struct timespec tv2;
   int tret1,tret2;
   int status = 0;
-  struct sg_header *sg_hd=d->private->sg_hd;
+  struct sg_header *sg_hd=d->private_data->sg_hd;
   long writebytes=SG_OFF+cmd_len+in_size;
 
   /* generic scsi device services */
@@ -195,7 +218,7 @@
 
   memset(sg_hd,0,sizeof(sg_hd)); 
   memset(sense_buffer,0,SG_MAX_SENSE); 
-  memcpy(d->private->sg_buffer,cmd,cmd_len+in_size);
+  memcpy(d->private_data->sg_buffer,cmd,cmd_len+in_size);
   sg_hd->twelve_byte = cmd_len == 12;
   sg_hd->result = 0;
   sg_hd->reply_len = SG_OFF + out_size;
@@ -209,7 +232,7 @@
      tell if the command failed.  Scared yet? */
 
   if(bytecheck && out_size>in_size){
-    memset(d->private->sg_buffer+cmd_len+in_size,bytefill,out_size-in_size); 
+    memset(d->private_data->sg_buffer+cmd_len+in_size,bytefill,out_size-in_size);
     /* the size does not remove cmd_len due to the way the kernel
        driver copies buffers */
     writebytes+=(out_size-in_size);
@@ -243,7 +266,7 @@
   }
 
   sigprocmask (SIG_BLOCK, &(d->sigset), NULL );
-  tret1=clock_gettime(d->private->clock,&tv1);  
+  tret1=clock_gettime(d->private_data->clock,&tv1);
   errno=0;
   status = write(d->cdda_fd, sg_hd, writebytes );
 
@@ -289,7 +312,7 @@
     }
   }
 
-  tret2=clock_gettime(d->private->clock,&tv2);  
+  tret2=clock_gettime(d->private_data->clock,&tv2);
   errno=0;
   status = read(d->cdda_fd, sg_hd, SG_OFF + out_size);
   sigprocmask ( SIG_UNBLOCK, &(d->sigset), NULL );
@@ -313,7 +336,7 @@
   if(bytecheck && in_size+cmd_len<out_size){
     long i,flag=0;
     for(i=in_size;i<out_size;i++)
-      if(d->private->sg_buffer[i]!=bytefill){
+      if(d->private_data->sg_buffer[i]!=bytefill){
 	flag=1;
 	break;
       }
@@ -326,9 +349,9 @@
 
   errno=0;
   if(tret1<0 || tret2<0){
-    d->private->last_milliseconds=-1;
+    d->private_data->last_milliseconds=-1;
   }else{
-    d->private->last_milliseconds = (tv2.tv_sec-tv1.tv_sec)*1000 + (tv2.tv_nsec-tv1.tv_nsec)/1000000;
+    d->private_data->last_milliseconds = (tv2.tv_sec-tv1.tv_sec)*1000 + (tv2.tv_nsec-tv1.tv_nsec)/1000000;
   }
   return(0);
 }
@@ -347,7 +370,7 @@
 
   memset(&hdr,0,sizeof(hdr));
   memset(sense,0,sizeof(sense));
-  memcpy(d->private->sg_buffer,cmd+cmd_len,in_size);
+  memcpy(d->private_data->sg_buffer,cmd+cmd_len,in_size);
 
   hdr.cmdp = cmd;
   hdr.cmd_len = cmd_len;
@@ -355,7 +378,7 @@
   hdr.mx_sb_len = SG_MAX_SENSE;
   hdr.timeout = 50000;
   hdr.interface_id = 'S';
-  hdr.dxferp =  d->private->sg_buffer;
+  hdr.dxferp =  d->private_data->sg_buffer;
   hdr.flags = SG_FLAG_DIRECT_IO;  /* direct IO if we can get it */
 
   /* scary buffer fill hack */
@@ -400,7 +423,7 @@
   if(bytecheck && in_size<out_size){
     long i,flag=0;
     for(i=in_size;i<out_size;i++)
-      if(d->private->sg_buffer[i]!=bytefill){
+      if(d->private_data->sg_buffer[i]!=bytefill){
 	flag=1;
 	break;
       }
@@ -412,7 +435,7 @@
   }
 
   /* Can't rely on .duration because we can't be certain kernel has HZ set to something useful */
-  /* d->private->last_milliseconds = hdr.duration; */
+  /* d->private_data->last_milliseconds = hdr.duration; */
 
   errno = 0;
   return 0;
@@ -432,6 +455,102 @@
   return sg2_handle_scsi_cmd(d,cmd,cmd_len,in_size,out_size,bytefill,bytecheck,sense);
 
 }
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+static int handle_scsi_cmd(cdrom_drive *d,
+			   unsigned char *cmd,
+			   unsigned int cmd_len,
+			   unsigned int write_len,
+			   unsigned int read_len,
+			   unsigned char bytefill,
+			   int bytecheck,
+			   unsigned char *sense){
+	int result;
+	struct timespec tv1;
+	struct timespec tv2;
+	int tret1, tret2;
+
+	bzero(&d->private_data->ccb->csio, sizeof(d->private_data->ccb->csio));
+
+	memcpy(d->private_data->ccb->csio.cdb_io.cdb_bytes, cmd, cmd_len);
+	memcpy(d->private_data->sg_buffer, cmd + cmd_len, write_len);
+
+	if (bytecheck && (read_len > write_len))
+		memset(d->private_data->sg_buffer+write_len, bytefill, read_len - write_len);
+
+	cam_fill_csio(&d->private_data->ccb->csio,
+	    /* retries */ 0,
+	    /* cbfcnp */ NULL,
+	    /* flags */ CAM_DEV_QFRZDIS | (write_len ? CAM_DIR_OUT : CAM_DIR_IN),
+	    /* tag_action */ MSG_SIMPLE_Q_TAG,
+	    /* data_ptr */ d->private_data->sg_buffer,
+	    /* dxfer_len */ write_len ? write_len : read_len,
+	    /* sense_len */ SSD_FULL_SIZE,
+	    /* cdb_len */ cmd_len,
+	    /* timeout */ 60000);	/* XXX */
+
+
+	tret1=clock_gettime(d->private_data->clock,&tv1);  
+        result = cam_send_ccb(d->private_data->dev, d->private_data->ccb);
+	tret2=clock_gettime(d->private_data->clock,&tv2);  
+
+	if(tret1<0 || tret2<0){
+	  d->private_data->last_milliseconds=-1;
+	}else{
+	  d->private_data->last_milliseconds = (tv2.tv_sec-tv1.tv_sec)*1000 + (tv2.tv_nsec-tv1.tv_nsec)/1000000;
+	}
+
+	if ((result < 0) ||
+	    (d->private_data->ccb->ccb_h.status & CAM_STATUS_MASK) == 0 /* hack? */)
+		return TR_EREAD;
+
+	if ((d->private_data->ccb->ccb_h.status & CAM_STATUS_MASK) != CAM_REQ_CMP &&
+	    (d->private_data->ccb->ccb_h.status & CAM_STATUS_MASK) != CAM_SCSI_STATUS_ERROR) {
+		fprintf (stderr, "\t\terror returned from SCSI command:\n"
+				 "\t\tccb->ccb_h.status == %d\n", d->private_data->ccb->ccb_h.status);
+		errno = EIO;
+		return TR_UNKNOWN;
+	}
+
+	if (d->private_data->ccb->csio.dxfer_len != (write_len ? write_len : read_len)) {
+		errno = EIO;
+		return TR_EREAD;
+	}
+
+	int errorCode, senseKey, addSenseCode, addSenseCodeQual;
+	scsi_extract_sense( &(d->private_data->ccb->csio.sense_data), &errorCode, &senseKey, &addSenseCode,
+		&addSenseCodeQual );
+	if (errorCode) {
+		switch (senseKey) {
+		case SSD_KEY_NO_SENSE:
+			errno = EIO;
+			return TR_UNKNOWN;
+		case SSD_KEY_RECOVERED_ERROR:
+			break;
+		case SSD_KEY_NOT_READY:
+			errno = EBUSY;
+			return TR_BUSY;
+		case SSD_KEY_MEDIUM_ERROR:
+			errno = EIO;
+			if (addSenseCode == 0x0c &&
+			    addSenseCodeQual == 0x09)
+				return TR_STREAMING;
+			else
+				return TR_MEDIUM;
+		case SSD_KEY_HARDWARE_ERROR:
+			errno = EIO;
+			return TR_FAULT;
+		case SSD_KEY_ILLEGAL_REQUEST:
+			errno = EINVAL;
+			return TR_ILLEGAL;
+		default:
+			errno = EIO;
+			return TR_UNKNOWN;
+		}
+	}
+	return 0;
+}
+#endif
+
 
 static int test_unit_ready(cdrom_drive *d){
   unsigned char sense[SG_MAX_SENSE];
@@ -445,9 +550,9 @@
 
   handle_scsi_cmd(d, cmd, 6, 0, 56, 0,0, sense);
 
-  key = d->private->sg_buffer[2] & 0xf;
-  ASC = d->private->sg_buffer[12];
-  ASCQ = d->private->sg_buffer[13];
+  key = d->private_data->sg_buffer[2] & 0xf;
+  ASC = d->private_data->sg_buffer[12];
+  ASCQ = d->private_data->sg_buffer[13];
   
   if(key == 2 && ASC == 4 && ASCQ == 1) return 0;
   return 1;
@@ -453,6 +558,7 @@
   return 1;
 }
 
+#if defined(__linux__)
 static void reset_scsi(cdrom_drive *d){
   int arg,tries=0;
   d->enable_cdda(d,0);
@@ -471,6 +577,29 @@
   
   d->enable_cdda(d,1);
 }
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+static void reset_scsi(cdrom_drive *d) {
+	d->enable_cdda(d,0);
+
+	d->private_data->ccb->ccb_h.func_code = XPT_RESET_DEV;
+	d->private_data->ccb->ccb_h.timeout = 5000;
+
+	cdmessage(d, "sending SCSI reset... ");
+	if (cam_send_ccb(d->private_data->dev, d->private_data->ccb)) {
+		cdmessage(d, "error sending XPT_RESET_DEV CCB");
+	} else {
+
+		if (((d->private_data->ccb->ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP) ||
+		    ((d->private_data->ccb->ccb_h.status & CAM_STATUS_MASK) == CAM_BDR_SENT))
+			cdmessage(d,"OK\n");
+		else
+			cdmessage(d,"FAILED\n");
+	}
+
+	d->enable_cdda(d,1);
+}
+#endif
+
 
 static int mode_sense_atapi(cdrom_drive *d,int size,int page){ 
   unsigned char sense[SG_MAX_SENSE];
@@ -492,7 +622,7 @@
   if (handle_scsi_cmd (d, cmd, 10, 0, size+4,'\377',1,sense)) return(1);
 
   {
-    unsigned char *b=d->private->sg_buffer;
+    unsigned char *b=d->private_data->sg_buffer;
     if(b[0])return(1); /* Handles only up to 256 bytes */
     if(b[6])return(1); /* Handles only up to 256 bytes */
 
@@ -604,8 +734,8 @@
 static unsigned int get_orig_sectorsize(cdrom_drive *d){
   if(mode_sense(d,12,0x01))return(-1);
 
-  d->orgdens = d->private->sg_buffer[4];
-  return(d->orgsize = ((int)(d->private->sg_buffer[10])<<8)+d->private->sg_buffer[11]);
+  d->orgdens = d->private_data->sg_buffer[4];
+  return(d->orgsize = ((int)(d->private_data->sg_buffer[10])<<8)+d->private_data->sg_buffer[11]);
 }
 
 /* switch CDROM scsi drives to given sector size  */
@@ -664,8 +794,8 @@
     return(-4);
   }
 
-  first=d->private->sg_buffer[2];
-  last=d->private->sg_buffer[3];
+  first=d->private_data->sg_buffer[2];
+  last=d->private_data->sg_buffer[3];
   tracks=last-first+1;
 
   if (last > MAXTRK || first > MAXTRK || last<0 || first<0) {
@@ -683,7 +813,7 @@
       return(-5);
     }
     {
-      scsi_TOC *toc=(scsi_TOC *)(d->private->sg_buffer+4);
+      scsi_TOC *toc=(scsi_TOC *)(d->private_data->sg_buffer+4);
 
       d->disc_toc[i-first].bFlags=toc->bFlags;
       d->disc_toc[i-first].bTrack=i;
@@ -704,7 +834,7 @@
     return(-2);
   }
   {
-    scsi_TOC *toc=(scsi_TOC *)(d->private->sg_buffer+4);
+    scsi_TOC *toc=(scsi_TOC *)(d->private_data->sg_buffer+4);
     
     d->disc_toc[i-first].bFlags=toc->bFlags;
     d->disc_toc[i-first].bTrack=0xAA;
@@ -738,7 +868,7 @@
   }
 
   /* copy to our structure and convert start sector */
-  tracks = d->private->sg_buffer[1];
+  tracks = d->private_data->sg_buffer[1];
   if (tracks > MAXTRK) {
     cderror(d,"003: CDROM reporting illegal number of tracks\n");
     return(-3);
@@ -754,33 +884,33 @@
       return(-5);
     }
     
-    d->disc_toc[i].bFlags = d->private->sg_buffer[10];
+    d->disc_toc[i].bFlags = d->private_data->sg_buffer[10];
     d->disc_toc[i].bTrack = i + 1;
 
     d->disc_toc[i].dwStartSector= d->adjust_ssize * 
-	(((signed char)(d->private->sg_buffer[2])<<24) | 
-	 (d->private->sg_buffer[3]<<16)|
-	 (d->private->sg_buffer[4]<<8)|
-	 (d->private->sg_buffer[5]));
+	(((signed char)(d->private_data->sg_buffer[2])<<24) |
+	 (d->private_data->sg_buffer[3]<<16)|
+	 (d->private_data->sg_buffer[4]<<8)|
+	 (d->private_data->sg_buffer[5]));
   }
 
   d->disc_toc[i].bFlags = 0;
   d->disc_toc[i].bTrack = i + 1;
-  memcpy (&foo, d->private->sg_buffer+2, 4);
-  memcpy (&bar, d->private->sg_buffer+6, 4);
+  memcpy (&foo, d->private_data->sg_buffer+2, 4);
+  memcpy (&bar, d->private_data->sg_buffer+6, 4);
   d->disc_toc[i].dwStartSector = d->adjust_ssize * (be32_to_cpu(foo) +
 						    be32_to_cpu(bar));
 
   d->disc_toc[i].dwStartSector= d->adjust_ssize * 
-    ((((signed char)(d->private->sg_buffer[2])<<24) | 
-      (d->private->sg_buffer[3]<<16)|
-      (d->private->sg_buffer[4]<<8)|
-      (d->private->sg_buffer[5]))+
+    ((((signed char)(d->private_data->sg_buffer[2])<<24) |
+      (d->private_data->sg_buffer[3]<<16)|
+      (d->private_data->sg_buffer[4]<<8)|
+      (d->private_data->sg_buffer[5]))+
      
-     ((((signed char)(d->private->sg_buffer[6])<<24) | 
-       (d->private->sg_buffer[7]<<16)|
-       (d->private->sg_buffer[8]<<8)|
-       (d->private->sg_buffer[9]))));
+     ((((signed char)(d->private_data->sg_buffer[6])<<24) |
+       (d->private_data->sg_buffer[7]<<16)|
+       (d->private_data->sg_buffer[8]<<8)|
+       (d->private_data->sg_buffer[9]))));
 
 
   d->cd_extra = FixupTOC(d,tracks+1);
@@ -817,7 +947,7 @@
   cmd[8] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,10,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -836,7 +966,7 @@
   cmd[9] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -854,7 +984,7 @@
   cmd[8] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,10,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -872,7 +1002,7 @@
   cmd[9] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -890,7 +1020,7 @@
   cmd[8] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,10,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -908,7 +1038,7 @@
   cmd[9] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -922,7 +1052,7 @@
   cmd[8] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -936,7 +1066,7 @@
   cmd[8] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -950,7 +1080,7 @@
   cmd[8] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -964,7 +1094,7 @@
   cmd[8] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -978,7 +1108,7 @@
   cmd[8] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -992,7 +1122,7 @@
   cmd[8] = sectors;
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -1026,7 +1156,7 @@
 
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -1039,7 +1169,7 @@
 
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -1052,7 +1182,7 @@
   
   if((ret=handle_scsi_cmd(d,cmd,12,0,sectors * CD_FRAMESIZE_RAW,'\177',1,sense)))
     return(ret);
-  if(p)memcpy(p,d->private->sg_buffer,sectors*CD_FRAMESIZE_RAW);
+  if(p)memcpy(p,d->private_data->sg_buffer,sectors*CD_FRAMESIZE_RAW);
   return(0);
 }
 
@@ -1080,10 +1210,20 @@
 	sprintf(b,"scsi_read error: sector=%ld length=%ld retry=%d\n",
 		begin,sectors,retry_count);
 	cdmessage(d,b);
+#if defined(__linux__)
 	sprintf(b,"                 Sense key: %x ASC: %x ASCQ: %x\n",
 		(int)(sense[2]&0xf),
 		(int)(sense[12]),
 		(int)(sense[13]));
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+	int errorCode, senseKey, addSenseCode, addSenseCodeQual;
+	scsi_extract_sense( &(d->private_data->ccb->csio.sense_data), &errorCode, &senseKey, &addSenseCode,
+		&addSenseCodeQual );
+	sprintf(b,"                 Sense key: %x ASC: %x ASCQ: %x\n",
+                senseKey,
+                addSenseCode,
+                addSenseCodeQual);
+#endif
 	cdmessage(d,b);
 	sprintf(b,"                 Transport error: %s\n",strerror_tr[err]);
 	cdmessage(d,b);
@@ -1092,10 +1228,19 @@
 
 	fprintf(stderr,"scsi_read error: sector=%ld length=%ld retry=%d\n",
 		begin,sectors,retry_count);
+#if defined(__linux__)
 	fprintf(stderr,"                 Sense key: %x ASC: %x ASCQ: %x\n",
 		(int)(sense[2]&0xf),
 		(int)(sense[12]),
 		(int)(sense[13]));
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+	scsi_extract_sense( &(d->private_data->ccb->csio.sense_data), &errorCode, &senseKey, &addSenseCode,
+		&addSenseCodeQual );
+	fprintf(stderr,"                 Sense key: %x ASC: %x ASCQ: %x\n",
+                senseKey,
+                addSenseCode,
+                addSenseCodeQual);
+#endif
 	fprintf(stderr,"                 Transport error: %s\n",strerror_tr[err]);
 	fprintf(stderr,"                 System error: %s\n",strerror(errno));
       }
@@ -1120,10 +1261,11 @@
 	}
 	sectors--;
 	continue;
+#ifdef ENOMEDIUM
       case ENOMEDIUM:
 	cderror(d,"404: No medium present\n");
 	return(-404);
-
+#endif
       default:
 	if(sectors==1){
 	  if(errno==EIO)
@@ -1275,7 +1417,7 @@
 static int count_2352_bytes(cdrom_drive *d){
   long i;
   for(i=2351;i>=0;i--)
-    if(d->private->sg_buffer[i]!=(unsigned char)'\177')
+    if(d->private_data->sg_buffer[i]!=(unsigned char)'\177')
       return(((i+3)>>2)<<2);
 
   return(0);
@@ -1284,7 +1426,7 @@
 static int verify_nonzero(cdrom_drive *d){
   long i,flag=0;
   for(i=0;i<2352;i++)
-    if(d->private->sg_buffer[i]!=0){
+    if(d->private_data->sg_buffer[i]!=0){
       flag=1;
       break;
     }
@@ -1587,6 +1729,7 @@
   }
 }
 
+#if defined(__linux__)
 static int check_atapi(cdrom_drive *d){
   int atapiret=-1;
   int fd = d->cdda_fd; /* check the device we'll actually be using to read */
@@ -1618,6 +1761,47 @@
   }
 }  
 
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+static int
+check_atapi(cdrom_drive *d)
+{
+	bzero(&(&d->private_data->ccb->ccb_h)[1], sizeof(d->private_data->ccb->cpi) - sizeof(d->private_data->ccb->ccb_h));
+
+	d->private_data->ccb->ccb_h.func_code = XPT_PATH_INQ;
+
+	cdmessage(d, "\nChecking for ATAPICAM...\n");
+
+	if (cam_send_ccb(d->private_data->dev, d->private_data->ccb) < 0) {
+		cderror(d, "\terror sending XPT_PATH_INQ CCB: ");
+		cderror(d, cam_errbuf);
+		cderror(d, "\n");
+		return -1;
+	}
+
+	if ((d->private_data->ccb->ccb_h.status & CAM_STATUS_MASK) != CAM_REQ_CMP) {
+		cderror(d, "\tXPT_PATH_INQ CCB failed: ");
+		cderror(d, cam_errbuf);
+		cderror(d, "\n");
+		return -1;
+	}
+
+	/*
+	 * if the bus device name is `ata', we're (obviously)
+	 * running ATAPICAM.
+	 */
+
+	if (strncmp(d->private_data->ccb->cpi.dev_name, "ata", 3) == 0) {
+		cdmessage(d, "\tDrive is ATAPI (using ATAPICAM)\n");
+		d->is_atapi = 1;
+	} else {
+		cdmessage(d, "\tDrive is SCSI\n");
+		d->is_atapi = 0;
+	}
+
+	return d->is_atapi;
+}
+#endif
+
 static int check_mmc(cdrom_drive *d){
   unsigned char *b;
   cdmessage(d,"\nChecking for MMC style command set...\n");
@@ -1625,7 +1809,7 @@
   d->is_mmc=0;
   if(mode_sense(d,22,0x2A)==0){
   
-    b=d->private->sg_buffer;
+    b=d->private_data->sg_buffer;
     b+=b[3]+4;
     
     if((b[0]&0x3F)==0x2A){
@@ -1664,6 +1848,7 @@
   }
 }
 
+#ifdef __linux__
 /* request vendor brand and model */
 unsigned char *scsi_inquiry(cdrom_drive *d){
   unsigned char sense[SG_MAX_SENSE];
@@ -1673,7 +1858,7 @@
     cderror(d,"008: Unable to identify CDROM model\n");
     return(NULL);
   }
-  return (d->private->sg_buffer);
+  return (d->private_data->sg_buffer);
 }
 
 int scsi_init_drive(cdrom_drive *d){
@@ -1675,6 +1860,7 @@
   }
   return (d->private_data->sg_buffer);
 }
+#endif
 
 int scsi_init_drive(cdrom_drive *d){
   int ret;
@@ -1742,8 +1928,12 @@
   check_cache(d);
 
   d->error_retry=1;
-  d->private->sg_hd=realloc(d->private->sg_hd,d->nsectors*CD_FRAMESIZE_RAW + SG_OFF + 128);
-  d->private->sg_buffer=((unsigned char *)d->private->sg_hd)+SG_OFF;
+#if defined(__linux__)
+  d->private_data->sg_hd=realloc(d->private_data->sg_hd,d->nsectors*CD_FRAMESIZE_RAW + SG_OFF + 128);
+  d->private_data->sg_buffer=((unsigned char *)d->private_data->sg_hd)+SG_OFF;
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+  d->private_data->sg_buffer==realloc(d->private_data->sg_buffer,d->nsectors * CD_FRAMESIZE_RAW + 128);
+#endif
   d->report_all=1;
   return(0);
 }
