#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FONT_SIZE 5*1024*1024
char wqy_font[FONT_SIZE];
char wqy_font_list[65534][1024];

char *strrev(char *str)
{
      char *p1, *p2;

      if (! str || ! *str)
            return str;
      for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
      {
            *p1 ^= *p2;
            *p2 ^= *p1;
            *p1 ^= *p2;
      }
      return str;
}

int strpos(char *haystack, char *needle)
{
	char *p = strstr(haystack, needle);
	if (p)
	return p - haystack;
	return -1;
}


void bin( char*string )  
{
	char a[17];
	char str[8];
	char nstr[6];
	int i,j;
	int num;
	int cas;

	if(strlen(string) == 4) cas = 8; //0x00
	if(strlen(string) == 6) cas = 16; //0x0000

	if( string[0] != '0' && string[1] != 'x') return;

	num = strtol(string,NULL,16) ;
	
	i = num;
	memset(a,0,17);

	while (num != 0) 
	{
		memset(str,0,8);
		sprintf (str,"%d",num % 2); 
		num = num /2;
		strcat(a,str);
		
	}
	
	if( i == 0)
	{
		memset(a,'0',cas);
	}
	
	else
	{
		if(strlen(a) < cas)
		{
			for(j = strlen(a); j < cas; j++)
			{
				a[j] = '0';
				//printf("oh yeah ");
			}
		}
	}
	
	printf("%s\n",strrev(a) );
	//printf("%s\n", a);
}

void read_wqy_to_mem( char* wqy_file)
{
	FILE*fp;
	fp = NULL;
	fp = fopen(wqy_file,"r");
	if(fp == NULL)
	{
		printf("read wqy font file error\n");
		return;
	}
	
	fread(wqy_font, FONT_SIZE, 1, fp);
	fclose(fp);
}

void  read_font_to_list( char* file )
{
	char linebuf[512];
	char*pch,*bk;
	FILE*fp;
	int n;

	n = 0;
	fp = NULL;
	fp = fopen(file,"r");
	if(fp == NULL)
	{
		printf("read file error\n");
		return;
	}
	while(!feof(fp))
	{
		fgets(linebuf,512,fp);
		if(strpos(linebuf,"ENCODING") == 0)
		{
			pch = strtok_r(linebuf," ", &bk);
			pch = strtok_r(NULL," ", &bk);
		//	printf("= %s\n", pch);
			n = atoi(pch);
			
			while(!feof(fp))
			{
				fgets(linebuf,512,fp);
					
				if(strpos(linebuf,"ENDCHAR") == 0)
				{
					strcat(wqy_font_list[n],linebuf);
					break;
				}else
				{
					strcat(wqy_font_list[n], linebuf);
				}
			}
			//printf("%s\n",wqy_font_list[n]);	
			//fclose(fp); break;
		}
	}	
	fclose(fp);
	
}

void search_font(int n)
{
	char*pch;
	int abc;
	char def[6];
	
	abc=0;
	pch = NULL;
	pch = strtok(wqy_font_list[n], "\n");
	while(pch != NULL)
	{
		if(strstr(pch,"ENDCHAR"))
		{
			abc = 0;
		}
		if(abc == 1)
		{
			//printf("%s\n", pch);
			sprintf(def,"0x%s",pch);
			bin( def );
		}
		if(strstr(pch,"BITMAP"))
		{
			abc=1;
		}
		
		pch = strtok(NULL,"\n");
	}
}

int main(int argc,char**argv)
{
	int i;
	FILE*fp;
	char*file = "wqy10.bdf";
	
	//read_wqy_to_mem( "wqy.bdf");
	read_font_to_list(file);

	search_font( 65 );
	
	return 0;
		
}


