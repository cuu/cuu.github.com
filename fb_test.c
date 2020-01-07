/*
	./a.out > /dev/null
*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include <math.h>

#include <ncurses.h>
#include <signal.h>

#include <pthread.h>

typedef struct{
	float x,y;

}POINT;
#define DW 640 
#define DH 480 

#define tomid(x,dn,up) if(x<dn)x=dn;else if(x>up)x=up;
#define draw4(a, b, x, y, c) draw_point(a+x,b+y,c),draw_point(a+x,b-y,c),draw_point(a-x,b+y,c),draw_point(a-x,b-y,c)
#define sqrt2  1.414
#define PI 3.1415

#define FONT_SIZE 5*1024*1024
char wqy_font[FONT_SIZE];
char wqy_font_list[65534][256];
char unifont_list[65535][256];

typedef struct _culist{
	int x;
	int y;
	int w; //always be DW;
	int h; //always be DH/12 ,12 lists in one page
	unsigned short hi; // determine if highlight

	char str[1024];
	

	struct _culist*next;
	struct _culist*prev; // to retrive the parent's x, y ,then add DH/page_skip

}CUULIST;

typedef struct
{
	int x,y,w,h;
	
}RECT;

typedef struct
{
	RECT rc;
	char str[128];

}BUTTON;

typedef struct
{
	int x,y;
	float r;
}CIRCLE;

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
char *frameBuffer = 0;
long int screensize = 0;
int fbFd = 0;

int window_id; // 1 for ?? 2 for ?? 3 for ??
int page_skip; // it always be 12
int lst_count; // lst_count / page_skip , 0 for first, 1 for page 1 2 for page 2

unsigned char image[DH][DW];

int SIZE;

CUULIST clist[512];
CUULIST *lp;
CUULIST *start_lst;

pthread_t pid[2];

void init_kernel();
void init_fb();

void clr();

int draw_wa(char * ,int ,int , int , int ,int*);
void fill_rect (int, int, int, int, int);

void drawRect (int , int, int , int , int );
void draw4_rect(int ,int , int ,int , int , int , int , int , int);
int draw_string(char*, int, int, int);
void draw_zhutf8(char*, int, int, int);


CUULIST* create_list(int, int, int, int, char*);

int strpos(char *haystack, char *needle)
{
	char *p = strstr(haystack, needle);
	if (p)
	return p - haystack;
	return -1;
}

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

int get_a( char*str ) // get arbic word unicode encoding number ,arbic word are two bytes
{
        static int a[3];
        if( strlen(str) < 2) return -1;
        a[0] = str[0];
        a[1] = str[1];
        a[2] = (a[0] & 0x1F) << 6 | ( a[1] & 0x3f);        
	return a[2];
}

int get_c(char*str) // get chinese word utf8 encoding number
{
	int l;
	int i;
	int cs[4];	
	l = strlen(str);
	if(l == 3)
	{
		cs[0] = str[0] & 0x0f;
		cs[1] = str[1] & 0x3f;
		cs[2] = str[2] & 0x3f;
	//	cs[4] = cs[0] << 12 | cs[1] << 6 | cs[2] << 0;	
		return  cs[0] << 12 | cs[1] << 6 | cs[2] << 0;
	}

	return 0;
}
 
int rotate_xy( POINT*in, POINT*out, float ax,float ay, POINT * offset)
{
	float x = in->x - offset->x,
		y = in->y - offset->y,
		Cax = cos(ax),
		Sax = sin(ax);

	out->y = x*Sax + y*Cax;
	out->x = y*sin(ay) - x*cos(ay);
	
	return  0;
}

int bin( char*string ,char* ft_string,int w)  
{
	char a[17];
	char str[8];
	char nstr[6];
	int i,j;
	int num;
	int cas;
	char*b;

	if(strlen(string) == 4) cas = 8; //0x00
	if(strlen(string) == 6) cas = 16; //0x0000

	if( string[0] != '0' && string[1] != 'x') return -1;

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
	
//	printf("%s\n",strrev(a) );
	b = strrev(a);
	b[w] = '\0';

	strcpy(ft_string, b);

	return 0;
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

void  read_unifont_to_list( char* file )
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
					strcat(unifont_list[n],linebuf);
					break;
				}else
				{
					strcat(unifont_list[n], linebuf);
				}
			}
			//printf("%s\n",wqy_font_list[n]);	
			//fclose(fp); break;
		}
	}	
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
		if(strpos(linebuf,"SIZE") == 0)
		{
			pch = strtok_r(linebuf," ",&bk);
			pch = strtok_r(NULL," ",&bk);
			SIZE = atoi(pch);
			printf("SIZE = %d\n", SIZE);
		}
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

void draw_unifont(int x, int y, int color, int n, int*bbx)
{
	char*pch;
	char*pch1;
	char*bk;
	int abc;
	char def[6];
	int i,j;
	char font_string[17];
	char font_list_str[1024];
	//int bbx[4];
	char buf[64];

	i = 0;
	j = 0;	
	abc=0;
	pch = NULL;
	strcpy(font_list_str, unifont_list[n]);

	pch = strtok(font_list_str, "\n");
//	printf("%s\n",pch);
	while(pch != NULL)
	{
		if(strstr(pch,"ENDCHAR"))
		{
			abc = 0;
		}
		if(abc == 1)
		{
		//	printf("%s\n", pch);
			sprintf(def,"0x%s",pch);
			bin( def,&font_string[0], bbx[0] );
			draw_wa(font_string, x, y+i, strlen(font_string), color, bbx);
			i++;
		}
		if(strstr(pch,"BITMAP"))
		{
			abc=1;
		}

		pch = strtok(NULL,"\n");
		//	memset(font_string, 0, 17);
	}
	return;
}

void draw_font(int x, int y, int color, int n, int*bbx)
{
	char*pch;
	char*pch1;
	char*bk;
	int abc;
	char def[6];
	int i,j;
	static char font_string[17];
	char font_list_str[1024];
	//int bbx[4];
	char buf[64];

	i = 0;
	j = 0;	
	abc=0;
	pch = NULL;
	strcpy(font_list_str, wqy_font_list[n]);

	pch = strtok(font_list_str, "\n");
	//	printf("%s\n",pch);
	while(pch != NULL)
	{
		if(strstr(pch,"ENDCHAR"))
		{
			abc = 0;
		}
		if(abc == 1)
		{
			//	printf("%s\n", pch);
			sprintf(def,"0x%s",pch);
			bin( def,&font_string[0], bbx[0] );
			draw_wa(font_string, x, y+i, strlen(font_string), color, bbx);
			i++;
		}
		if(strstr(pch,"BITMAP"))
		{
			abc=1;
		}
		
		pch = strtok(NULL,"\n");
		memset(font_string, 0, 17);
	}
	return;
}

void printFixedInfo ()
{
    printf ("Fixed screen info:\n"
            "\tid: %s\n"
            "\tsmem_start: 0x%lx\n"
            "\tsmem_len: %d\n"
            "\ttype: %d\n"
            "\ttype_aux: %d\n"
            "\tvisual: %d\n"
            "\txpanstep: %d\n"
            "\typanstep: %d\n"
            "\tywrapstep: %d\n"
            "\tline_length: %d\n"
            "\tmmio_start: 0x%lx\n"
            "\tmmio_len: %d\n"
            "\taccel: %d\n"
            "\n",
            finfo.id, finfo.smem_start, finfo.smem_len, finfo.type,
            finfo.type_aux, finfo.visual, finfo.xpanstep, finfo.ypanstep,
            finfo.ywrapstep, finfo.line_length, finfo.mmio_start,
            finfo.mmio_len, finfo.accel);
}

//打印fb驱动中var结构信息，注：fb驱动加载后，var结构可根据实际需要被重置
void
printVariableInfo ()
{
    printf ("Variable screen info:\n"
            "\txres: %d\n"
            "\tyres: %d\n"
            "\txres_virtual: %d\n"
            "\tyres_virtual: %d\n"
            "\tyoffset: %d\n"
            "\txoffset: %d\n"
            "\tbits_per_pixel: %d\n"
            "\tgrayscale: %d\n"
            "\tred: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tgreen: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tblue: offset: %2d, length: %2d, msb_right: %2d\n"
            "\ttransp: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tnonstd: %d\n"
            "\tactivate: %d\n"
            "\theight: %d\n"
            "\twidth: %d\n"
            "\taccel_flags: 0x%x\n"
            "\tpixclock: %d\n"
            "\tleft_margin: %d\n"
            "\tright_margin: %d\n"
            "\tupper_margin: %d\n"
            "\tlower_margin: %d\n"
            "\thsync_len: %d\n"
            "\tvsync_len: %d\n"
            "\tsync: %d\n"
            "\tvmode: %d\n"
            "\n",
            vinfo.xres, vinfo.yres, vinfo.xres_virtual, vinfo.yres_virtual,
            vinfo.xoffset, vinfo.yoffset, vinfo.bits_per_pixel,
            vinfo.grayscale, vinfo.red.offset, vinfo.red.length,
            vinfo.red.msb_right, vinfo.green.offset, vinfo.green.length,
            vinfo.green.msb_right, vinfo.blue.offset, vinfo.blue.length,
            vinfo.blue.msb_right, vinfo.transp.offset, vinfo.transp.length,
            vinfo.transp.msb_right, vinfo.nonstd, vinfo.activate,
            vinfo.height, vinfo.width, vinfo.accel_flags, vinfo.pixclock,
            vinfo.left_margin, vinfo.right_margin, vinfo.upper_margin,
            vinfo.lower_margin, vinfo.hsync_len, vinfo.vsync_len,
            vinfo.sync, vinfo.vmode);
}

void swap(int *a, int *b)
{
    int c = *a;
    *a = *b;
    *b = c;
}


//画大小为width*height的同色矩阵，8alpha+8reds+8greens+8blues
void drawRect_rgb32 (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 4;
    const int stride = finfo.line_length / bytesPerPixel;

    int *dest = (int *) (frameBuffer)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color;
        }
        dest += stride;
    }
}

int draw16 (int x0, int y0, int color)
{
    int bytesPerPixel = 2;
    int stride = finfo.line_length / bytesPerPixel;
    int red = (color & 0xff0000) >> (16 + 3);
    int green = (color & 0xff00) >> (8 + 2);
    int blue = (color & 0xff) >> 3;
    short color16 = blue | (green << 5) | (red << (5 + 6));

    short *dest = (short *) (frameBuffer) + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);

	dest[0] = color16;
/*	

    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color16;
        }
        dest += stride;
    }
*/
	return 0;
}

