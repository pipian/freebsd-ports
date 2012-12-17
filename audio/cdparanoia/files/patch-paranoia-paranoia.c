--- paranoia/paranoia.c
+++ paranoia/paranoia.c
@@ -2439,6 +2439,7 @@
 			    secread))<secread){
 
 	if(thisread<0){
+#ifdef ENOMEDIUM
 	  if(errno==ENOMEDIUM){
 	    /* the one error we bail on immediately */
 	    if(new)free_c_block(new);
@@ -2446,6 +2447,7 @@
 	    if(flags)free(flags);
 	    return NULL;
 	  }
+#endif
 	  thisread=0;
 	}
 
@@ -2687,7 +2689,9 @@
 
 	/* Was the medium removed or the device closed out from
 	   under us? */
+#ifdef ENOMEDIUM
 	if(errno==ENOMEDIUM) return NULL;
+#endif
       
       }
     }
