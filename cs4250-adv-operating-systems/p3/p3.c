#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (*p3lib_function)(void);

int main (int argc, char** argv) 
{
     void *library;
     p3lib_function p3func;
     const char *funcname, *error;
  
     if(argc > 1){ 
         funcname = argv[1];
     } else if ((funcname = getenv("AOS_PLUGIN")) == 0) {
	 printf("Error, no function specified in argument or environment!\n");
	 printf("Pass the function to be called on the command line or ");
	 printf("set the AOS_PLUGIN environment variable!\n");
	 exit(1);
     }
         

     library = dlopen("libp3d.so", RTLD_LAZY);
     if (library == NULL)
     {
          fprintf(stderr, "Could not open libp3d.so: %s\n", dlerror());
          exit(1);
     }
  
     dlerror();    /* clear errors */
     p3func = dlsym(library, funcname);
     error = dlerror();
     if (error)
     {
          fprintf(stderr, "Could not find function %s: %s\n", funcname, error);
          exit(1);
     }
  
     (*p3func)();
     dlclose(library);
     return(0);
}
