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
8. system() may never return and cause the process dead.
9. stop timer before call execl(), or it the process will get 'Alarm clock' error
   then abort.

TODO:
0. Cancel extern vars in all head file, add xx_get_xxVAR() functions.
1. Screen sleep
2. LCD_SIZE_X,Y to be cancelled. use FB parameter instead.
3. home page refresh button... 3s pressing test.
4. apply mutli-process for separative jobs: reading-touch-pad, CLOCK,texting,... etc.
5. systme()... sh -c ...
6. btn-press and btn-release signal
7. To read FBDE vinfo to get all screen/fb parameters as in fblines.c, it's improper in other source files.
8. after set tm_timer(), SIGALM will disfunction sleep() and usleep(), try tm_delayms()...

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include "egi_color.h"
#include "spi.h"
#include "egi_fbgeom.h"
#include "egi.h"
#include "egi_math.h"
#include "egi_page.h"
#include "egi_txt.h"
#include "egi_btn.h"
#include "egi_pic.h"
#include "egi_timer.h"
#include "egi_objtxt.h"
#include "egi_pagehome.h"
//#include "egi_pagemplay.h"
#include "xpt2046.h"
#include "egi_bjp.h"
//#include "dict.h"
#include "egi_symbol.h"
#include "egi_objlist.h"
#include "egi_iwinfo.h"
#include "egi_touch.h"
#include "egi_log.h"

char mvicon_load[16]={0};

void roam_pic1(void)
{
	pthread_detach(pthread_self());

        /* roaming 100 trips, step len=3, offset orign (40,60), in a 160x80 widonw */
        egi_roampic_inwin("/tmp/lights.jpg", &gv_fb_dev, 5, 1000000, 60, 40, 160, 80);

	pthread_exit(0);
}


void roam_pic2(void)
{
	pthread_detach(pthread_self());

        /* roaming 100 trips, step len=3, offset orign (40,60), in a 160x80 widonw */
        egi_roampic_inwin("/tmp/dance.jpg", &gv_fb_dev, 2, 1000000, 30, 180, 200, 100);

	pthread_exit(0);
}

void test_log1(void)
{
	int i;
	pthread_detach(pthread_self());
	for(i=0;i<10;i++)
	{
		EGI_PLOG(LOGLV_INFO," From file %s,  %s(): LOGLV_INFO---%d--- \n",__FILE__,__FUNCTION__,i);
	}
	pthread_exit(0);
}
void test_log2(void)
{
	int i;
	pthread_detach(pthread_self());
	for(i=0;i<10;i++)
	{
		EGI_PLOG(LOGLV_ERROR,"%s(): LOGLV_ERROR---%d--- \n",__FUNCTION__,i);
//		exit(0);
	}
	pthread_exit(0);
}
void test_log3(void)
{
	int i;
	pthread_detach(pthread_self());
	for(i=0;i<10;i++)
	{
		EGI_PLOG(LOGLV_INFO,"log3: LOGLV_INFO---- %d ---- \n",i);
	}
	pthread_exit(0);
}


