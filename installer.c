#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

char version[10];

int main (argc,argv)
char argc, *argv[];
{
	int x, cc;
	char pwd[80], src_path[255], dest_path[255], destination[80];
	FILE *infile, *outfile;

	cc=0;
	getcwd(pwd,79);

	infile=fopen(".version","r");
	fscanf(infile,"%s",version);
	fclose(infile);

	printf("flyby appears to be installed under %s/\n",pwd);

	/* Write flyby.h */

	outfile=fopen("flyby.h","w");

	fprintf(outfile,"/* This file was generated by the installer program */\n\n");
	fprintf(outfile,"char *flybypath={\"%s/\"}, ",pwd);
	fprintf(outfile, "soundcard=0,");
	fprintf(outfile, " *version={\"%s\"};\n",version);
	fclose(outfile);

	printf("flyby.h was successfully created!\n");
	printf("Now compiling flyby...\n");

	/* Compile flyby... */

	cc=system("cc -Wall -O3 -s -fomit-frame-pointer -lm -lmenu -lncurses -pthread flyby.c -o flyby");

	/* Now install the programs and man page by creating symlinks
	   between the newly created executables and the destination
	   directory.  The default destination directory for the
	 	 executables is /usr/local/bin.  This default may be
	   overridden by specifying a different path as an argument
	   to this program (ie: installer /usr/bin).  Normally this
	   is passed along from the "build" script. */
	
	if (argc==2)
		strncpy(destination,argv[1],78);
	else	
		strncpy(destination,"/usr/local/bin/",15);

	/* Ensure a trailing '/' is
	   present in "destination". */

	x=strlen(destination);

	if (destination[x-1]!='/' && x!=0)
	{
		destination[x]='/';
		destination[x+1]=0;
	}

	if (cc==0)
	{
		printf("Linking flyby binaries to %s\n",destination);
		sprintf(dest_path,"%sflyby",destination);
		unlink(dest_path);
		sprintf(src_path,"%s/flyby",pwd);
		symlink(src_path,dest_path);
		sprintf(dest_path,"%sxflyby",destination);
		unlink(dest_path);
		sprintf(src_path,"%s/xflyby",pwd);
		symlink(src_path,dest_path);
		unlink("/usr/local/man/man1/flyby.1");
		sprintf(dest_path,"%s/docs/man/flyby.1",pwd);
		symlink(dest_path,"/usr/local/man/man1/flyby.1");

		printf("Done! Visit http://github.com/la1k/flyby for the latest news!\n");
	}

	else
	{
		printf("*** Compilation failed.  Program not installed.  :-(\n");
	}
	
	unlink("installer");
	exit(0);
}
