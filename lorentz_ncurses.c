#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <ncurses.h>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include <signal.h>
#include <pthread.h>

#define Max_l 10000
#define Scr_x 420 
#define Scr_y 420

int fbfd;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize;
char *fbp;
long int location;

int BytePixel;
int Linelength;

typedef struct {
	float x; float y; float z;
} data_3d;
typedef struct {
	int x; int y;
}POINT;

POINT pts[Max_l];
data_3d data_l[Max_l];
int c;

pthread_t pid[2];

void swap(int *a, int *b)
{
    int c = *a;
    *a = *b;
    *b = c;
}

void set_location(int x, int y)
{

	location =  ( x + vinfo.xoffset ) * (vinfo.bits_per_pixel/8) + 
		    ( y + vinfo.yoffset ) * finfo.line_length;
}

void draw16 (char*fb, int x0, int y0, int color)
{
    const int bytesPerPixel = 2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 2);
    const int blue = (color & 0xff) >> 3;
    const short color16 = blue | (green << 5) | (red << (5 + 6));

    short *dest = (short *) (fb) + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);
	dest[0] = color16;
/*	

*/

}

void draw_point( char *a, int x, int y, unsigned long color )
{
	if( x > vinfo.xres ) x = x - vinfo.xres;
	if( x < 0 )	x = x + vinfo.xres;
	if( y > vinfo.yres ) y = y - vinfo.yres;
	if( y < 0 )	y = y + vinfo.yres;

	switch(vinfo.bits_per_pixel)
	{
		case 16:
			draw16 (a, x,y, color);
		break;
		default:
			printf("only support 16bits framebuffer\n");
		break;
	}

}

void init_fb()
{
	fbfd = 0;
	screensize = 0;
	fbp = 0;
	location = 0;

        fbfd = open("/dev/fb0", O_RDWR);
        if (!fbfd) {
                printf("Error: cannot open framebuffer device.\n");
                exit(1);
        }
        printf("The framebuffer device was opened successfully.\n");

        if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
                printf("Error reading fixed information.\n");
                exit(2);
        }

        /* Get variable screen information */
        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
                printf("Error reading variable information.\n");
                exit(3);
        }

	BytePixel = vinfo.bits_per_pixel / 8;
	Linelength = finfo.line_length;

	printf("%d %d\n", vinfo.xres, vinfo.yres);

        /* Figure out the size of the screen in bytes */
        screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

        /* Map the device to memory */
        fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                fbfd, 0);
        if ((int)fbp == -1) {
        printf("Error: failed to map framebuffer device to memory.\n"); exit(4);
        }
        printf("The framebuffer device was mapped to memory successfully.\n");
	set_location(0,0);

}

void unmap_fb()
{
       munmap(fbp, screensize);
       close(fbfd);
}

int lorentz()
{
        float x = -15, y = 0, z = 0;
        float x1, y1, z1;
        float dt = .022;
        float a = 5, b = 15, c = 1;
        int i;

        for (i=0;i< 100 ;i++)
        {
                x1 = x +(-a *x *dt) +(a *y *dt);
                y1 = y +( b *x *dt) -(y *dt) -(z *x *dt);
                z1 = z +(-c *z *dt) +(x *y *dt);
                x = x1; y = y1; z = z1;
        }

        for (i=0;i< Max_l ;i++)
        {
                x1 = x +(-a *x *dt) + (a *y *dt);
                y1 = y +( b *x *dt) - (y *dt) -(z *x *dt);
                z1 = z +(-c *z *dt) + (x *y *dt);
                x = x1; y = y1; z = z1;

                data_l[i].x = x;
                data_l[i].y = y;
                data_l[i].z = z;
        }

                return 0;
}

int DrawLorentz()
{
        int i;
	int x,y;
	

        for(y=0;y<Scr_y; y++)
                for(x = 0; x < Scr_x; x++)
        {
                draw_point(fbp, x,  y, 0);
        }

		
        for (i=0;i< Max_l  ;i++)
        {
                pts[i].x = data_l[i].y *14 +210;
                pts[i].y = 410 -data_l[i].z *14;        
	}
	/*	
	for(y=0;y<Scr_y; y++)
		for(x = 0; x < Scr_x; x++)
	{
		draw_point(fbp, x,  y, 0);
	}
	*/

	for(i=0;i<Max_l;i++)
	{
		draw_point(fbp, pts[i].x, pts[i].y, 0x445566);
	}

        return 0;
}

int pro3d(float rz)
{
        int i;
        data_3d swap3d[ Max_l ];

        for(i=0;i< Max_l ;i++)
        {
                swap3d[i].x = data_l[i].x * cos(rz) - data_l[i].y * sin(rz);
                swap3d[i].y = data_l[i].x * sin(rz) + data_l[i].y * cos(rz);
                swap3d[i].z = data_l[i].z;

                data_l[i].x = swap3d[i].x;
                data_l[i].y = swap3d[i].y;
                data_l[i].z = swap3d[i].z;
        }

        return 0;
}

int action()
{
        int i;

        for(i=0;i<1350;i++)
        {
                pro3d(.02*i/300);
                        DrawLorentz ();
        }
        return 0;
}

char *choices[] = { 
			"Choice 1",
			"Choice 2",
			"Choice 3",
			"Choice 4",
			"Exit操",
		  };
int n_choices = sizeof(choices) / sizeof(char *);

void print_menu(WINDOW *menu_win, int highlight)
{
	int x, y, i;	

	x = 2;
	y = 2;
	box(menu_win, 0, 0);
	for(i = 0; i < n_choices; ++i)
	{	if(highlight == i + 1) /* High light the present choice */
		{	wattron(menu_win, A_REVERSE); 
			mvwprintw(menu_win, y, x, "%s", choices[i]);
			wattroff(menu_win, A_REVERSE);
		}
		else
			mvwprintw(menu_win, y, x, "%s", choices[i]);
		++y;
	}
	move(100,100);
	wrefresh(menu_win);
}

void init_ncurses()
{
	initscr();
	echo();
	keypad(stdscr,TRUE);	
}

void * thread_keypad(void* ptr)
{
	int c;

	while(1)
	{	c = wgetch(stdscr);
		switch(c)
		{	
			case 'q':
			{
				printf("exiting\n");
				pthread_kill(pid[1], SIGINT);
				//exit(0);
			}
				break;
			default:
				break;
		}
	}	

}

void *thread_dolorentz(void*ptr)
{
	lorentz();
	action();
}


int main()
{

	init_ncurses();
	init_fb();

	pthread_create(&pid[0], NULL, thread_keypad, NULL);

	pthread_create(&pid[1], NULL, thread_dolorentz,NULL);

	//pthread_join( pid[0], NULL);
	pthread_join( pid[1], NULL);
	
//	getch();
	
	unmap_fb();
	endwin();
        return 0;
}
