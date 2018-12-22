/*------------------------------------------------------------------------------
Referring to: http://blog.chinaunix.net/uid-22666248-id-285417.html

 本文的copyright归yuweixian4230@163.com 所有，使用GPL发布，可以自由拷贝，转载。
但转载请保持文档的完整性，注明原作者及原链接，严禁用于任何商业用途。

作者：yuweixian4230@163.com
博客：yuweixian4230.blog.chinaunix.net


TODO:
0.  draw_dot(): check FB mem. boundary during writing. ---OK


Modified by Midas-Zhou
1. add
-----------------------------------------------------------------------------*/
#include "fblines.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <math.h>


#ifndef _TYPE_FBDEV_
#define _TYPE_FBDEV_
    typedef struct fbdev{
        int fdfd; //open "dev/fb0"
        struct fb_var_screeninfo vinfo;
        struct fb_fix_screeninfo finfo;
        long int screensize;
        char *map_fb;
    }FBDEV;
#endif


/* global variale, Frame buffer device */
FBDEV   gv_fb_dev;


 /* default color set */
 static uint16_t fb_color=(30<<11)|(10<<5)|10;  //r(5)g(6)b(5)

 void init_dev(FBDEV *dev)
 {
        FBDEV *fr_dev=dev;

        fr_dev->fdfd=open("/dev/fb0",O_RDWR);
//        printf("the framebuffer device was opended successfully.\n");
        ioctl(fr_dev->fdfd,FBIOGET_FSCREENINFO,&(fr_dev->finfo)); //获取 固定参数
        ioctl(fr_dev->fdfd,FBIOGET_VSCREENINFO,&(fr_dev->vinfo)); //获取可变参数
        fr_dev->screensize=fr_dev->vinfo.xres*fr_dev->vinfo.yres*fr_dev->vinfo.bits_per_pixel/8;
        fr_dev->map_fb=(char *)mmap(NULL,fr_dev->screensize,PROT_READ|PROT_WRITE,MAP_SHARED,fr_dev->fdfd,0);
//        printf("init_dev successfully. fr_dev->map_fb=%p\n",fr_dev->map_fb);
  }

    void release_dev(FBDEV *dev)
    {
	if(!dev->map_fb)return;

	munmap(dev->map_fb,dev->screensize);
	close(dev->fdfd);
    }


  /*------------------------------------
  check if (px,py) in box(x1,y1,x2,y2)
  return true or false
  -------------------------------------*/
  bool point_inbox(int px,int py, int x1, int y1,int x2, int y2)
  {
      int xl,xh,yl,yh;

	if(x1>=x2){
		xh=x1;xl=x2;
	}
	else {
		xl=x1;xh=x2;
	}

	if(y1>=y2){
		yh=y1;yl=y2;
	}
	else {
		yh=y2;yl=y1;
	}

	if( (px>=xl && px<=xh) && (py>=yl && py<=yh))
		return true;
	else
		return false;
  }


    /*  set color for every dot */
    void fbset_color(uint16_t color)
    {
	fb_color=color;
    }

    /* clear screen with given color */
    void clear_screen(FBDEV *dev, uint16_t color)
    {
	FBDEV *fr_dev=dev;
	long int location=0;

	for(location=0; location < (fr_dev->screensize); location++)
	        *((unsigned short int *)(fr_dev->map_fb+location))=color;
    }


    /*-----------------------------------------------------

    Return:
	0	OK
	-1	get out of FB mem.(ifndef FB_DOTOUT_ROLLBACK)
    --------------------------------------------------------*/
    int draw_dot(FBDEV *dev,int x,int y) //(x.y) 是坐标
    {
        FBDEV *fr_dev=dev;
	int fx=x;
	int fy=y;
        long int location=0;
	int xres=fr_dev->vinfo.xres;
	int yres=fr_dev->vinfo.yres;

#ifdef FB_DOTOUT_ROLLBACK
	/* map to LCD(X,Y) */
	if(fx>xres-1)
		fx=fx%xres;
	else if(fx<0)
	{
		fx=xres-(-fx)%xres;
		fx=fx%xres; /* here fx=1-240 */
	}
	if( fy > yres-1) {
		fy=fy%yres;
	}
	else if(fy<0) {
		fy=yres-(-fy)%yres;
		fy=fy%yres; /* here fy=1-320 */
	}
#else /* NO ROLLBACK */
	/* map to LCD(X,Y) */
	if(fx>xres-1)
		fx=xres-1;
	else if(fx<0)
		fx=0;

	if( fy > yres-1) {
		fy=yres-1;
	}
	else if(fy<0) {
		fy=0;
	}

	/* check if no space left for a 16bit_pixel in FB mem */
	if( location<0 || location > (fr_dev->screensize-sizeof(uint16_t)) )
	{
		printf("WARNING: point location out of fb mem.!\n");
		return -1;
	}
#endif

        location=(fx+fr_dev->vinfo.xoffset)*(fr_dev->vinfo.bits_per_pixel/8)+
                     (fy+fr_dev->vinfo.yoffset)*fr_dev->finfo.line_length;

        *((unsigned short int *)(fr_dev->map_fb+location))=fb_color;

	return 0;
    }