int draw_point( int x, int y, int color )
{
	//if( x > vinfo.xres ) x = x - vinfo.xres;
	//if( x < 0 )	x = x + vinfo.xres;
	//if( y > vinfo.yres ) y = y - vinfo.yres;
	//if( y < 0 ) y = y + vinfo.yres;
	
	if( x > DW ) return;
	if( x < 0 ) return;
	if( y > DH) return;
	if( y < 0 ) return;

	switch(vinfo.bits_per_pixel)
	{
		case 16:
			draw16 (x, y, color);
		break;
		default:
			printf("only support 16bits framebuffer\n");
		break;
	}
	return 0;
}

void draw_line(int x0, int y0, int x1, int y1, int color)
{
    int deltax, deltay;
    int deltak, mid, up, t, x, y;
    tomid(x0, 0, vinfo.xres);
    tomid(x1, 0, vinfo.xres);
    tomid(y0, 0, vinfo.yres);
    tomid(y1, 0, vinfo.yres);
    if (x0 < x1)
        deltax = x1 - x0;
    else
        deltax = x0 - x1;
    if (y0 < y1)
        deltay = y1 - y0;
    else
        deltay = y0 - y1;
    if (deltax < deltay) {    /* if y change faster than x, then use this block */
        if (y0 > y1)
            swap(&x0, &x1), swap(&y0, &y1);
        deltak = 2 * (x1 - x0);
        mid = y1 - y0;
        up = 2 * mid;
        draw_point(x0, y0, color);
        y = y0, x = x0;
        t = 0;
        if (deltak < 0) {    /* if x0 > x1, x shoud degression */
            deltak = -deltak;
            while (y < y1) {
                y++;
                t += deltak;
                if (t > mid)
                    x--, t -= up;
                draw_point(x, y, color);
            }
        } else {
            while (y < y1) {
                y++;
                t += deltak;
                if (t > mid)
                    x++, t -= up;
                draw_point(x, y, color);
            }
        }
    } else {        /* x change faster than y */
        if (x0 > x1)
            swap(&x0, &x1), swap(&y0, &y1);
        deltak = 2 * (y1 - y0);
        mid = x1 - x0;
        up = 2 * mid;
        draw_point(x0, y0, color);
        y = y0, x = x0;
        t = 0;
        if (deltak < 0) {
            deltak = -deltak;
            while (x < x1) {
                x++;
                t += deltak;
                if (t > mid)
                    y--, t -= up;
                draw_point(x, y, color);
            }
        } else {
            while (x < x1) {
                x++;
                t += deltak;
                if (t > mid)
                    y++, t -= up;
                draw_point(x, y, color);
            }
        }
    }
}

