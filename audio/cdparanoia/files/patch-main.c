--- main.c
+++ main.c
@@ -1365,7 +1365,11 @@
 	  if(err)free(err);
 	  if(mes)free(mes);
 	  if(readbuf==NULL){
-	    if(errno==EBADF || errno==ENOMEDIUM){
+	    if(errno==EBADF
+#ifdef ENOMEDIUM
+	    || errno==ENOMEDIUM
+#endif
+	    ){
 	      report("\nparanoia_read: CDROM drive unavailable, bailing.\n");
 	      exit(1);
 	    }
