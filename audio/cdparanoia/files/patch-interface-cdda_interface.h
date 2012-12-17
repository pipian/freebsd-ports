--- interface/cdda_interface.h.orig	Sat Mar 24 02:15:46 2001
+++ interface/cdda_interface.h	Thu Jan  5 22:27:11 2006
@@ -84,7 +84,7 @@
   int is_atapi;
   int is_mmc;
 
-  cdda_private_data_t *private;
+  cdda_private_data_t *private_data;
   void         *reserved;
   unsigned char inqbytes[4];
 