void circle(int px, int py, int r, int color)
{
    int dis, limit;
    int x = 0, y = r;
    dis = 3 - 2 * r;
    limit = (int) (r / sqrt2);
    while (x <= limit) {
        draw4(px, py, x, y, color);
        draw4(px, py, y, x, color);
        if (dis < 0) {
            dis += 4 * x + 6;
        } else {
            dis += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void ellipse(int px, int py, int a, int b, int color)
{
    int dis, x, y, crt;
    int a2, b2, ab2;
    a2 = a * a;
    b2 = b * b;
    ab2 = a2 + b2;
    if (a > b) {
        crt = b2 * a;
        x = a, y = 0;
        dis = a * b * b;
        while (crt >= 0) {
            draw4(px, py, x, y, color);
            if (dis > 0) {
                dis += 2 * (a2 * y - b2 * x) + ab2;
                crt -= ab2;
                x--, y++;
            } else {
                dis += 2 * a2 * y + a2;
                crt -= a2;
                y++;
            }
        }
        while (x >= 0) {
            draw4(px, py, x, y, color);
            if (dis > 0) {
                dis += b2 - 2 * b2 * x;
                x--;
            } else {
                dis += 2 * (a2 * y - b2 * x) + ab2;
                x--, y++;
            }
        }
    } else {
        crt = a2 * b;
        x = 0, y = b;
        dis = b * a * a;
        while (crt >= 0) {
            draw4(px, py, x, y, color);
            if (dis > 0) {
                dis += 2 * (b2 * x - a2 * y) + ab2;
                crt -= ab2;
                y--, x++;
            } else {
                dis += 2 * b2 * x + b2;
                crt -= b2;
                x++;
            }
        }
        while (y >= 0) {
            draw4(px, py, x, y, color);
            if (dis > 0) {
                dis += a2 - 2 * a2 * y;
                y--;
            } else {
                dis += 2 * (b2 * x - a2 * y) + ab2;
                y--, x++;
            }
        }
    }
}

void add_list( char* s )
{
	CUULIST*p;
	p = start_lst;
	while(p!=NULL)
	{
		
		if( p->next == NULL)
		{
		//	p = (CUULIST*)malloc(sizeof(CUULIST));
		//	p->next = create_list(p->x, p->y+p->h, p->w,p->h, s);
			p->next = create_list(p->x, p->y, p->w, p->h, s);
			p->next->next = NULL;
			p->next->prev = p;
			return;
			break;
		}
		p = p->next;
	}
	
	start_lst = create_list( 0,0,DW, DH/page_skip, s);
	start_lst->next=NULL;
	start_lst->prev=NULL;
	start_lst->hi = 1;
	
}

CUULIST* create_list(int x, int y, int w, int h, char*string)
{
	CUULIST*p;
	p = (CUULIST*)malloc( sizeof(CUULIST));
	p->x = x;
	p->y = y;
	p->w = w;
	p->h = h;
	p->next = NULL;
	p->prev = NULL;
	p->hi = 0;
	strcpy(p->str, string);
	return p;
}

void draw_list( int fgcolor, int bgcolor )
{
	CUULIST*p;
	p = start_lst;
	clr();
	int i,j,k;
	i = 0;
	k = 0;
	j = lst_count - page_skip;

	while(p!= NULL)
	{
		if( i >= j || j <= 0 )
		{
			if(p ->hi == 0)
			{
				drawRect (p->x, p->y+ k*p->h, p->w, p->h, fgcolor);	
				draw_string( p->str , p->x+5, p->y+k*p->h+12, bgcolor);	
			
			}else	
			{
				fill_rect(p->x, p->y+ k*p->h, p->w, p->h, fgcolor);
				draw_string(p->str, p->x+5, p->y+k*p->h+12, bgcolor);	
			}
			k++;
			if( k >= page_skip)  break;
		}
		i++;
		p = p->next;
	}	
}

void init_list(CUULIST*p)
{
	
	p->x = 0;
	p->y = 0;
	p->w = DW;
	p->h = DH/page_skip;
	p->next = NULL;
	p->prev = NULL;	

	p->hi = 0;
}

void draw_a_button(int x, int y, int w, int h ,int color, int type, char*title)
{
//	type 0 1 2 , a,c,e arbic,chinese, english
	int n;
	int w0,h0;

	n = 0;
	fill_rect(x, y, w, h, 0);

	drawRect(x,y,w,h,color);
	switch(type)
	{
		case 0:
			n = strlen(title);
			n = n/2;
			n = n *16;
			w0 = (w-n)/2;
		//	fill_rect(x+w0, y+2, n,h-2, 0);
			draw_arabic(title,x+w0,y+2,  color);
		break;
		case 1:
			n = strlen(title);
			n = n /3;
			n = n * 16;
			w0 = (w-n)/2;
			draw_zhutf8(title, x+w0,y+2, color);		
		break;
		case 2:
			
			n = strlen(title);
			n = n * 10;
			w0 = (w-n)/2;
			draw_string(title, x+w0,y+2, color);	
		break;
		default:break;
	}	

}

void draw_a_car(int x,int y, int w ,int h, int color, float angle)
{
	POINT a[4],b[4],c[4];

	int px,py;
	int mw,mh;
	int pw,ph;
	mw = w * 0.73;
	mh = w * 0.66;
	mh = mh /2;
	pw = (w - mw)/2;

	c[0].x = x + w/2;
	c[0].y = y + h/2;
	
	a[0].x = x+pw;
	a[0].y = y;
	a[1].x = x + pw + mw;
	a[1].y = y + mh;
	
	a[2].x = x+pw+mw;
	a[2].y = y;
	
	a[3].x = x + pw;
	a[3].y = y + mh;
	
	rotate_xy(&a[0], &b[0], angle, angle, &c[0]);
	rotate_xy(&a[1], &b[1], angle, angle, &c[0]);
	
	rotate_xy(&a[2], &b[2], angle, angle, &c[0]);
	rotate_xy(&a[3], &b[3], angle, angle, &c[0]);

	draw4_rect(c[0].x+b[0].x, c[0].y +b[0].y,
			c[0].x+b[3].x, c[0].y+b[3].y,
			c[0].x+b[2].x, c[0].y+b[2].y,
			c[0].x+b[1].x, c[0].y+b[1].y, 0x0000ff);
	
	drawRect(x+pw, y, mw,mh, color );

	a[0].x = x;
	a[0].y = y+mh;
	
	a[1].x = x + w;
	a[1].y = y + mh;

	rotate_xy(&a[0], &b[0], angle, angle, &c[0]);
	rotate_xy(&a[1], &b[1], angle, angle, &c[0]);

	draw_line( c[0].x + b[0].x, c[0].y+b[0].y, c[0].x+ b[1].x, c[0].y+b[1].y, 0x0000ff);
	draw_line(x, y+mh, x+w, y+mh, color);

	a[0].x = x+pw; a[0].y = y+mh+mh/3; 
	rotate_xy(&a[0], &b[0], angle, angle, &c[0]);
	circle( c[0].x + b[0].x, c[0].y+b[0].y, mh/3, 0x0000ff);
	circle(x+pw,y+mh+mh/3, mh/3, color );
	a[0].x = x+pw+mw; a[0].y = y+mh+mh/3; 
	rotate_xy(&a[0], &b[0], angle,angle, &c[0]);
	circle( c[0].x + b[0].x, c[0].y+b[0].y, mh/3, 0x0000ff);

	circle(x+pw+mw, y+mh+mh/3, mh/3, color);	
			
}

int draw_wa(char * line,int x,int y, int n, int color,int*bbx)
{
	int i;

	
	
	for(i=0; i< n; i++)
	{
		if(line[i] == '1')	
		{
			//printf("1 %d",n);
			
			draw_point(x+i + bbx[2],y+(SIZE-bbx[1] - bbx[3]), color);
		}
		
	}
	
//	printf("\n");	
	return 0;
}

void draw_word( int a, int x,int y, int color, int acjk, int *bbx)
{
	unsigned int enc_a;
	enc_a = a;
	// to find this word in wqy.bdf
	/*
00010000 
01111100 
10010010
10010010
10010000
01010000
00111000
00010100
00010010
10010010
10010010
01111100
00010000
*/
/*	
*/
/*
	draw_wa("1000000010000000", x, y+10, 16, color);
*/
	switch(acjk)
	{
		case 0:
			draw_unifont(x,y, color, a,bbx );
		break;
		case 1:
			draw_font(x, y, color, a, bbx);
		break;
		default:
			printf("Font not support!\n");
		break;
	}

	return;
}

void draw_meter(int x, int y, int ang, int r, int color)
{
	if( ang == 90 ) 
	{
		circle( x,y, r, color);
		
	}

}

void fillRect_rgb16 (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 2);
    const int blue = (color & 0xff) >> 3;
    const short color16 = blue | (green << 5) | (red << (5 + 6));

    short *dest = (short *) (frameBuffer) + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);
//short *dest = (short *) (frameBuffer) + (y0 + DH) * stride + (x0 + DW);
	int x ,y;

	if( x0+ width > DW) return;
	if( y0+ height > DH) return;

	
/*	for(x = 0; x < width; x++)
	{
		dest[x] = color16;
	
	}
	for(y = 0; y < height; y++)
	{
		dest[ 0 ] = color16;
		dest[ width  ] = color16;
		
		dest+= stride;
	}
	
	for(x = 0; x <= width; x++)
	{
		dest[x] = color16;
	}
*/
	
/*
	for(y = 0; y < height; y++)
	{
		dest[0] = color16;
		dest[width] = color16;
		dest+= stride;
	}
*/
    for (y = 0; y < height; ++y)
    {
	if( y > DH) break;
        for (x = 0; x < width; ++x)
        {
		if( x > DW ) break;
            dest[x] = color16;
        }
	dest[0] = color16;
	dest[width] = color16;
        dest += stride;
    }


}

int draw_arabic(char*str, int x, int y, int color)
{
	char*pch;
	char*pch1;
	char*bk;
	int i,j;
	char font_list_str[1024];
	int bbx[4];
	char buf[64];
	char arb[3];
	int k;
	int n;
	int l;
	
	i = 0;
	j = 0;

	//abc=0;
	pch = NULL;
	k = 0;
	
	l = strlen(str);
	for(i=0; i<l; i++)
	{
		
		if( (int)str[i] > 127 || (int)str[i] < 0 )
		{
			if( i + 1 <= l)
			{
				arb[0] = str[i];
				arb[1] = str[i+1];
				n = get_a ( arb );
				//printf(">127 %d\n", n);
			}
			i++;
		}else if( (int)str[i] < 127 || (int)str[i] > 0 )
		{
			n = (int)str[i];
			//printf("<127 %d\n", n);
		}
		
		//n = 25105;

		strcpy(font_list_str, unifont_list[n]);

		pch = strtok(font_list_str, "\n");
	//	printf("%s\n",pch);
		while(pch != NULL)
		{
			if(strstr(pch,"BBX"))
			{
				strcpy(buf,pch);
			
				pch1 = strtok_r(buf," ",&bk);
				j = 0;
				while(pch1 != NULL)
				{
					//printf("%s\n", pch1);
					pch1 = strtok_r(NULL," ",&bk);
					if(pch1!=NULL)
					{
						bbx[j] = strtol(pch1,NULL, 10);
					}
					j++;
				}	
			
			}
			pch = strtok(NULL,"\n");		
		}

		draw_word( n , x + k, y, color, 0, bbx);
		k+= bbx[0]+3; 
	}
	return k;
}

void draw_zhutf8(char*str, int x, int y, int color)
{
	char*pch;
	char*pch1;
	char*bk;
	char *bk0;
	int i,j;
	char font_list_str[1024];
	int bbx[4];
	char buf[64];
	char cjk[4];

	int k;
	int n;
	int l;
	
	i = 0;
	j = 0;

	//abc=0;
	pch = NULL;
	k = 0;
	
	l = strlen(str);
	for(i=0; i<l; i++)
	{
		
		memset(cjk,0,3);
		if( (int)str[i] > 127 || (int)str[i] < 0 )
		{	
			if( i + 2 <= l)
			{
				cjk[0] = str[i];
				cjk[1] = str[i+1];
				cjk[2] = str[i+2];
				n = get_c ( cjk);
			//	printf("%d\n", n);
			}
			i+=2;
		}else if( (int)str[i] < 127 || (int)str[i] > 0 )
		{
			n = (int)str[i];
			//printf("<127 %d\n", n);
		}


		
		//n = 25105;

		strcpy(font_list_str, wqy_font_list[n]);

		pch = strtok_r(font_list_str, "\n",&bk0);
	//	printf("%s\n",pch);
		while(pch != NULL)
		{
			if(strstr(pch,"BBX"))
			{
				strcpy(buf,pch);
			
				pch1 = strtok_r(buf," ",&bk);
				j = 0;
				while(pch1 != NULL)
				{
					//printf("%s\n", pch1);
					pch1 = strtok_r(NULL," ",&bk);
					if(pch1!=NULL)
					{
						bbx[j] = strtol(pch1,NULL, 10);
					}
					j++;
				}	
			
			}
			pch = strtok_r(NULL,"\n",&bk0);		
		}

		draw_word( n , x + k, y, color, 1, bbx);
		k+= bbx[0]+3; 
	}

//	return k;
}

int draw_string(char*str, int x, int y, int color)
{
	char*pch;
	char*pch1;
	char*bk;
	int i,j;
	char font_list_str[1024];
	int bbx[4];
	char buf[64];
	int k;
	int n;

	i = 0;
	j = 0;
	
	//abc=0;
	pch = NULL;
	k = 0;
	for(i=0;i< strlen(str); i++)
	{
		n = (int)str[i];

		strcpy(font_list_str, wqy_font_list[n]);

		pch = strtok(font_list_str, "\n");
	//	printf("%s\n",pch);
		while(pch != NULL)
		{
			if(strstr(pch,"BBX"))
			{
				strcpy(buf,pch);
			
				pch1 = strtok_r(buf," ",&bk);
				j = 0;
				while(pch1 != NULL)
				{
					//printf("%s\n", pch1);
					pch1 = strtok_r(NULL," ",&bk);
					if(pch1!=NULL)
					{
						bbx[j] = strtol(pch1,NULL, 10);
					}
					j++;
				}	
			
			}
			pch = strtok(NULL,"\n");		
		}

		draw_word( n , x + k, y, color, 1, bbx);
		k+= bbx[0]+3; 
	}
	return k;
}

//画大小为width*height的同色矩阵，5reds+6greens+5blues

void
drawRect_rgb16 (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 2);
    const int blue = (color & 0xff) >> 3;
    const short color16 = blue | (green << 5) | (red << (5 + 6));

    short *dest = (short *) (frameBuffer) + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);

	int x,y;

	if( x0+width > DW) return;
	if( y0+height > DH) return;
	
	for(x = 0; x < width; x++)
	{
		dest[x] = color16;
	
	}
	for(y = 0; y < height; y++)
	{
		dest[ 0 ] = color16;
		dest[ width  ] = color16;
		
		dest+= stride;
	}
	
	for(x = 0; x <= width; x++)
	{
		dest[x] = color16;
	}

