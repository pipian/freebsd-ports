--- interface/interface.c.orig	Thu Apr 20 00:41:04 2000
+++ interface/interface.c	Sat Jan  7 14:31:19 2006
@@ -39,9 +39,17 @@
     if(d->drive_model)free(d->drive_model);
     if(d->cdda_fd!=-1)close(d->cdda_fd);
     if(d->ioctl_fd!=-1 && d->ioctl_fd!=d->cdda_fd)close(d->ioctl_fd);
-    if(d->private){
-      if(d->private->sg_hd)free(d->private->sg_hd);
-      free(d->private);
+    if(d->private_data){
+#if defined(__linux__)
+      if(d->private_data->sg_hd)free(d->private_data->sg_hd);
+#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
+      if(d->private_data->dev)
+        cam_close_device(d->private_data->dev);
+      if(d->private_data->ccb)
+        cam_freeccb(d->private_data->ccb);
+      if(d->private_data->sg_buffer)free(d->private_data->sg_buffer);
+#endif
+      free(d->private_data);
     }
 
     free(d);
@@ -118,7 +118,7 @@
 	if(d->bigendianp==-1) /* not determined yet */
 	  d->bigendianp=data_bigendianp(d);
 	
-	if(d->bigendianp!=bigendianp()){
+	if(buffer && d->bigendianp!=bigendianp()){
 	  int i;
 	  u_int16_t *p=(u_int16_t *)buffer;
 	  long els=sectors*CD_FRAMESIZE_RAW/2;
@@ -127,7 +127,7 @@
 	}
       }	
     }
-    if(ms)*ms=d->private->last_milliseconds;
+    if(ms)*ms=d->private_data->last_milliseconds;
     return(sectors);
   }
   
