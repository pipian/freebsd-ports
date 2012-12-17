--- interface/test_interface.c.orig	2001-03-23 17:15:46.000000000 -0800
+++ interface/test_interface.c	2011-10-17 21:33:00.000000000 -0700
@@ -66,9 +66,9 @@
   if(!fd)fd=fdopen(d->cdda_fd,"r");
 
   if(begin<lastread)
-    d->private->last_milliseconds=20;
+    d->private_data->last_milliseconds=20;
   else
-    d->private->last_milliseconds=sectors;
+    d->private_data->last_milliseconds=sectors;
 
 #ifdef CDDA_TEST_UNDERRUN
   sectors-=1;