/*	

    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color16;
        }
        dest += stride;
    }
*/

}

//画大小为width*height的同色矩阵，5reds+5greens+5blues
void
drawRect_rgb15 (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 3);
    const int blue = (color & 0xff) >> 3;
    const short color15 = blue | (green << 5) | (red << (5 + 5)) | 0x8000;

    short *dest = (short *) (frameBuffer)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color15;
        }
        dest += stride;
    }
}

void draw4_rect(int x0,int y0, int x1,int y1, int x2, int y2, int x3, int y3, int color)
{

	/*
	(x0,y0)---------(x2,y2)
	|		|
	|		|
	|		|
	(x1,y1)---------(x3,y3)
	*/	

	draw_line(x0,y0, x2,y2, color);
	draw_line(x0,y0, x1,y1, color);
	draw_line(x1,y1, x3,y3, color);
	draw_line(x2,y2, x3,y3, color);

}

void fill_rect (int x0, int y0, int width, int height, int color)
{
    switch (vinfo.bits_per_pixel)
    {
    case 32:
        //drawRect_rgb32 (x0, y0, width, height, color);
        break;
    case 16:
        fillRect_rgb16 (x0, y0, width, height, color);
        break;
    case 15:
       	//fillRect_rgb15 (x0, y0, width, height, color);
        break;
    default:
        printf ("Warning: fill rect() not implemented for color depth %i\n",
                vinfo.bits_per_pixel);
        break;
    }
}