/*  ---------------------  MAIN  ---------------------  */
int main(int argc, char **argv)
{
	int i,j;
	int index;
	int ret;
	uint16_t sx,sy;  //current TOUCH point coordinate, it's a LCD screen coordinates derived from TOUCH coordinate.

	int delt=5; /* incremental value*/
	//char str_syscmd[100];
	//char strf[100];
	int disk_on=0;
	int radio_on=0;
	int kr=0;

	uint16_t *buf;
//	buf=(uint16_t *)malloc(320*240*sizeof(uint16_t));

	uint16_t *buf2;
//	buf2=(uint16_t *)malloc(320*240*sizeof(uint16_t));

	uint16_t *nbuf;
//	nbuf=(uint16_t *)malloc(320*240*sizeof(uint16_t));

	pthread_t   thread_loopread;



	/* --- start egi tick --- */
	tm_start_egitick();
//	tm_tick_settimer(TM_TICK_INTERVAL);/* set global tick timer */
//	signal(SIGALRM, tm_tick_sigroutine);


	/* test random max */
#if 0
	while(1)
	{
		printf("egi_random_max(5):%d\n",egi_random_max(5));
		printf("egi_random_max(-5):%d\n",egi_random_max(-5));
		tm_delayms(100);
	}
	exit(0);
#endif


	/* --- init log --- */
	if(egi_init_log("/mmc/egi_log") !=0)
	{
	   printf("egi_init_log() fails! \n");
	   exit(0);
	}

	/* --- open spi dev --- */
	SPI_Open();

	/* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);

	//clear_screen(&gv_fb_dev, 0) ;
	//exit(0);


	/* ---- OBSELTE!!!! Clash with tm_tick_settimer() !  set timer for time display ---- */
	//tm_settimer(500000);/* set timer interval interval */
	//signal(SIGALRM, tm_sigroutine);


	/* ---- start touch thread ---- */
	if( pthread_create(&thread_loopread, NULL, (void *)egi_touch_loopread, NULL) !=0 )
	{
		printf(" pthread_create(... egi_touch_loopread() ... ) fails!\n");
		exit(1);
	}

/*	mat_create_fptrigontab();
	//printf(" -3%%100=%d \n",(-3)%100);
	//printf(" -203%%100=%d \n",(-203)%100);
 for(i=0;i<20;i++)
 {
	printf("egi_random_max(10)=%d \n",egi_random_max(10));
	printf("egi_random_max(-10)=%d \n",egi_random_max(-10));
 }
*/
///////////////////////////////////////////////////////////////////////////////
	EGI_EBOX  *hitbtn;

	/* ------  create txt type ebox objects -------*/
	/* note */
	EGI_EBOX *ebox_note = create_ebox_note();
	if(ebox_note == NULL)return -1;
	EGI_DATA_TXT *note_txt=(EGI_DATA_TXT *)(ebox_note->egi_data);
	/* txt */
	EGI_EBOX *ebox_clock = create_ebox_clock();
	if(ebox_clock == NULL)return -2;
	EGI_DATA_TXT *clock_txt=(EGI_DATA_TXT *)(ebox_clock->egi_data);
	/* memo */
	EGI_EBOX *ebox_memo=create_ebox_memo();
	if(ebox_memo == NULL)return -3;


	/* ------------   home_button eboxes definition  ------------------ */

	EGI_PAGE *page_home=NULL;
	EGI_PAGE *page_mplay=NULL;
	EGI_PAGE *page_openwrt=NULL;


  /* test -------- imgbuf funcs-------- */
#if 0
	show_jpg("/tmp/home.jpg",&gv_fb_dev,0,0,0); /*black on*/

	pthread_t thread_roampic1;
	pthread_t thread_roampic2;

	/* ---- start roaming threads ---- */
	if( pthread_create(&thread_roampic1, NULL, (void *)roam_pic1, NULL) !=0 )
	{
		printf(" pthread_create(... thread_roampic1() ... ) fails!\n");
		exit(1);
	}

	if( pthread_create(&thread_roampic2, NULL, (void *)roam_pic2, NULL) !=0 )
	{
		printf(" pthread_create(... thread_roampic2() ... ) fails!\n");
		exit(1);
	}

	/* NO need for detached thread, wait thread */
//	pthread_join(thread_roampic1,NULL);
//	pthread_join(thread_roampic2,NULL);
	exit(1);
#endif


#if 0
	/* test ------- touch loopread ------- */
	int dy;
	int mark=280; /* */
	int hvol=mark;
	int setvol;
	int strcmd[50];

	EGI_TOUCH_DATA touch_data;

	show_jpg("/tmp/lights.jpg",&gv_fb_dev,0,0,0); /*black on*/
	fb_cpyto_buf(&gv_fb_dev, 50, 0, 50+50-1, 320-1, buf);

	while(1)
    	{
		if( !egi_touch_getdata(&touch_data) )
		{
			//printf("touch data NOT ready!\n");
			//usleep(2000);
			tm_delayms(10);

		}
		else
		{
			/* reset mark when releasing */
			if( touch_data.status==releasing )
			{
				printf("------- touch_data: releaseing -------\n");
				mark=hvol;
				continue;
			}

			dy=touch_data.dely;

			if(dy>0)
				hvol=mark+dy/4*4;
			else if(dy<0)
				hvol=mark+dy/4*4;
			/* set limit */
			if(hvol<20)hvol=30;
			if(hvol>280)hvol=280;
			printf("dy=%d, mark=%d, hvol=%d\n",dy,mark,hvol);

			/* PCM 0-999 */
			 setvol=(280-hvol)*1000/(280-30);
			sprintf(strcmd,"amixer set PCM %d",setvol);
			system(strcmd);

			fb_cpyfrom_buf(&gv_fb_dev, 50, 0, 50+50-1, 320-1, buf);
			draw_filled_rect(&gv_fb_dev,50,280,50+50-1,hvol);

			tm_delayms(55);
		}
    	}
#endif

	/* test ---- IW RSSI ------ */
#if 0
	while(1)
	{
		//tm_delayms(1000) will crash get_iw_rssi() !!!?
//		usleep(900000);
		get_iw_rssi();
	}
#endif

#if 0 /* test ----- egi txtbox read file ---------- */
	 ret=egi_txtbox_readfile(ebox_memo, "/tmp/memo.txt");
	 printf("ret=egi_txtbox_readfile()=%d\n",ret);
	 exit(1);
#endif


	/* --- clear screen with BLACK --- */
#if 0
	//fbset_color(COLOR_RGB_TO16BITS(0X44,0x44,0X88));
	//(r>>3)<<11 | (g>>2)<<5 | b>>3
//	uint16_t color= ( (0xAA>>3)<<11 | (0xAA>>2)<<5 | 0xAA>>3 );
	uint16_t color = egi_colorgray_random(light);
	printf("color=0x%04X \n",color);
	fbset_color(color);
	draw_filled_rect(&gv_fb_dev,0,0,240-1,320-1);
	tm_delayms(1000);
	clear_screen(&gv_fb_dev, 0xAD55) ;
	tm_delayms(1000);
#endif

	/* --- load screen paper --- */
	show_jpg("home.jpg",&gv_fb_dev,0,0,0); /*black on*/


	/* test: ----- image scale,page dispearing effect ---- */
#if 0
	int wid,hgt;
	printf("start fb_cpyto_buf\n");
	/* 1. grap page image */
       	fb_cpyto_buf(&gv_fb_dev, 0, 0, 240-1, 320-1, buf);
	printf("start for...\n");
	/* 2. zoom out to left top point */
	for(wid=240;wid>1;wid-=1)
	{
		hgt=wid*4/3;
		fb_scale_pixbuf(240,320,wid,hgt,buf,nbuf);
		/* restore previous page image */

		/* put scaled  dispearing image */
		fb_cpyfrom_buf(&gv_fb_dev,0,0,wid-1,hgt-1,nbuf);
		usleep(100000);
	}
	/* 3. */

	exit(1);
#endif


	/* test: --------- image rotate ----------- */
#if 0
        /* copy fb image to buf */
	int centx=120;
	int centy=140;
	int sq=237;
	int sq2=105;
        fb_cpyto_buf(&gv_fb_dev, centx-sq/2, centy-sq/2, centx+sq/2, centy+sq/2, buf); /* buf */
        fb_cpyto_buf(&gv_fb_dev, centx-sq2/2, centy-sq2/2, centx+sq2/2, centy+sq2/2, buf2); /* buf2 */

	/* for image rotation */
	struct egi_point_coord	*SQMat; /* the map matrix, */
	SQMat=malloc(sq*sq*sizeof(struct egi_point_coord));
	memset(SQMat,0,sizeof(*SQMat));

	struct egi_point_coord	*SQMat2; /* the map matrix,  */
	SQMat2=malloc(sq2*sq2*sizeof(struct egi_point_coord));
	memset(SQMat2,0,sizeof(*SQMat2));

	struct egi_point_coord  centxy={centx,centy}; /* center of rotation */
	struct egi_point_coord  x0y0={centx-sq/2,centy-sq/2};
	struct egi_point_coord  x0y02={centx-sq2/2,centy-sq2/2};

	i=0;
	j=0;
	int sign=1;
	while(1)
	{
		i += 2*sign;
		j += 8*sign;
		printf("%d%%360=%d\n",i,i%360);
/* // reverse after one circle
		if(i%360==0)
		{
			sign=-sign;
			printf("sign=-sign=%d\n",sign);
		}
*/
		/* --- get square rotation map ---- */
  	        // float point:	mat_pointrotate_SQMap(101, 2*i, centxy, SQMat);/* side,angle,center, map matrix */
		//  mat_pointrotate_fpSQMap(sq, i, centxy, SQMat);/* side,angle,center, map matrix */

		/* --- get  annulus rotaton map --- */
		mat_pointrotate_fpAnnulusMap(sq, 105, i, centxy, SQMat); /* ANMat = SQMat */
		mat_pointrotate_fpAnnulusMap(sq2, 1, -j, centxy, SQMat2); /* ANMat = SQMat */

		/* draw rotated square image */
		// fb_drawimg_SQMap(sq, x0y0, buf, SQMat); /* square:  side,center,image buf, map matrix */

		/* draw rotated annulus image */
		fb_drawimg_ANMap(sq, 105, x0y0, buf, SQMat); /* annulus: ANMat = SQMat */
		fb_drawimg_ANMap(sq2, 1, x0y02, buf2, SQMat2); /* annulus: ANMat = SQMat */

		tm_delayms(55);
	}
	exit(1);
#endif



	/* --- print and display symbols --- */
#if 0
	for(i=0;i<10;i++)
		dict_writeFB_symb20x15(&gv_fb_dev,1,(30<<11|45<<5|10),i,30+i*15,320-40);
#endif

	if(symbol_load_allpages() !=0 ) exit(-2);
#if 0
	/*------------------ Load Symbols ------------------*/
	/* load testfont */
	if(symbol_load_page(&sympg_testfont)==NULL)
		exit(-2);
	/* load numbfont */
	if(symbol_load_page(&sympg_numbfont)==NULL)
		exit(-2);
	/* load buttons icons */
	if(symbol_load_page(&sympg_buttons)==NULL)
		exit(-2);
	/* load icons for home head-bar*/
	if(symbol_load_page(&sympg_icons)==NULL)
		exit(-2);
	/* load icons for PLAYERs */
	if(symbol_load_page(&sympg_icons_2)==NULL)
		exit(-2);
#endif

	/* --------- test:  print all symbols in the page --------*/
#if 0
	for(i=32;i<127;i++)
	{
		symbol_print_symbol(&sympg_testfont,i,0xffff);
		//getchar();
	}
#endif
#if 0
	for(i=48;i<58;i++)
		symbol_print_symbol(&sympg_numbfont,i,0x0);
#endif

#if 0
        /* BDEV *fb_dev, sym_page, fontcolor, transpcolo, x0, y0, sym_code */
	for(i=1;i<5;i++)
	        symbol_writeFB(&gv_fb_dev,&sympg_icons, -1, 0 , 30*i, 0, i);
	exit(1);
#endif


#if 0 /* test  ----------------- egi_pic --------------- */

	/* -------- to search jpg files ------ */
	char path[]="/mmc/photos";
	int maxflen=100; /* max jpg file name length */
	int maxfnum=20; /* max number of items in fpaths */
	char **fpaths=NULL; /* jpg file paths */
	fpaths=malloc(maxfnum*sizeof(char *));
	memset(fpaths,0,maxfnum*sizeof(char *));
	for(i=0;i<maxfnum;i++)
	{
		fpaths[i]=malloc(maxflen*sizeof(char));
		memset(fpaths[i], 0, maxflen*sizeof(char));
	}
	int count=0;
	/* search all jpg files in a path */
	egi_find_jpgfiles(path, &count, fpaths, maxfnum, maxflen);
	printf("Totally %d jpg files are found.\n",count);

	/* ------- display jpg files ------- */
	int pich=130;
	int picw=230;
	int poffx=4;
	int poffy=4;
	int symh=26;
	EGI_DATA_PIC *data_pic=NULL;
	EGI_EBOX *pic_box=NULL;

	EGI_IMGBUF imgbuf;
	uint16_t  *picbuf=NULL;

	EGI_POINT  pxy;
	/* set x0y0 of pic in box */
	EGI_BOX	 box={ {0,0},{240-230-1,320-130/2-1} };
	//EGI_BOX  box={ {0-100,0-150},{240-1-100,320-1-150} };
	//EGI_BOX  box={ {0,0},{240-picw-poffx-1,320-pich-poffy-symh-1}};


for(i=0;i<count+1;i++)
{
	if(i==count)
	{
	   	i=0;
		/* re_search all jpg files in a path */
		egi_find_jpgfiles(path, &count, fpaths, maxfnum, maxflen);
		printf("Totally %d jpg files are found.\n",count);
		continue;
	}

	/* allocate data_pic */
        data_pic= egi_picdata_new( poffx, poffy,    /* int offx, int offy */
 	               		 pich, picw, /* heigth,width of displaying window */
                       		 0,0,	   /* int imgpx, int imgpy */
 	       	                 &sympg_testfont  /* struct symbol_page *font */
	                        );
	/* set title */
	data_pic->title="Happy New Year 2019!";

	/* get a random point */
	egi_randp_inbox(&pxy, &box);
	pic_box=egi_picbox_new( "pic_box", /* char *tag, or NULL to ignore */
       				  data_pic,  /* EGI_DATA_PIC *egi_data */
			          1,	     /* bool movable */
			   	  pxy.x,pxy.y,//10,100,    /*  x0, y0 for host ebox*/
        			  1,	     /* int frame */
				  WEGI_COLOR_GRAY /* int prmcolor,applys only if prmcolor>=0  */
	);
	//printf("egi_picbox_activate()...\n");
	egi_picbox_activate(pic_box);

	//system("date");
	printf("egi_imgbuf_loadjpg(): %s...\n", fpaths[i]);
	/* load jpg file to buf */
	if(egi_imgbuf_loadjpg(fpaths[i], &imgbuf) != 0 )
	   exit(-1);

	/* scale pic to data_pic */
	printf("start egi_scale_pixbuf()...\n");
	fb_scale_pixbuf(imgbuf.width, imgbuf.height, data_pic->imgbuf->width, data_pic->imgbuf->height,
				imgbuf.imgbuf, data_pic->imgbuf->imgbuf);

	/* release useless imgbuf then */
	egi_imgbuf_release(&imgbuf);

 for(j=0;j<5;j++)
 {
	/* get a random point */
	egi_randp_inbox(&pxy, &box);
	pic_box->x0=pxy.x;
	pic_box->y0=pxy.y;

	/* refresh picbox to show the picture */
	printf("start egi_picbox_refresh()...\n");
	egi_ebox_needrefresh(pic_box);
	//printf("egi_picbox_refresh()...\n");
	egi_picbox_refresh(pic_box);
	tm_delayms(1000);
  }

	/* sleep to erase the image */
	egi_picbox_sleep(pic_box);

	/* release it */
	pic_box->free(pic_box);
}

	/* free fpaths */
	for(i=0;i<maxfnum;i++)
	{
		free(fpaths[i]);
	}
	free(fpaths);

	exit(1);

#endif /* end of test: egi_pic */


#if 0 /* test -----  egi_display_msgbox() and egi_push_datatxt( ) ----- */
	int  nl;
	char *msg="test egi_push_datatxt() \
	\n	...\
	\n		...\
	\n			...\
	\n	\
	-----hello--- \
	\n	\
	\n	\
	-----hello--- \
	\n	\
	\n	\
	-----hello---";

	EGI_EBOX *pvmsgbox=NULL;
	char *pvmsg="Generating matrix, please wait.";
	int pv=0;

while(1)
{
    pvmsgbox=egi_msgbox_create(pvmsg, -1, WEGI_COLOR_ORANGE);
    if(pvmsgbox==NULL)exit(-1);

    pv=0;
    while(1)
    {
//	egi_msgbox_create(msg, 55, WEGI_COLOR_ORANGE);
	tm_delayms(55);
	pv +=20;
	if(pv==100)break;
	egi_msgbox_pvupdate(pvmsgbox,pv);
    }
    egi_msgbox_destroy(pvmsgbox);
    pvmsgbox=NULL;
}

	exit(1);
#endif


#if 0
	/* ------------------ test list --------------------- */
	EGI_EBOX *ebox;
	enum egi_touch_status status;
		egi_listbox_test(ebox, status);
		//tm_delayms(100);
		//clear_screen(&gv_fb_dev,0);
		//tm_delayms(50);
		//show_jpg("home.jpg",&gv_fb_dev,0,0,0); /*black on*/
	exit(1);
#endif


	/* ----------- activate pages and its listed eboxes ---------*/
	//egi_page_activate(page_home);


	/* txt ebox demon */
#if 0
	egi_txtbox_demo();
	exit(1);
#endif


	/* ----- set default color ----- */
        fbset_color((30<<11)|(10<<5)|10);/* R5-G6-B5 */

	/* ----- test:  home page ----- */
        page_home=egi_create_homepage();
	egi_page_activate(page_home);
	page_home->routine(page_home); /* get into routine loop */
	printf("------- Eixt home page --------\n");


#if 0 /* if loop */
/* ===============-------------(((  MAIN LOOP  )))-------------================= */
	while(1)
	{
		/*------ relate with number of touch-read samples -----*/
		//usleep(6000); //3000
		//printf(" while() loop start....\n");
		tm_delayms(2);

		/* page home */
	        page_home=egi_create_homepage();
		egi_page_activate(page_home);
		//egi_page_refresh(page_home);
		tm_delayms(1000);

#if 1
		/* page mplayer */
		page_mplay=egi_create_mplaypage();
		egi_page_activate(page_mplay);
		//egi_page_refresh(page_mplay);
		tm_delayms(1000);
#endif
		/* page openwrt system */
		page_openwrt=egi_create_openwrtpage();
		egi_page_activate(page_openwrt);
		//egi_page_refresh(page_openwrt);
		tm_delayms(1000);

		/* free all pages */
		egi_page_free(page_home);
		egi_page_free(page_mplay);
		egi_page_free(page_openwrt);

		continue;

		/*--------- read XPT to get avg tft-LCD coordinate --------*/
		//printf("start xpt_getavt_xy() \n");
		ret=xpt_getavg_xy(&sx,&sy); /* if fail to get touched tft-LCD xy */
		if(ret == XPT_READ_STATUS_GOING )
		{
			//printf("XPT READ STATUS GOING ON....\n");
			continue; /* continue to loop to finish reading touch data */
		}

		/* -------  put PEN-UP status events here !!!! ------- */
		else if(ret == XPT_READ_STATUS_PENUP )
		{
		       /* continous test ebox here */
		//	printf("start egi_txtbox_demo() ... \n");
			//egi_txtbox_demo();
			//continue;

		  /*  Heavy load task MUST NOT put here ??? */
			/* get hour-min-sec and display */
			tm_get_strtime(tm_strbuf);

/* TODO:  use tickcount to (tickcount%N) set a routine jobs is NOT reliable !!!! the condition may NEVER
	  appear if CPU is too busy to handle other things.
*/
			/* refresh timer NOTE eboxe according to tick */
			//if( tm_get_tickcount()%100 == 0 ) /* 30*TM_TICK_INTERVAL(2ms) */
			if(tm_pulseus(500000))
			{
				//printf("tick = %lld\n",tm_get_tickcount());
				if(ebox_note->x0 <=60  ) delt=10;
				if(ebox_note->x0 >=300 ) delt=-10;
				ebox_note->x0 += delt; //85 - (320-60)
				ebox_note->refresh(ebox_note);
			}
			/* refresh MEMO eboxe according to tick */
#if 1

			//if( tm_get_tickcount()%400 == 0 ) /* 1000*TM_TICK_INTERVAL(2ms) */
			if(tm_pulseus(800000))
			{
				//printf("tm pulseus!\n");
				//ebox_memo->y0 += 3;
				//ebox_memo->refresh(ebox_memo);
			}
#endif
			/* -----ONLY if tm changes, update txt and clock */
			if( strcmp(note_txt->txt[1],tm_strbuf) !=0 )
			{
				// printf("time:%s\n",tm_strbuf);
				/* update NOTE ebox txt  */
				strncpy(note_txt->txt[1],tm_strbuf,10);
				/* -----refresh CLOCK ebox---- */
				//wirteFB_str20x15(&gv_fb_dev, 1, (30<<11|45<<5|10), tm_strbuf, 60, 320-38);
				strncpy(clock_txt->txt[1],tm_strbuf,10);
				//clock_txt->color += (6<<8 | 4<<5 | 2 );
				ebox_clock->prmcolor = egi_color_random(all);
				ebox_clock->refresh(ebox_clock);
			}

			/* get year-mon-day and display */
			tm_get_strday(tm_strbuf);
//			symbol_string_writeFB(&gv_fb_dev, &sympg_testfont,WEGI_COLOR_SPRINGGREEN,
//					SYM_FONT_DEFAULT_TRANSPCOLOR,32,90,tm_strbuf);//(32,90,12,2)
			/* copy to note_txt */
			strncpy(note_txt->txt[0],tm_strbuf,22);

			/* ----------- test rotation map------------- */
		    	if(disk_on)
		    	{
				kr++;
				/* get rotation map */
				mat_pointrotate_SQMap(sq, 5*kr, centxy, SQMat);/* side,angle,center, map matrix */
				/* draw rotated image */
				fb_drawimg_SQMap(sq, x0y0, buf, SQMat); /* side,center,image buf, map matrix */
				//tm_delayms(5000);
		    	}

			continue; /* continue to loop to read touch data */
		}


		else if(ret == XPT_READ_STATUS_COMPLETE)
		{
			//printf("--- XPT_READ_STATUS_COMPLETE ---\n");
			/* going on then to check and activate pressed button */
		}

	/* -----------------------  Touch Event Handling  --------------------------*/

		hitbtn=egi_hit_pagebox(sx, sy, page_home );
		if(hitbtn != NULL)
			printf("a page ebox '%s' is hit.\n",hitbtn->tag);
		continue;

		/*---  get index of pressed ebox and activate the button ----*/
	    	//index=egi_get_boxindex(sx,sy,ebox_buttons[0],9);
		//printf("get touched box index=%d\n",index);



#if 1
		if(index>=0) /* if get valid index */
		{
			printf("button[%d] pressed!\n",index);
			switch(index)
			{
				case 0: /* use */
					printf("refresh fb now.\n");
					//system("killall mplayer");
					//system("/tmp/tmp_app");
					setitimer(0,0,NULL);
					execl("/tmp/tmp_app","tmp_app"," ",NULL);
					//exit(1);
					break;
				case 1:
					/* test dispear */
					egi_page_dispear(ebox_memo);
					break;
				case 2:
					egi_txtbox_demo();
					break;
				case 3:
					break;
				case 4: /*--------ON/OFF:  disk play ---------*/
					if(disk_on)
					{
						disk_on=0;
						printf("disk off.\n");
						//system("killall mplayer");
					}
					else
					{
						disk_on=1;
						printf("disk on. ----- \n");
						//system("killall mplayer");
						//system("mplayer /tmp/year.mp3 >/dev/null 2>&1 &");
					}
					printf("case 4: start tm_delayms(300)\n");
					tm_delayms(300); /* jitting */
					break;
				case 5: /*-------ON/OFF:  memo txt display -------*/
					if(ebox_memo->status!=status_active)
					{
						printf("BEFORE: egi_txtbox_activate(&ebox_memo)\n");
						ebox_memo->activate(ebox_memo);
					}
					else if(ebox_memo->status==status_active)
						ebox_memo->sleep(ebox_memo);

					tm_delayms(200);
					break;
				case 6: break;
				case 7: /*------  memo txt refresh -------*/
					//egi_txtbox_refresh(&ebox_memo);
					ebox_memo->refresh(ebox_memo);
					break;
				case 8:
					if(radio_on)
					{
						radio_on=0;
						system("killall mplayer");
					}
					else
					{
						radio_on=1;
						system("/home/radio.sh");
					}
					tm_delayms(500);
					break;
			}/* switch */

			//usleep(200000); //this will make touch points scattered.
		}/* end of if(index>=0) */
#endif
	} /* end of while() loop */

#endif /*end loop if */


	/* relese egi objects */
	egi_page_free(page_home);
	egi_page_free(page_mplay);

	/* release symbol mem page */
	symbol_free_allpages();

	/* close fb dev */
//      munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
//      close(gv_fb_dev.fdfd);
	release_fbdev(&gv_fb_dev);

	/* close spi dev */
	SPI_Close();

	/* quit log system,flush log_buff */
	egi_quit_log();

	return 0;
}
