/*-----------------------  touch_home.c ------------------------------
1. Only first byte read from XPT is meaningful !!!??

2. Keep enough time-gap for XPT to prepare data in each read-session,
usleep 3000us seems OK, or the data will be too scattered.

3. Hold LCD-Touch module carefully, it may influence touch-read data.

4. TOUCH Y edge doesn't have good homogeneous property along X,
   try to draw a horizontal line and check visually, it will bend.

5. if sx0=sy0=0, it means that last TOUCH coordinate is invalid or pen-up.

6. Linear piont to piont mapping from TOUCH to LCD. There are
   unmapped points on LCD.

7. point to an oval area mapping form TOUCH to LCD.


TODO:
1. Screen sleep
2. LCD_SIZE_X,Y to be cancelled. 


Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include "spi.h"
#include "fblines.h"
#include "egi.h"
#include "xpt2046.h"
#include "bmpjpg.h"
#include "dict.h"

/*-------------------  MAIN  ---------------------*/
int main(void)
{
	int i,j,k;
	int index;
	/*-------------------------------------------------
	 native XPT touch pad coordinate value
	!!!!!!!! WARNING: use xp[0] and yp[0] only !!!!!
	---------------------------------------------------*/
	uint8_t  xp[2]; /* when untoched: Xp[0]=0, Xp[1]=0 */
	uint8_t  yp[2]; /* untoched: Yp[0]=127=[2b0111,1111] ,Yp[1]=248=[2b1111,1100]  */
	uint8_t	 xyp_samples[2][10]; /* store 10 samples of x,y */
	int xp_accum; /* accumulator of xp */
	int yp_accum; /* accumulator of yp */
	int nsample=0; /* number of samples for each touch-read session */

	/* LCD coordinate value */
	uint16_t sx,sy;  //current TOUCH point coordinate, it's a LCD screen coordinates derived from TOUCH coordinate.
	uint16_t sx0=0,sy0=0;//saved last TOUCH point coordinate, for comparison.

	//uint8_t cmd;
        FBDEV     fr_dev;


	/* buttons array param */
	int nrow=2; /* number of buttons at Y directions */
	int ncolumn=3; /* number of buttons at X directions */
	struct egi_element_box ebox[nrow*ncolumn]; /* square boxes for buttons */
	int startX=0;  /* start X of eboxes array */
	int startY=120; /* start Y of eboxes array */
	int sbox=70; /* side length of the square box */
	int sgap=(LCD_SIZE_X - ncolumn*sbox)/(ncolumn+1); /* gaps between boxes(bottons) */


	char str_syscmd[100];


	/* open spi dev */
	SPI_Open();
	/* prepare fb device */
        fr_dev.fdfd=-1;
        init_dev(&fr_dev);
	/* clear the screen with BLACK*/
	clear_screen(&fr_dev,(0|0|0));

	/* --- load screen paper --- */
	show_jpg("home.jpg",&fr_dev,0,0);

	/* --- load symbol dict --- */
	//dict_display_img(&fr_dev,"dict.img");

	if(dict_load_h20w15("/home/dict.img")==NULL)
		exit(-1);

	dict_print_symb20x15(dict_h20w15);
	for(i=0;i<10;i++)
		dict_display_symb20x15(1,&fr_dev, (uint16_t *)(dict_h20w15+i*15*20), 30+i*15, 320-40);

//	exit(1);

	/* ----- generate eboxe parameters ----- */
	for(i=0;i<nrow;i++) /* row */
	{
		for(j=0;j<ncolumn;j++) /* column */
		{
			/* generate boxes */
			ebox[ncolumn*i+j].height=sbox;
			ebox[ncolumn*i+j].width=sbox;
			ebox[ncolumn*i+j].x0=startX+(j+1)*sgap+j*sbox;
			ebox[ncolumn*i+j].y0=startY+i*(sgap+sbox);
		}
	}
	/* print box position for debug */
	// for(i=0;i<nrow*ncolumn;i++)
	//	printf("ebox[%d]: x0=%d, y0=%d\n",i,ebox[i].x0,ebox[i].y0);
	/* ----- draw the boxes ----- */
	for(i=0;i<nrow*ncolumn;i++)
	{
		fbset_color( (30-i*5)<<11 | (50-i*8)<<5 | (i+1)*10 );/* R5-G6-B5 */

		draw_filled_rect(&fr_dev,ebox[i].x0,ebox[i].y0,\
			ebox[i].x0+ebox[i].width,ebox[i].y0+ebox[i].height);
	}



        fbset_color((30<<11)|(10<<5)|10);/* R5-G6-B5 */

	/* ---------------- MAIN LOOP -----------------------*/
	while(1)
	{
		/*------ relate with number of touch-read samples -----*/
		usleep(3000); //3000

		/*--------- read XPT to get touch-pad coordinate --------*/
		if( xpt_read_xy(xp,yp)!=0 ) /* if read XPT fails,break or pen-up */
		{
			/* reset sx0,sy0 if there is a break or pen-up */
			sx0=0;sy0=0;
			/* reset nsample and accumulator then */
			nsample=0;
			xp_accum=0;yp_accum=0;
			continue;
		}
		else
		{
//			xyp_samples[0][nsample]=xp;
//			xyp_samples[1][nsample]=yp;
			xp_accum += xp[0];
			yp_accum += yp[0];
			nsample++;
		}
		if(nsample<XPT_SAMPLE_NUMBER)continue; /* if not get enough samples */

		/* average of accumulated value */
		xp[0]=xp_accum>>XPT_SAMPLE_EXPNUM; /* shift exponent number of 2 */
		yp[0]=yp_accum>>XPT_SAMPLE_EXPNUM;
		/* reset nsample and accumulator then */
		nsample=0;
		xp_accum=0;yp_accum=0;

		/*----- convert to LCD coordinate sx,sy ------*/
	    	xpt_maplcd_xy(xp, yp, &sx, &sy);
	    	printf("xp=%d, yp=%d;  sx=%d, sy=%d\n",xp[0],yp[0],sx,sy);

	    	/*  if sx0,sy0 is set to 0, then store new data */
	    	if(sx0==0 || sy0==0){
			sx0=sx;
			sy0=sy;
	    	}

		/*---  get index of pressed ebox ----*/
	    	index=egi_get_boxindex(sx,sy,ebox,nrow*ncolumn);
		if(index>=0) /* if get meaningful index */
		{
			printf("button[%d] pressed!\n",index);
			sprintf(str_syscmd,"jpgshow m_%d.jpg",index+1);
			system(str_syscmd);
			//usleep(200000); //this will make touch points scattered.
		}



#if 0
	    /* ignore uncontinous points */
	    // too small value will cause more breaks and gaps
	    if( abs(sx-sx0)>3 || abs(sy-sy0)>3 ) {
		  /* reset sx0,sy0 */
		  sx0=0;sy0=0;
  		  continue;
	    }

	    /* ignore repeated points */
	    if(sx0==sx && sy0==sy)continue;
#endif

	    /* ------------- draw points to LCD -------------*/
#if 0
	    /*  ---Method 1. draw point---  */
//	    printf("sx=%d, sy=%d\n",sx,sy);
	    /*  --- valid range of sx,sy ---- */
	    if(sx<1+1 || sx>240-1 || sy<1+1 || sy>320-1)continue;

	    /*  ---Method 2. draw oval---  */
	    draw_oval(&fr_dev,sx,sy);

	    /*  ---Method 3. draw rectangle---  */
//    	    draw_rect(&fr_dev,sx-3,sy-1,sx,sy+2); //2*4 rect

	    /*  draw line */
//	    draw_line(&fr_dev,sx0,sy0,sx,sy);

#endif
	    /* update sx0,sy0 */
	    sx0=sx;sy0=sy;
	}

	/* release dict data */
	dict_release_h20w15();

	/* close fb dev */
        munmap(fr_dev.map_fb,fr_dev.screensize);
        close(fr_dev.fdfd);

	/* close spi dev */
	SPI_Close();
	return 0;
}