void
drawRect (int x0, int y0, int width, int height, int color)
{
    switch (vinfo.bits_per_pixel)
    {
    case 32:
        drawRect_rgb32 (x0, y0, width, height, color);
        break;
    case 16:
        drawRect_rgb16 (x0, y0, width, height, color);
        break;
    case 15:
        drawRect_rgb15 (x0, y0, width, height, color);
        break;
    default:
        printf ("Warning: drawRect() not implemented for color depth %i\n",
                vinfo.bits_per_pixel);
        break;
    }
}

#define PERFORMANCE_RUN_COUNT 5
void
performSpeedTest (void *fb, int fbSize)
{
    int i, j, run;
    struct timeval startTime, endTime;
    unsigned long long results[PERFORMANCE_RUN_COUNT];
    unsigned long long average;
    unsigned int *testImage;

    unsigned int randData[17] = {
        0x3A428472, 0x724B84D3, 0x26B898AB, 0x7D980E3C, 0x5345A084,
        0x6779B66B, 0x791EE4B4, 0x6E8EE3CC, 0x63AF504A, 0x18A21B33,
        0x0E26EB73, 0x022F708E, 0x1740F3B0, 0x7E2C699D, 0x0E8A570B,
        0x5F2C22FB, 0x6A742130
    };

    printf ("Frame Buffer Performance test...\n");
    for (run = 0; run < PERFORMANCE_RUN_COUNT; ++run)
    {
        /* Generate test image with random(ish) data: */
        testImage = (unsigned int *) malloc (fbSize);
        j = run;
        for (i = 0; i < (int) (fbSize / sizeof (int)); ++i)
        {
            testImage[i] = randData[j];
            j++;
            if (j >= 17)
                j = 0;
        }

        gettimeofday (&startTime, NULL);
        memcpy (fb, testImage, fbSize);
        gettimeofday (&endTime, NULL);

        long secsDiff = endTime.tv_sec - startTime.tv_sec;
        results[run] =
            secsDiff * 1000000 + (endTime.tv_usec - startTime.tv_usec);

        free (testImage);
    }

    average = 0;
    for (i = 0; i < PERFORMANCE_RUN_COUNT; ++i)
        average += results[i];
    average = average / PERFORMANCE_RUN_COUNT;

    printf (" Average: %llu usecs\n", average);
    printf (" Bandwidth: %.03f MByte/Sec\n",
            (fbSize / 1048576.0) / ((double) average / 1000000.0));
    printf (" Max. FPS: %.03f fps\n\n",
            1000000.0 / (double) average);

    /* Clear the framebuffer back to black again: */
    memset (fb, 0, fbSize);
}

