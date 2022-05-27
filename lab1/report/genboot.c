#include<stdio.h>
#include<stddef.h>

char buf[512];

int main(int argc,char**argv)
{
	if(argc!=2){
		fprintf(stderr,"ERROR: expected 1 argument, given %d\n",argc-1);
		return 2;
	}
	FILE* sig=fopen(argv[1],"r");
	int n=fread(buf,1,512,sig);
	fclose(sig);
	sig=NULL;
	if(n>510){
		fprintf(stderr,"ERROR: boot block too large: %d bytes (max 510)\n",n);
		return 1;
	}
	fprintf(stderr,"OK: boot block is %d bytes (max 510)\n",n);
	buf[510]=0x55;
	buf[511]=0xaa;
	sig=fopen(argv[1],"w");
	fwrite(buf,1,512,sig);
	fclose(sig);
	return 0;
}