#if 0   /*------ OBSOLETE, seems the same effect as above !! --------*/
    void draw_dot(FBDEV *dev,int x,int y) //(x.y) 是坐标
    {
        FBDEV *fr_dev=dev;
        int *xx=&x;
        int *yy=&y;
        long int location=0;

        location=(*xx+fr_dev->vinfo.xoffset)*(fr_dev->vinfo.bits_per_pixel/8)+
                     (*yy+fr_dev->vinfo.yoffset)*fr_dev->finfo.line_length;

        *((unsigned short int *)(fr_dev->map_fb+location))=fb_color;
    }
#endif



    void draw_line(FBDEV *dev,int x1,int y1,int x2,int y2) 
    {
        FBDEV *fr_dev=dev;
        int *xx1=&x1;
        int *yy1=&y1;
        int *xx2=&x2;
        int *yy2=&y2;

        int i=0;
        int j=0;
        int tekxx=*xx2-*xx1;
        int tekyy=*yy2-*yy1;



// draw_line(&fr_dev,240,318,240,246);

        //if((*xx2>=*xx1)&&(*yy2>=*yy1))
        if(*xx2>*xx1)
        {
            for(i=*xx1;i<=*xx2;i++)
            {
                j=(i-*xx1)*tekyy/tekxx+*yy1;
                draw_dot(fr_dev,i,j);
//		draw_dot(fr_dev,i+1,j);
//		draw_dot(fr_dev,i,j+1);
//		draw_dot(fr_dev,i+1,j+1);
            }
        }
	else if(*xx2 == *xx1)
	{
	   if(*yy2>=*yy1)
	   {
		for(i=*yy1;i<=*yy2;i++)
		   draw_dot(fr_dev,*xx1,i);
	    }
	    else //yy2<yy1
	   {
		for(i=*yy2;i<=*yy1;i++)
			draw_dot(fr_dev,*xx1,i);
	   }
	}
        else
        {
            //if(*xx2<*xx1)
            for(i=*xx2;i<=*xx1;i++)
            {
                j=(i-*xx2)*tekyy/tekxx+*yy2;
                draw_dot(fr_dev,i,j);
            }
        }


    }


    /*------------------------------------------------
	           draw an oval
    -------------------------------------------------*/
    void draw_oval(FBDEV *dev,int x,int y) //(x.y) 是坐标
    {
        FBDEV *fr_dev=dev;
        int *xx=&x;
        int *yy=&y;

	draw_line(fr_dev,*xx,*yy-3,*xx,*yy+3);
	draw_line(fr_dev,*xx-1,*yy-2,*xx-1,*yy+2);
	draw_line(fr_dev,*xx-2,*yy-1,*xx-2,*yy+1);
	draw_line(fr_dev,*xx+1,*yy-2,*xx+1,*yy+2);
	draw_line(fr_dev,*xx+2,*yy-2,*xx+2,*yy+2);
    }


    void draw_rect(FBDEV *dev,int x1,int y1,int x2,int y2)
    {
        FBDEV *fr_dev=dev;

	draw_line(fr_dev,x1,y1,x1,y2);
	draw_line(fr_dev,x1,y2,x2,y2);
	draw_line(fr_dev,x2,y2,x2,y1);
	draw_line(fr_dev,x2,y1,x1,y1);
    }


    /*----------------------------------------------------------
	Return:
		0	OK
		-1	point out of FB mem
    Midas
    ------------------------------------------------------------*/
    int draw_filled_rect(FBDEV *dev,int x1,int y1,int x2,int y2)
    {
	int xr,xl,yu,yd;
	int i,j;

        /* sort point coordinates */
        if(x1>x2)
        {
                xr=x1;
		xl=x2;
        }
        else
	{
                xr=x2;
		xl=x1;
	}

        if(y1>y2)
        {
                yu=y1;
		yd=y2;
        }
        else
	{
                yu=y2;
		yd=y1;
	}

	for(i=yd;i<yu;i++)
	{
		for(j=xl;j<xr;j++)
		{
                	if(draw_dot(dev,j,i)<0)
				return -1;
		}
	}

	return 0;
    }



    /*------------------------------------------------
    draw a circle,
	(x,y)	circle center
	r	radius
    Midas
    -------------------------------------------------*/
    void draw_circle(FBDEV *dev, int x, int y, int r)
    {
	int i;
	int s;

	for(i=0;i<r;i++)
	{
		s=sqrt(r*r-i*i);
		if(i==0)s-=1; /* erase four tips */
		draw_dot(dev,x-s,y+i);
		draw_dot(dev,x+s,y+i);
		draw_dot(dev,x-s,y-i);
		draw_dot(dev,x+s,y-i);
		/* flip X-Y */
		draw_dot(dev,x-i,y+s);
		draw_dot(dev,x+i,y+s);
		draw_dot(dev,x-i,y-s);
		draw_dot(dev,x+i,y-s);
	}
    }

    /*------------------------------------------------
    draw a filled circle
    Midas
    -------------------------------------------------*/
    void draw_filled_circle(FBDEV *dev, int x, int y, int r)
    {
	int i;
	int s;

	for(i=0;i<r;i++)
	{
		s=sqrt(r*r-i*i);
		if(i==0)s-=1; /* erase four tips */
		draw_line(dev,x-s,y+i,x+s,y+i);
		draw_line(dev,x-s,y-i,x+s,y-i);
	}

    }


   /*----------------------------------------------------------------------------
   copy a block of FB memory  to buffer
   x1,y1,x2,y2:	  LCD area corresponding to FB mem. block
   buf:		  data dest.

   Return
		1	partial area out of FB mem boundary
		0	OK
		-1	fails
   Midas
   ----------------------------------------------------------------------------*/
   int fb_cpyto_buf(FBDEV *fb_dev, int x1, int y1, int x2, int y2, uint16_t *buf)
   {
	int i,j;
	int xl,xr; /* left right */
	int yu,yd; /* up down */
	int ret=0;
	long int location=0;
	int xres=fb_dev->vinfo.xres;
	int yres=fb_dev->vinfo.yres;
	int tmpx,tmpy;

	/* check buf */
	if(buf==NULL)
	{
		printf("fb_cpyto_buf(): buf is NULL!\n");
		return -1;
	}

	/* sort point coordinates */
	if(x1>x2){
		xr=x1;
		xl=x2;
	}
	else{
		xr=x2;
		xl=x1;
	}
	if(y1>y2){
		yu=y1;
		yd=y2;
	}
	else{
		yu=y2;
		yd=y1;
	}


	/* ---------  copy mem --------- */
	for(i=yd;i<=yu;i++)
	{
        	for(j=xl;j<=xr;j++)
		{

#ifdef FB_DOTOUT_ROLLBACK /* -------------   ROLLBACK  ------------------*/
			/* map i,j to LCD(Y,X) */
			if(i<0) /* map Y */
			{
				tmpy=yres-(-i)%yres; /* here tmpy=1-320 */
				tmpy=tmpy%yres;
			}
			else if(i > yres-1)
				tmpy=i%yres;
			else
				tmpy=i;

			if(j<0) /* map X */
			{
				tmpx=xres-(-j)%xres; /* here tmpx=1-240 */ 
				tmpx=tmpx%xres;
			}
			else if(j > xres-1)
				tmpx=j%xres;
			else
				tmpx=j;

			location=(tmpx+fb_dev->vinfo.xoffset)*(fb_dev->vinfo.bits_per_pixel/8)+
        	        	     (tmpy+fb_dev->vinfo.yoffset)*fb_dev->finfo.line_length;


#else  /* -----------------  NO ROLLBACK  ------------------------*/
			if( i<0 || j<0 || i>yres-1 || j>xres-1 )
			{
				printf("WARNING: fb_cpyfrom_buf(): coordinates out of range!\n");
				ret=1;
			}
			/* map i,j to LCD(Y,X) */
			if(i>yres-1) /* map Y */
				tmpy=yres-1;
			else if(i<0)
				tmpy=0;
			else
				tmpy=i;

			if(j>xres-1) /* map X */
				tmpx=xres-1;
			else if(j<0)
				tmpx=0;
			else
				tmpx=j;

			location=(tmpx+fb_dev->vinfo.xoffset)*(fb_dev->vinfo.bits_per_pixel/8)+
        	        	     (tmpy+fb_dev->vinfo.yoffset)*fb_dev->finfo.line_length;

#endif
			/* copy to buf */
        		*buf = *((uint16_t *)(fb_dev->map_fb+location));
			 buf++;
		}
	}
//	printf(" fb_cpyfrom_buf = %d\n", ret);

	return ret;
   }



 /*----------------------------------------------------------------------------
   copy a block of buffer to FB memory.
   x1,y1,x2,y2:	  LCD area corresponding to FB mem. block
   buf:		  data source

   Return
		1	partial area out of FB mem boundary
		0	OK
		-1	fails
   Midas
   ----------------------------------------------------------------------------*/
   int fb_cpyfrom_buf(FBDEV *fb_dev, int x1, int y1, int x2, int y2, uint16_t *buf)
   {
	int i,j;
	int xl,xr; /* left right */
	int yu,yd; /* up down */
	int ret=0;
	long int location=0;
	int xres=fb_dev->vinfo.xres;
	int yres=fb_dev->vinfo.yres;
	int tmpx,tmpy;

	/* check buf */
	if(buf==NULL)
	{
		printf("fb_cpyfrom_buf(): buf is NULL!\n");
		return -1;
	}

	/* sort point coordinates */
	if(x1>x2){
		xr=x1;
		xl=x2;
	}
	else{
		xr=x2;
		xl=x1;
	}

	if(y1>y2){
		yu=y1;
		yd=y2;
	}
	else{
		yu=y2;
		yd=y1;
	}


	/* ------------ copy mem ------------*/
	for(i=yd;i<=yu;i++)
	{
        	for(j=xl;j<=xr;j++)
		{
#ifdef FB_DOTOUT_ROLLBACK /* -------------   ROLLBACK  ------------------*/
			/* map i,j to LCD(Y,X) */
			if(i<0) /* map Y */
			{
				tmpy=yres-(-i)%yres; /* here tmpy=1-320 */
				tmpy=tmpy%yres;
			}
			else if(i>yres-1)
				tmpy=i%yres;
			else
				tmpy=i;

			if(j<0) /* map X */
			{
				tmpx=xres-(-j)%xres; /* here tmpx=1-240 */
				tmpx=tmpx%xres;
			}
			else if(j>xres-1)
				tmpx=j%xres;
			else
				tmpx=j;


			location=(tmpx+fb_dev->vinfo.xoffset)*(fb_dev->vinfo.bits_per_pixel/8)+
        	        	     (tmpy+fb_dev->vinfo.yoffset)*fb_dev->finfo.line_length;

#else  /* -----------------  NO ROLLBACK  ------------------------*/
			if( i<0 || j<0 || i>yres-1 || j>xres-1 )
			{
				printf("WARNING: fb_cpyfrom_buf(): coordinates out of range!\n");
				ret=1;
			}
			/* map i,j to LCD(Y,X) */
			if(i>yres-1) /* map Y */
				tmpy=yres-1;
			else if(i<0)
				tmpy=0;
			else
				tmpy=i;

			if(j>xres-1) /* map X */
				tmpx=xres-1;
			else if(j<0)
				tmpx=0;
			else
				tmpx=j;

			location=(tmpx+fb_dev->vinfo.xoffset)*(fb_dev->vinfo.bits_per_pixel/8)+
        	        	     (tmpy+fb_dev->vinfo.yoffset)*fb_dev->finfo.line_length;
#endif
			/* --- copy to fb ---*/
        		*((uint16_t *)(fb_dev->map_fb+location))=*buf;
			buf++;
		}
	}

	return ret;
   }