void clr()
{
	int x,y;
	for(y = 0; y< DH; y++)
		for(x=0; x< DW; x++)
	{
		draw_point(x,y, 0);
	}
}

void init_fb()
{
    const char *devfile = "/dev/fb0";
		
    fbFd = open (devfile, O_RDWR);
    if (fbFd == -1)
    {
        perror ("Error: cannot open framebuffer device");
        exit (1);
    }


    if (ioctl (fbFd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror ("Error reading fixed information");
        exit (2);
    }

//    printFixedInfo ();

    if (ioctl (fbFd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror ("Error reading variable information");
        exit (3);
    }
//    printVariableInfo ();

    screensize = finfo.smem_len;

    frameBuffer =
        (char *) mmap (0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                     fbFd, 0);
    if (frameBuffer == MAP_FAILED)
    {
        perror ("Error: Failed to map framebuffer device to memory");
        exit (4);
    }


}

void init_ncurses()
{
	initscr();
	echo();
	keypad(stdscr,TRUE);
	//move(700,700);	
	curs_set(0);
}

void * thread_keypad(void* ptr)
{
	int c;
	CUULIST *p;
	static int n;
	n =1;
	while( n == 1)
	{	c = wgetch(stdscr);
		p = start_lst;

		switch(c)
		{
			case 'a':
			{	clr();
				draw_a_button(15, 0, 100, 30 ,0x00ff00, 0,"سشط ف ك");
			}break;
			case 'c':
			{	clr();
				draw_a_button(15, 0, 100, 30 ,0x00ff00, 1,"按妞");
			}break;
			case 'e':
			{
				clr();
				draw_a_button(15, 0, 100, 30 ,0x00ff00, 2,"button");	
			}break;	
			case 'q':
			{
				printf("exiting\n");
				n = 0;
				pthread_kill(pid[1], SIGINT);
				//exit(0);
			}
			break;
			case KEY_LEFT:
			{
				
			}break;
			case KEY_RIGHT:
			{
				
			}break;
			case KEY_UP:
			{
				while(p!=NULL)
				{
					if(p->hi == 1 && p->prev!=NULL)
					{
						p->hi=0;
						p->prev->hi=1;
						lst_count--;
						draw_list(0x00ff00,0xff0000);
						break;	
					}
					p = p->next;
				}	
			}break;
			case KEY_DOWN:
			{
				while(p!=NULL)
				{
					if(p->hi == 1 && p->next!=NULL)
					{
						p->hi = 0;
						p->next->hi=1;
						lst_count++;
						draw_list(0x00ff00, 0xff0000);
						break;
					}
					p = p->next;
				}				
			}break;
			default:
				break;
		}
	}	

}


void init_kernel()
{
	init_fb();

	init_ncurses();

	read_font_to_list("wqy.bdf");
	read_unifont_to_list("unifont5.1.bdf");
	
	window_id = 0;
	page_skip = 12;

	start_lst = NULL;
	lst_count = 1;	
	//init_list(start_lst);

}

float angle(int n)
{
	return (float)n * PI/180.0f;
}

void *thread_docu(void*ptr)
{
	int i;
	char buf[32];
	init_kernel();
	clr();
/*
	add_list("first");
	add_list("two");
	add_list("three");
	add_list("four");
	add_list("five");
	add_list("six");
	add_list("seven");
add_list("eight");
add_list("nine");
add_list("ten");
add_list("eleven");
add_list("twelve");
add_list("thirteen");
add_list("forteen");
add_list("fifteen");
add_list("sixteen");
add_list("sevevteen");
	for(i = 1;i< 30;i++)
	{
		sprintf(buf, "------%d\n", 17+i);
		add_list(buf);
	}

	draw_list(0x00ff00, 0xff0000);
*/
	draw_a_button(15, 0, 100, 30 ,0x00ff00, 0,"سشط ف ك");

	draw_a_button(150, 0, 100, 30 ,0x00ff00, 1,"威控");

	draw_arabic("سشط ف ك", 250, 350+20+20, 0xffff0000);
	draw_arabic("سشط ف ك", 250, 350+20+20+40, 0xffff0000);

	draw_zhutf8("威控", 250,350+20, 0xffff0000);
	draw_zhutf8("威控2", 250,350+40+20, 0xffff0000);
}

int main (int argc, char **argv)
{
	
	int a,b;
	int x,y;

//    performSpeedTest (frameBuffer, screensize);

//  printf ("Will draw 3 rectangles on the screen,\n"
//        "they should be colored red, green and blue (in that order).\n");

/*	
	drawRect(10, 0, 150,25, 0xffff0000);

	drawRect(170,0, 150,25, 0xffff0000);

	drawRect(10,  455, 150, 25, 0xffff0000);
	drawRect(170, 455, 150, 25, 0xffff0000);

	drawRect(0,0,640,480, 0x00ff00);

//   drawRect (vinfo.xres * 3 / 8, vinfo.yres * 3 / 8,
//             vinfo.xres / 4, vinfo.yres / 4, 0xff00ff00);
//    drawRect (vinfo.xres * 5 / 8, vinfo.yres * 5 / 8,
//             vinfo.xres / 4, vinfo.yres / 4, 0xff0000ff);

	circle( 190, 200, 50, 0x667788);
	
	a = (int)(cos( angle( 470) )*50.0f);
	b = (int)(sin( angle( 470) )*50.0f);

	printf("%d %d\n", a,b);
	draw_line(190,200, 190+a, 200-b, 0xffff0000);
	

	draw_string("ABCDEFGHIJKLMNOPQ$abcdg", 250,350, 0xffff0000);
	draw_zhutf8("威abced 控", 250,350+20, 0xffff0000);
	
	draw_arabic("سشط ف ك", 250, 350+20+20, 0xffff0000);

	draw_string("Test" , 15, 2, 0xffff0000);
	draw_string("Test2", 175,2, 0xffff0000);
	
	draw_string("Test3", 15,455+2, 0xffff0000);
	draw_string("Test4", 175, 455+2, 0xffff0000);

	draw_a_car(30,350, 150 ,50, 0x00ff00, angle( 67 ) );

	for(x =0; x < 1000; x++)
		for(y = 0; y+=10;y<360)
	{
		circle(190,200,50,0x667788);
		a = (int)(cos( angle( y ) )*50.0f);
		b = (int)(sin( angle( y ) )*50.0f);
		draw_line(190,200, 190+a, 200-b, 0xffff0000);
		usleep(7000);
		fill_rect(190-50,200-50,50*2,50*2, 0);
	}
*/
	
	//fill_rect (0, 0, 640, 40, 0x00ff00);
	//draw_string("abcdefghijklmnopqrstuvwxyz" , 5, 12, 0x0000ff);
	

	pthread_create(&pid[0], NULL, thread_keypad, NULL);
	
	pthread_create(&pid[1], NULL, thread_docu, NULL);

	pthread_join( pid[0], NULL);	
		
//	getchar();
	
	printf (" Done.\n");


	munmap (frameBuffer, screensize);    //解除内存映射，与mmap对应
	close (fbFd);
	endwin();
	return 0;
}

