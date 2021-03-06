/*---------------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License version 2 as published by the Free Software
Foundation.

Midas Zhou
midaszhou@yahoo.com
----------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <freetype2/ft2build.h>
#include <freetype2/ftglyph.h>
#include "egi_common.h"
#include "egi_utils.h"
#include "egi_FTsymbol.h"
#include "egi_cstring.h"

int main( int  argc,   char**  argv )
{
  int 		i,j,k;

  wchar_t *wbook=L"assd人心生一念，天地尽皆知。善恶若无报，乾坤必有私。\
　　那菩萨闻得此言，满心欢喜，对大圣道：“圣经云：‘出其言善。\
　　则千里之外应之；出其言不善，则千里之外适之。’你既有此心，待我到了东土大唐国寻一个取经的人来，教他救你。你可跟他做个徒弟，秉教伽持，入我佛门。再修正果，如何？”大圣声声道：“愿去！愿去！”菩萨道：“既有善果，我与你起个法名。”大圣道：“我已有名了，叫做孙悟空。”菩萨又喜道：“我前面也有二人归降，正是‘悟’字排行。你今也是‘悟’字，却与他相合，甚好，甚好。这等也不消叮嘱，我去也。”那大圣见性明心归佛教，这菩萨留情在意访神谱。\
　　他与木吒离了此处，一直东来，不一日就到了长安大唐国。敛雾收云，师徒们变作两个疥癫游憎，入长安城里，竟不觉天晚。行至大市街旁，见一座土地庙祠，二人径进，唬得那土地心慌，鬼兵胆战。知是菩萨，叩头接入。那土地又急跑报与城隍社令及满长安城各庙神抵，都来参见，告道：“菩萨，恕众神接迟之罪。”菩萨道：“汝等不可走漏消息。我奉佛旨，特来此处寻访取经人。借你庙宇，权住几日，待访着真僧即回。”众神各归本处，把个土地赶到城隍庙里暂住，他师徒们隐遁真形。\
　　毕竟不知寻出那个取经来，且听下回分解。";

  uint32_t a=0x12345678;
  char *pa=(char *)&a;
  printf(" *pa=0x%x \n", *(pa+3));


#if 1	///////////////  cstr/char_unicode_to_uft8( ) 	/////////////

	char char_uft8[4+1];
	char buff[2048];
	wchar_t *wp=wbook;

	char *pbuf=buff;

	#if  1 /* ------- char_unicode_to_uft8() ------- */
	memset(buff,0,sizeof(buff));
	while(*wp) {
		//memset(char_uft8,0,sizeof(char_uft8));
		k=char_unicode_to_uft8(wp, pbuf);
		if(k<=0)
			break;

		pbuf+=k;
		wp++;
	}
	printf("%s\n",buff);
	#endif

	#if  1 /* ------- cstr_unicode_to_uft8() ------- */
	memset(buff,0,sizeof(buff));
	cstr_unicode_to_uft8(wbook,buff);
	printf("%s\n",buff);
	#endif


	exit(0);



#endif

  return 0;
}

